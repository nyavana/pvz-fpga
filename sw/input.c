/*
 * Keyboard / gamepad input via /dev/input/eventX
 *
 * Uses non-blocking I/O to read events from a Linux input device.
 * Maps both keyboard keys and Xbox 360-compatible gamepad buttons
 * (xpad: BTN_DPAD_*, BTN_SOUTH/EAST, BTN_TL/TR, BTN_START, plus
 * ABS_HAT0X/Y for variants that report the D-pad as hat axes) to
 * the same INPUT_* action codes consumed by main.c.
 *
 * ===================================================================
 * 中文总览
 * ===================================================================
 *
 * 这个文件就是个"翻译器"：从 Linux evdev 事件流 -> INPUT_* 抽象代码。
 *
 * 关于 Linux evdev 一定要知道几件事：
 *   1. /dev/input/eventN 是字符设备。每读一次返回若干个 struct input_event：
 *        struct input_event {
 *            struct timeval time;   // 事件发生时间
 *            __u16 type;            // EV_KEY / EV_ABS / EV_SYN ...
 *            __u16 code;            // 按键代码 KEY_UP=103 / 按钮代码 BTN_SOUTH ...
 *            __s32 value;           // 状态值：按键 0=松开 1=按下 2=自动重复
 *        };
 *      type 和 code 都来自 <linux/input.h>，定义有上千个，我们只关心几个。
 *
 *   2. 我们只看 type == EV_KEY 且 value == 1（按下瞬间）。这样：
 *      - 松开 (value=0) 忽略 —— 不影响游戏，松开不是一个动作。
 *      - 自动重复 (value=2) 忽略 —— 防止长按 SPACE 连续种植物。
 *        如果以后要让方向键长按移动光标，可以改成 value==1 || value==2。
 *
 *   3. 手柄的 D-pad 在某些 xpad 驱动里报成 EV_ABS / ABS_HAT0X / ABS_HAT0Y
 *      （把上下左右当成轴）。这种情况 value 是 -1 / 0 / +1，0 表示松开。
 *      所以 ABS 分支里要看 value 的正负号，0 直接 continue 掉。
 *
 *   4. 文件用 O_NONBLOCK 打开（见 input_init），read 没数据时返回 -1，
 *      errno = EAGAIN/EWOULDBLOCK。这是非阻塞 IO 的关键点：游戏循环不能阻塞。
 *
 * 调用模式：
 *   main.c 的 process_input 里:
 *       while ((key = input_poll()) != INPUT_NONE) { ... }
 *   每帧把队列里堆积的事件全抽干。每次 input_poll 顶多返回一个事件，
 *   "排干"在调用方做，比让 input_poll 内部 batch 处理简单很多。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      /* O_RDONLY、O_NONBLOCK 标志位都在这里 */
#include <unistd.h>     /* read、close */
#include <linux/input.h> /* struct input_event、EV_KEY、KEY_*、BTN_* 等都在这里 */
#include "input.h"

/*
 * 模块级文件描述符 —— 整个进程只打开一个输入设备。
 * 用 -1 当"未初始化"哨兵：
 *   - input_poll 看到 < 0 直接返回 INPUT_NONE，不会瞎 read 一个非法 fd
 *   - input_close 看到 < 0 跳过 close，避免重复关闭
 *   - input_init 失败后保持 -1，让程序后续逻辑还能正常关 fpga 退出
 *
 * 用 static 把它限制在文件作用域，外部模块没法乱动 —— input.h 的接口里
 * 不暴露 fd，这是好的封装。
 */
static int input_fd = -1;

int input_init(const char *device_path)
{
    /*
     * O_RDONLY  —— 只读，evdev 设备本来就只能读事件。
     * O_NONBLOCK —— 非阻塞模式。后续 read 没数据立刻返回 -1 + EAGAIN，
     *               这样 60Hz 的游戏循环不会被堵在这里。
     *
     * device_path 一般是 "/dev/input/event0"。在 DE1-SoC 板子上需要 root
     * 或者用户在 "input" 组里才能打开（rwxr-xr-x root:input 644）。
     */
    input_fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (input_fd < 0) {
        /* perror 会自动打印 errno 对应的错误描述，例如:
         *   "input_init: open: No such file or directory"
         *   "input_init: open: Permission denied"
         * 这对 debug 板子上插哪个 USB 口很有用。 */
        perror("input_init: open");
        return -1;
    }
    return 0;
}

