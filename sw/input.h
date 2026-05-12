#ifndef PVZ_INPUT_H
#define PVZ_INPUT_H

/*
 * ===================================================================
 * 中文总览 —— input.h 是"键盘/手柄输入"的对外接口
 * ===================================================================
 *
 * 这个模块就干一件事：把 Linux evdev 事件流（来自 /dev/input/eventN）
 * 翻译成游戏能理解的 INPUT_* 整数代码。main.c 里的 process_input 拿到
 * 这些代码后改 game_state_t 的字段（cursor_row/col、selected_plant_type、
 * state 等）。
 *
 * 上下游关系：
 *   - 上游：Linux 内核 evdev 子系统，把键盘按键和手柄按钮统一成
 *           struct input_event 流。
 *   - 下游：sw/main.c 的 process_input —— 它每帧调用 input_poll 直到
 *           返回 INPUT_NONE，期间根据返回码改 game state。
 *
 * 设计上为什么这么干净？
 *   把"按了哪个键"和"游戏要做什么"解耦。input 模块完全不知道有"光标"
 *   或者"种植物"这些游戏概念，它只负责把硬件按键代码（KEY_UP=103 之类
 *   依赖 Linux 内核头文件）抽象成"游戏动作代码"。这样以后想换控制方式
 *   （比如改成 USB 手柄、网络远程控制）只动 input.c 就够了。
 */

/* Input event codes returned by input_poll() */
/*
 * input_poll() 的返回码。
 *
 * 高层：这是给 main.c 用的"游戏动作代码"。每个值代表一个抽象的操作意图，
 *      跟具体硬件按键（KEY_UP 还是 BTN_DPAD_UP）无关。input.c 里 switch
 *      映射到这里，main.c 的 process_input 再 switch 这个值决定怎么改游戏状态。
 *
 * 低层：INPUT_NONE = 0 这个安排很重要 —— 当队列里没有事件时 input_poll
 *      返回 0，main.c 的 while 循环 (key = input_poll()) != INPUT_NONE 自然停下，
 *      不用额外的"空"判断。把 NONE 放 0 是 C 里"零值即默认"的惯用法。
 */
#define INPUT_NONE   0   /* 没有事件，跳出 main.c 的轮询循环 */
#define INPUT_UP     1   /* 上：光标 row-- */
#define INPUT_DOWN   2   /* 下：光标 row++ */
#define INPUT_LEFT   3   /* 左：光标 col-- */
#define INPUT_RIGHT  4   /* 右：光标 col++ */
#define INPUT_SPACE  5   /* 空格：在当前格子种当前选中的植物（game_place_plant） */
#define INPUT_D      6   /* D 键：铲除当前格子的植物（game_remove_plant） */
#define INPUT_ESC    7   /* ESC：把 gs->state 设为 -1，main 循环退出 */
#define INPUT_TAB    8   /* TAB：在 Peashooter / Sunflower 之间切换选中植物 */

/*
 * Initialize keyboard input from a Linux input device.
 * Returns 0 on success, -1 on failure.
 *
 * 中文：打开 Linux 输入设备文件，准备读取按键事件。
 *   - device_path 一般是 "/dev/input/event0"，main.c 默认值；
 *     也可以从命令行参数传别的，比如 USB 键盘可能是 event1 / event2。
 *   - 内部用 O_RDONLY | O_NONBLOCK 打开，这样 input_poll 里的 read 在
 *     没有事件时会立即返回 -1 (errno=EAGAIN) 而不是阻塞游戏循环。
 *   - 返回 0 成功；返回 -1 时一般是设备不存在或者没权限（运行需要 root
 *     或者用户在 input 组里），错误信息通过 perror 打到 stderr。
 */
int input_init(const char *device_path);

/*
 * Poll for a keyboard event (non-blocking).
 * Returns one of the INPUT_* codes, or INPUT_NONE if no key pressed.
 *
 * 中文：取一个按键事件。这是 main.c 60Hz 循环每帧调用的核心函数。
 *
 * 高层：每次调用最多返回"一个"按键事件。main.c 的 process_input 是用
 *      while 循环把这一帧累积的事件全部抽干 —— 因为玩家可能在一帧
 *      (16.67ms) 里按了好几次键，而我们不想丢按键。
 *
 * 低层：读 /dev/input/eventN 是非阻塞的，没事件时立刻返回 INPUT_NONE。
 *      只识别"按下" (value==1)，不识别"松开" (value==0) 和"长按重复"
 *      (value==2) —— 游戏里 SPACE / D / TAB 都是脉冲式操作，长按多次
 *      触发反而很烦人。手柄方向键的 ABS_HAT0X/Y 例外处理（见 input.c）。
 *
 * 返回 INPUT_NONE 在两种情况下：
 *   1. fd < 0（input_init 没调成功）
 *   2. read 返回 -EAGAIN/EWOULDBLOCK（队列空）
 */
int input_poll(void);

/*
 * Close the input device.
 *
 * 中文：关掉 /dev/input/eventN。
 *   - main.c 退出前在 cleanup 阶段调一次。
 *   - 多次调用是安全的：内部 fd 用 -1 当哨兵，关一次就置回 -1。
 */
void input_close(void);

#endif /* PVZ_INPUT_H */
