/*
 * Plants vs Zombies — Main game loop
 *
 * Initializes the FPGA driver, keyboard input, and game state,
 * then runs a 60 Hz loop: input -> update -> render -> write to FPGA.
 *
 * Usage: ./pvz [/dev/input/eventN]
 *
 * ===================================================================
 * 中文总览 —— main.c 是程序入口，负责"把所有模块拼起来"
 * ===================================================================
 *
 * 这个文件本身不实现任何游戏逻辑。它只做四件事：
 *   1. 初始化：开 /dev/pvz (FPGA 驱动)、开键盘事件设备、初始化 render、
 *      初始化 game。
 *   2. 主循环：60Hz 跑 input -> update -> render -> 睡眠补齐这一帧。
 *   3. 状态打印：每秒往 stdout 打一行调试信息。
 *   4. 退出处理：赢/输/ESC 都走 cleanup 路径。
 *
 * 跨文件依赖：
 *   - game.h / game.c        —— 提供 game_init / game_update 等逻辑。
 *   - input.h / input.c      —— 提供 input_init / input_poll / input_close
 *                                封装 Linux /dev/input/eventN 的读取。
 *   - render.h / render.c    —— 提供 render_init / render_frame。
 *                                render_frame 把 game_state_t 翻译成 51 个
 *                                32 位寄存器字，通过 ioctl(fd, PVZ_WRITE_REG, ..)
 *                                推给 FPGA 里 pvz_top.sv 的 Avalon-MM 从设备。
 *   - pvz.h                  —— 定义 ioctl 编号 PVZ_WRITE_REG 和寄存器表。
 *                                render.c 用到，main.c 这里只是间接 include。
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "pvz.h"
#include "game.h"
#include "input.h"
#include "render.h"

#define FRAME_USEC 16667  /* ~60 fps */
/*
 * 中文：FRAME_USEC 是"一帧应该占用多少微秒"。
 *      1 秒 = 1,000,000 微秒，除以 60 fps ≈ 16666.67，取整到 16667。
 *      主循环用它和真实运行耗时比较，决定 usleep 多少补齐这一帧。
 *      注意取整误差：每帧多睡 0.33 微秒，每秒少跑 0.33*60 ≈ 20us，
 *      所以理论上实际帧率比 60 略低，但完全在容差内。
 */

static int pvz_fd;
/*
 * 中文：pvz_fd 是 /dev/pvz 的文件描述符，全局静态变量。
 *      main 打开它后传给 render_init，render.c 里就靠它做 ioctl。
 *      main 退出前用 close(pvz_fd) 关闭。
 *      为什么放全局？因为只有一个 FPGA 设备，全局比到处传简单。
 */

/* Get current time in microseconds */
/*
 * get_time_usec —— gettimeofday 的便捷包装，返回 int64 微秒数。
 * 高层：用来做帧率限制 (frame pacing)。开始的时候 frame_start = get_time_usec()，
 *      做完 update/render 之后再读一次算 elapsed，用 FRAME_USEC - elapsed
 *      决定睡多久。
 * 低层：gettimeofday 写 struct timeval { tv_sec, tv_usec }；
 *      合并成 sec*1e6 + usec 得到一个连续递增的 int64。
 *      long long 至少 64 位，存得下未来很多年的 usec 数 (2^63/1e6/86400/365 ≈ 29 万年)。
 *      gettimeofday 不保证单调（NTP 调时可能跳），但游戏内 8 秒级别的
 *      用途下没什么影响。
 */
static long long get_time_usec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
}

/*
 * process_input —— 把本帧攒下来的按键全部消化掉。
 *
 * 高层：input_poll() 是非阻塞的，每次返回一个按键 (INPUT_UP / INPUT_SPACE 等)
 *      或 INPUT_NONE。while 循环把这一帧累积的所有事件都处理完再退出 —— 这样
 *      避免了"按键事件积压、几秒钟后才生效"的延迟感。
 *
 * 低层：每个 case 改 game_state_t 里的某个字段。游戏逻辑不在这里 —— 这里
 *      只是把按键翻译成"对 gs 的修改请求"。
 *
 *   方向键          —— 光标在 4x8 网格里走。带边界检查避免越界。
 *   SPACE           —— 调 game_place_plant(gs)，在光标位置种当前选中的植物。
 *                      返回值忽略 (失败也不报错，用户自己看 HUD 阳光数能猜到原因)。
 *   D               —— 调 game_remove_plant(gs) 把光标位置的植物挖掉。
 *   TAB             —— 在 PEASHOOTER / SUNFLOWER 之间切换 selected_plant_type。
 *                      这个字段有两个用处：
 *                        1) game_place_plant 用它决定种什么；
 *                        2) render.c 把它写到 PVZ_REG_SELECTED (word 50)，
 *                           硬件 entity_drawer.sv 用它决定 HUD 上哪个图标
 *                           画黄色描边。
 *   ESC             —— 把 state 设成 -1，主循环 while(gs.state >= 0) 会退出。
 *                      用 -1 是因为 STATE_PLAYING/WIN/LOSE 都是 0/1/2，-1 不冲突。
 */
