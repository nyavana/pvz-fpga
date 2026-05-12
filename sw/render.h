#ifndef _RENDER_H
#define _RENDER_H

#include "game.h"

/*
 * ===================================================================
 * 中文总览 —— render.h 是"软件 -> FPGA 寄存器"的桥
 * ===================================================================
 *
 * 这个模块的角色：把 game_state_t（纯 C struct，软件世界）翻译成 51 个
 * 32 位寄存器写入（硬件世界）。它是整个系统里最重要的"接缝"之一 ——
 * 上面是抽象的"植物在 (row=1, col=3)，第 2 个僵尸 x=200 像素"，下面是
 * 一摞 ioctl 调用，最终落到 FPGA 的 always_ff 寄存器组里。
 *
 * 上下游：
 *   - 上游：sw/main.c 每帧调 render_frame(&gs) 一次，60Hz。
 *   - 下游：sw/pvz_driver.c::pvz_ioctl 接 PVZ_WRITE_REG，转成
 *           iowrite32(virtbase + word_index*4)。再下面就是 Avalon-MM
 *           总线 -> hw/pvz_top.sv 的 always_ff 写解码 -> 寄存器组。
 *           hw/entity_drawer.sv 每条扫描线读这些寄存器决定每个像素颜色。
 *
 * 设计选择：不做 dirty 跟踪。
 *   每帧无脑写全部 51 个寄存器。看起来浪费，但：
 *     - 51 * 一次 ioctl 大概几十微秒，远快于 16.67ms 帧时间
 *     - 实现简单，没有"软件状态和硬件状态不一致"的 bug 类
 *     - 没有 vsync 锁存，写得太聪明也救不了 tearing（pvz_top.sv 注释里
 *       承认会有 1 帧 tearing，60Hz 下肉眼看不见）
 */

/*
 * Initialize the renderer (set up the FPGA fd).
 * Returns 0 on success, -1 on failure.
 *
 * 中文：把已经打开好的 /dev/pvz 的 fd 存到模块内部的 static 变量里。
 *   - main.c 先 open("/dev/pvz", O_RDWR) 拿到 fd，再传给我们
 *   - 这样后续的 render_frame 不用每次都接受 fd 参数
 *   - 同一进程只能有一个 FPGA 设备，所以一个 static 就够
 *   - 返回值现在永远是 0；预留 -1 给将来如果要做参数校验或多设备
 */
int render_init(int fpga_fd);

/*
 * Render the full game state to the FPGA.
 * Converts game state into background cells + shape table entries
 * and writes them via ioctls.
 *
 * 中文：一帧渲染的入口。被 main.c 每帧调一次。
 *
 * 高层：把整个 game_state_t 翻译成对 FPGA 寄存器的 51 次写。所有
 *      渲染相关的"硬件细节"都封装在这个函数里 —— 上层只要管游戏逻辑。
 *
 * 低层：内部按 6 个 phase 顺序写：
 *   1. plant_present / sunflower_present 位图（word 0、1）
 *   2. 8 个 zombie 槽位（word 32..39）
 *   3. 8 个 pea 槽位（word 40..47）
 *   4. cursor（word 48）
 *   5. sun（word 49）
 *   6. selected（word 50）
 *
 * 输/赢画面会调 hide_all_entities 把实体全部清零，只留 sun + selected
 * （留 HUD 给玩家看最终阳光数和当时选的植物，将来加结算画面会用）。
 */
void render_frame(const game_state_t *gs);

#endif /* _RENDER_H */