int input_poll(void)
{
    struct input_event ev;

    /* 防御性：fd 没初始化好就直接告诉调用方"没事件" */
    if (input_fd < 0)
        return INPUT_NONE;

    /*
     * read 的返回值：
     *   == sizeof(ev)  完整读到一个事件，进入 while 体处理。
     *   == 0           设备 EOF（一般不会发生在 evdev）。
     *   < 0            errno = EAGAIN/EWOULDBLOCK 表示队列空，跳出循环。
     *                  其他 errno（极少）也跳出，反正这帧没事件。
     *
     * 注意 while 条件是 "== sizeof(ev)"，部分读取（不太可能）也会跳出。
     *
     * 为什么用 while 而不是 if？
     *   因为有些事件（比如 EV_SYN 同步标记、或者我们不认识的按键）需要被
     *   "消耗掉"才能读到下一个有意义的事件 —— 否则它会一直堵在队列前面。
     *   循环里所有 case 都用 return，命中第一个就把这次结果给调用方；
     *   不认识的事件 continue 跳到下一次循环，继续往后读。
     */
    while (read(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        /* Some xpad variants report the D-pad as absolute hat axes
         * rather than BTN_DPAD_* keys. value == 0 is the release
         * event; ignore it so an idle hat doesn't spam INPUT_NONE. */
        /*
         * 中文：手柄方向键的两种实现方式。
         *
         * 有的 xpad 驱动把 D-pad 报成 EV_KEY + BTN_DPAD_UP/DOWN/LEFT/RIGHT
         * （正常按键模式，下面 switch 处理），有的报成 EV_ABS + ABS_HAT0X/Y
         * （把方向键当成"轴"，value 取 -1/0/+1）。这里两种都兼容。
         *
         * ABS_HAT0X 报"水平方向"：value < 0 表示左，value > 0 表示右。
         * ABS_HAT0Y 报"垂直方向"：value < 0 表示上，value > 0 表示下。
         *   （注意 Y 是反的 —— 屏幕坐标 Y 向下增大但摇杆 Y 向上为负，
         *    evdev 沿用了"屏幕坐标系"约定。）
         *
         * value == 0 是"松开/回中"，不当作动作；continue 让循环继续读下一个事件，
         * 否则手柄空闲时一直冒出 0 会让我们什么都不返回，浪费循环。
         */
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_HAT0X) {
                if (ev.value < 0) return INPUT_LEFT;
                if (ev.value > 0) return INPUT_RIGHT;
            } else if (ev.code == ABS_HAT0Y) {
                if (ev.value < 0) return INPUT_UP;
                if (ev.value > 0) return INPUT_DOWN;
            }
            continue;  /* 不是 HAT 轴，或者 value == 0，跳过 */
        }

        /* Only handle key/button press events (value == 1) */
        /*
         * 中文：只处理"按下瞬间"。
         *   ev.type != EV_KEY 跳过：可能是 EV_SYN 同步标记或别的我们不关心的类型。
         *   ev.value != 1 跳过：
         *     value == 0 是松开 —— 游戏里所有动作都是脉冲式的，不需要松开事件。
         *     value == 2 是 auto-repeat —— 内核帮你模拟"长按重复"，
         *                 但游戏里 SPACE / TAB 长按一直触发会很烦，所以不要。
         */
        if (ev.type != EV_KEY || ev.value != 1)
            continue;

        /*
         * 按键代码到 INPUT_* 的映射表。
         *
         * 数值参考 <linux/input-event-codes.h>（按系统不同路径有差异）：
         *   KEY_ESC   =  1     KEY_TAB   = 15    KEY_D     = 32
         *   KEY_SPACE = 57     KEY_UP    =103    KEY_LEFT  =105
         *   KEY_RIGHT =106     KEY_DOWN  =108
         *
         * 手柄按钮（xpad 驱动报上来的，跟 Xbox 360 手柄按键名约定一致）：
         *   BTN_DPAD_UP/DOWN/LEFT/RIGHT  —— 方向键（如果不是 HAT 模式）
         *   BTN_SOUTH                   —— "南键"，对应 Xbox 的 A 键，
         *                                  这里映射成 SPACE（确认/种植物）
         *   BTN_EAST                    —— "东键"，对应 Xbox 的 B 键，
         *                                  映射成 D（取消/铲除）
         *   BTN_START                   —— Start 键，映射成 ESC（退出）
         *   BTN_TL / BTN_TR             —— 左右肩键 L1/R1，都映射成 TAB
         *                                  （切换植物），双绑让左右手都顺手
         *
         * default: break 故意保留 —— 不认识的按键不返回也不 return，
         * 循环继续往下读，让下一个有意义的事件能被处理。
         */
        switch (ev.code) {
        case KEY_UP:    case BTN_DPAD_UP:    return INPUT_UP;     /* 103 / 手柄上 */
        case KEY_DOWN:  case BTN_DPAD_DOWN:  return INPUT_DOWN;   /* 108 / 手柄下 */
        case KEY_LEFT:  case BTN_DPAD_LEFT:  return INPUT_LEFT;   /* 105 / 手柄左 */
        case KEY_RIGHT: case BTN_DPAD_RIGHT: return INPUT_RIGHT;  /* 106 / 手柄右 */
        case KEY_SPACE: case BTN_SOUTH:      return INPUT_SPACE;  /*  57 / 手柄 A 键 */
        case KEY_D:     case BTN_EAST:       return INPUT_D;      /*  32 / 手柄 B 键 */
        case KEY_ESC:   case BTN_START:      return INPUT_ESC;    /*   1 / 手柄 Start */
        case KEY_TAB:   case BTN_TL: case BTN_TR: return INPUT_TAB; /* 15 / 手柄 L1/R1 */
        default:        break;  /* 不认识的按键，循环继续读下一个事件 */
        }
    }

    /*
     * 走到这里说明 read 返回 != sizeof(ev) —— 队列空了（EAGAIN）
     * 或者发生罕见错误。告诉调用方"这一帧没事件了"。
     */
    return INPUT_NONE;
}

void input_close(void)
{
    /*
     * 关闭设备文件并把哨兵复位为 -1。
     *
     * 检查 >= 0 让"重复调用"安全：main.c 退出路径上调一次，但万一以后
     * 加了异常退出分支再调一次也不会 close(-1) 出错。
     *
     * 关闭后内核会自动把事件队列也丢掉，不需要单独 drain。
     */
    if (input_fd >= 0) {
        close(input_fd);
        input_fd = -1;
    }
}