static void process_input(game_state_t *gs)
{
    int key;

    while ((key = input_poll()) != INPUT_NONE) {
        switch (key) {
        case INPUT_UP:
            if (gs->cursor_row > 0) gs->cursor_row--;
            break;
        case INPUT_DOWN:
            if (gs->cursor_row < GRID_ROWS - 1) gs->cursor_row++;
            break;
        case INPUT_LEFT:
            if (gs->cursor_col > 0) gs->cursor_col--;
            break;
        case INPUT_RIGHT:
            if (gs->cursor_col < GRID_COLS - 1) gs->cursor_col++;
            break;
        case INPUT_SPACE:
            game_place_plant(gs);
            break;
        case INPUT_D:
            game_remove_plant(gs);
            break;
        case INPUT_TAB:
            gs->selected_plant_type =
                (gs->selected_plant_type == PLANT_PEASHOOTER)
                ? PLANT_SUNFLOWER : PLANT_PEASHOOTER;
            break;
        case INPUT_ESC:
            gs->state = -1; /* signal exit */
            return;
        }
    }
}

/*
 * main —— 程序入口。
 *
 * 命令行：./pvz [/dev/input/eventN]
 *   - 不带参数则默认用 /dev/input/event0。
 *   - 板子上不同的 USB 键盘可能挂在 event0/1/2，跑之前可以先用
 *     `cat /proc/bus/input/devices` 查一下。
 *
 * 初始化顺序很有讲究：
 *   1. srand(time(NULL))     —— 随机种子，game.c 里的 random_range 用。
 *                               每次启动种子都不一样，僵尸出生顺序也不一样。
 *   2. open("/dev/pvz")      —— 打开 FPGA 驱动设备节点。这一步失败几乎
 *                               肯定是因为没 insmod pvz_driver.ko，所以
 *                               专门给一行友好提示。
 *   3. input_init            —— 打开键盘 event 设备。失败时记得 close(pvz_fd)
 *                               防止 fd 泄漏。
 *   4. render_init(pvz_fd)   —— 把 fd 存到 render.c 的静态变量里。
 *                               render.c 里没有 render_close，因为它只是
 *                               借用 fd，关闭是 main 的责任。
 *   5. game_init(&gs)        —— 把游戏世界状态清零并设默认。
 */
int main(int argc, char *argv[])
{
    const char *input_dev = "/dev/input/event0";
    game_state_t gs;

    if (argc > 1)
        input_dev = argv[1];

    /* Seed random number generator */
    /* 中文：每次启动用当前时间播种，让 game.c 里的 random_range 每局不同。 */
    srand((unsigned)time(NULL));

    /* Open FPGA device */
    /*
     * 中文：/dev/pvz 是 pvz_driver.c 注册的 misc 设备。
     *      O_RDWR 是因为驱动支持 ioctl 双向 (虽然这个项目实际只 write)。
     *      失败 -> 大概率是没 insmod pvz_driver.ko，给出明确指引。
     */
    pvz_fd = open("/dev/pvz", O_RDWR);
    if (pvz_fd < 0) {
        perror("open /dev/pvz");
        fprintf(stderr, "Is the pvz_driver module loaded?\n");
        return 1;
    }

    /* Initialize keyboard input */
    /*
     * 中文：input_init 打开 /dev/input/eventN，把它设成 O_NONBLOCK。
     *      失败常见原因：路径不存在、权限不够 (需要 root 或 input 组)。
     *      失败时记得先 close(pvz_fd) 再返回。
     */
    if (input_init(input_dev) < 0) {
        fprintf(stderr, "Failed to open %s\n", input_dev);
        fprintf(stderr, "Usage: %s [/dev/input/eventN]\n", argv[0]);
        close(pvz_fd);
        return 1;
    }

    /* Initialize renderer */
    /*
     * 中文：把 pvz_fd 存到 render.c 内部静态变量里。后面 render_frame
     *      用它做 ioctl。没有对应的 render_close 函数 —— 关 fd 是 main 的事。
     */
    render_init(pvz_fd);

    /* Initialize game state */
    /* 中文：清零并设默认 (sun=100, 光标 (0,0), 选中 Peashooter, 等等)。 */
    game_init(&gs);

    printf("Plants vs Zombies MVP\n");
    printf("Controls: Arrows=move, Tab=toggle plant, Space=place, D=remove, ESC=quit\n");
    printf("Sun: %d | Plant cost: %d\n\n", gs.sun, PLANT_COST);

    /* Main game loop */
    /*
     * 中文：主循环。每次迭代 = 1 帧。
     * while 条件 gs.state >= 0 的含义：
     *   - STATE_PLAYING(0) / STATE_WIN(1) / STATE_LOSE(2) 都 >=0，继续。
     *   - ESC 时 process_input 把 state 设成 -1，循环退出。
     *   - WIN/LOSE 状态下其实下面的判定会 break，所以 state>=0 主要拦 ESC。
     *
     * 一帧分四步：input -> update -> render -> 睡眠补齐 16667us。
     */
    long long frame_start;

    while (gs.state >= 0) {
        frame_start = get_time_usec();
        /* 中文：记下这一帧开始的时间戳，用来算 elapsed 决定 usleep 多久。 */

        /* 1. Process input */
        /* 中文：消化本帧所有累积的按键，可能修改 cursor / 种植 / 切换 / ESC。 */
        process_input(&gs);
        if (gs.state < 0)
            break;

        /* 2. Update game logic */
        /*
         * 中文：推进游戏世界 1 帧。这是纯软件计算，不碰硬件。
         *      game_update 内部跑 sun/spawn/firing/projectile/zombie/collision/win
         *      七个子阶段，详见 game.c。
         */
        game_update(&gs);

        /* 3. Render to FPGA */
        /*
         * 中文：把 gs 翻译成 51 个 32 位寄存器字，通过 ioctl 推给 FPGA。
         *      render_frame 在 render.c 里实现 —— 它依次调
         *        ioctl(pvz_fd, PVZ_WRITE_REG, &arg)
         *      把 plant/sunflower 位图、僵尸/豌豆槽位、光标、阳光、选中类型
         *      全部刷一遍。FPGA 端 pvz_top.sv 的 always_ff 把这些字解码到
         *      内部寄存器，下一帧 VGA 扫描时 entity_drawer.sv 就用新值画像素。
         */
        render_frame(&gs);

        /* Print status periodically */
        /*
         * 中文：每 60 帧 (= 1 秒) 在 stdout 打一行状态。\r 让光标回行首，
         *      下一次 printf 覆盖上一行，看起来像"实时更新一行"。
         *      fflush(stdout) 强制冲掉缓冲区 —— stdout 是行缓冲，但因为我们
         *      没打 \n，所以不主动 flush 看不到输出。
         */
        if (gs.frame_count % 60 == 0) {
            printf("\rSun: %3d | Zombies: %d/%d | Frame: %d",
                   gs.sun, gs.zombies_spawned, TOTAL_ZOMBIES,
                   gs.frame_count);
            fflush(stdout);
        }

        /* Check game over */
        /*
         * 中文：胜利/失败处理。注意这里都额外调一次 render_frame —— 因为
         *      STATE_WIN/LOSE 被设上之后，gs 内的实体可能已经变化 (比如
         *      最后一只僵尸刚死)，要把这"最后一帧"也推给 FPGA 玩家才能看到。
         *      然后 sleep(5) 让玩家看 5 秒结果再退出。
         */
        if (gs.state == STATE_WIN) {
            printf("\n\n*** YOU WIN! All zombies defeated! ***\n");
            render_frame(&gs); /* Render win state */
            sleep(5);
            break;
        }
        if (gs.state == STATE_LOSE) {
            printf("\n\n*** GAME OVER! A zombie reached your house! ***\n");
            render_frame(&gs); /* Render lose state */
            sleep(5);
            break;
        }

        /* 4. Frame timing: sleep for remainder of frame */
        /*
         * 中文：穷人版帧率限制器。
         * 算法：
         *   elapsed = 现在 - frame_start   (= 这帧 update+render 实际花了多久)
         *   if (elapsed < 16667) usleep(16667 - elapsed);
         * 限制：
         *   - 不补偿：如果某一帧跑超了 16667，下一帧不会缩短补回来 ——
         *     单纯下一帧也按 16667 走，结果整体帧率会"被拖慢"。
         *     正版游戏引擎一般会做累积时间 + catch-up，这里简化处理。
         *   - usleep 的精度本身只有约 100 微秒，所以实际帧率会比 60 略低
         *     (估计 58~60 之间)，足够这个项目了。
         */
        long long elapsed = get_time_usec() - frame_start;
        if (elapsed < FRAME_USEC)
            usleep(FRAME_USEC - elapsed);
    }

    /*
     * 中文：清理阶段。顺序和 init 反过来：
     *   input_close  —— 关键盘 event fd。
     *   close(pvz_fd) —— 关 FPGA 设备 fd。注意 render.c 没自己的 close，
     *                    所以这里 close 之后 render_frame 就不能再调了。
     * 没有显式 free —— gs 是栈上的局部变量，函数返回自动释放。
     */
    printf("\nCleaning up...\n");
    input_close();
    close(pvz_fd);
    return 0;
}
