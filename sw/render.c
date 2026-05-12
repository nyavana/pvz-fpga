/*
 * Game state -> FPGA register file
 *
 * Each frame we walk the game state and write one 32-bit value per
 * entity into the hardware register file via the PVZ_WRITE_REG ioctl.
 * No commit handshake: the entity_drawer reads whatever is in the
 * registers as it scans each line.
 *
 * Hardware capacity: 32 plant cells, 8 zombies, 8 peas, 1 cursor.
 *
 * ===================================================================
 * 中文总览 —— 这个文件是 SW <-> HW 之间最重要的"翻译层"
 * ===================================================================
 *
 * 数据流（看一遍这个心里就有数了）：
 *
 *   game.c 维护 game_state_t（C 结构体，纯软件世界）
 *      |
 *      v
 *   render.c 每帧调 render_frame(&gs) 把它打包成 51 个 32 位字
 *      |
 *      | ioctl(fpga_fd, PVZ_WRITE_REG, &arg)   <-- 在 write_reg 函数里
 *      v
 *   sw/pvz_driver.c 的 pvz_ioctl
 *      |
 *      | iowrite32(value, virtbase + word_index * 4)
 *      v
 *   Avalon-MM bridge（在 SoC 内部，FPGA fabric 端）
 *      |
 *      v
 *   hw/pvz_top.sv 的 always_ff @(posedge clk)：
 *      根据 address (= word_index) 把 writedata 拆进 plant_present[31:0]、
 *      zombie_alive[i]/zombie_x[i]/zombie_row[i]、cursor_*、sun_value、
 *      selected_plant 等寄存器。
 *      |
 *      v
 *   hw/entity_drawer.sv 每个 VGA 像素都读这些寄存器，决定该像素颜色：
 *      bg -> plant sprite -> pea -> zombie sprite -> cursor border ->
 *      sun HUD -> 植物选择 HUD 框 -> 最终 VGA RGB
 *
 * 关键点：
 *   1. 没有 vsync 锁存 —— 写到一半 FPGA 还在扫描的话，那一帧可能看到
 *      旧/新数据混合（pvz_top.sv 里有说明）。60Hz + 51 次写很快，
 *      肉眼基本看不出来。
 *   2. 没有 dirty 检查 —— 每帧把所有 51 个寄存器都重新写一遍。
 *      简单粗暴，但够用：FPGA 那边都是组合逻辑读，每次写都是覆盖。
 *   3. 寄存器编号、位布局、pack 格式必须和 hw/pvz_top.sv 一致 ——
 *      这些都集中在 sw/pvz.h 里，是 SW 和 HW 共用的"ABI 声明"。
 */

#include <stdio.h>
#include <stdint.h>       /* uint32_t */
#include <sys/ioctl.h>    /* ioctl(); 实际命令 PVZ_WRITE_REG 在 pvz.h 里 */
#include "render.h"
#include "pvz.h"          /* 寄存器编号 + pack 函数，是和硬件共用的 ABI */

/*
 * 模块级 fd，存放 /dev/pvz 的文件描述符。
 *
 * 为什么用 static？
 *   - 整个进程只对应一个 FPGA 设备，没必要把 fd 当参数到处传。
 *   - 限制在文件作用域，外部模块碰不到，封装更干净。
 *   - render_init 写入，write_reg 读出。
 *
 * 没有显式初始化 = 0；fd=0 是合法的（stdin），所以严格说应该初始化成 -1，
 * 但 render_init 在 main.c 里早于第一次 render_frame 调用，实际不会出问题。
 */
static int fd;

/*
 * write_reg —— 所有寄存器写的唯一入口（chokepoint）。
 *
 * 高层：把这个函数当作"软件世界向硬件世界投递一个字"的统一接口。
 *      render_frame 里所有 write_reg 都经过这里 —— 想加日志、加边界
 *      检查、加 mock 测试都只改这一个函数。
 *
 * 低层：
 *   1. 构造 pvz_write_arg_t（定义在 pvz.h）：{ word_index, value }。
 *   2. ioctl(fd, PVZ_WRITE_REG, &w) —— PVZ_WRITE_REG 是用 _IOW('p', 1, ...)
 *      在 pvz.h 里定义的命令号。
 *   3. 内核侧 sw/pvz_driver.c 的 pvz_ioctl 拿到 cmd == PVZ_WRITE_REG 后:
 *        copy_from_user(&arg, user_ptr, sizeof(arg));
 *        if (arg.word_index < PVZ_NUM_REGS)
 *            iowrite32(arg.value, virtbase + arg.word_index * 4);
 *   4. iowrite32 触发一次 Avalon-MM 写事务，传到 FPGA 端。
 *   5. hw/pvz_top.sv 的 always_ff 在 clk 上升沿（chipselect && write）
 *      根据 address 把 writedata 存进对应寄存器。
 *
 * 没检查 ioctl 返回值 —— MVP 阶段假设驱动稳定。生产代码应该 if (ret < 0) perror。
 */
static void write_reg(unsigned int word_index, unsigned int value)
{
    pvz_write_arg_t w = { .word_index = word_index, .value = value };
    ioctl(fd, PVZ_WRITE_REG, &w);
}

int render_init(int fpga_fd)
{
    /*
     * main.c 已经 open("/dev/pvz", O_RDWR) 拿到 fd，传给我们存起来。
     * 这一步是为了让后续 write_reg 不用每次都接 fd 参数 ——
     * "依赖注入"的最简形式。
     */
    fd = fpga_fd;
    return 0;  /* 现在没有失败路径；返回类型留着以后做参数校验时用 */
}

/*
 * render_plants —— 把 4x8 网格里的植物种类压缩成两个 32 位位图。
 *
 * 高层：游戏侧用二维数组 gs->grid[r][c] 维护植物（更直观），但硬件用
 *      bitmap 表达（更省寄存器、读起来更快）。这里就是这两种表示之间的转换。
 *
 * 低层：
 *   - 走完整个 4x8 = 32 个格子，每格看 plant.type：
 *       PLANT_PEASHOOTER -> pea_bits 的第 (r*8+c) 位置 1
 *       PLANT_SUNFLOWER  -> sun_bits 的第 (r*8+c) 位置 1
 *   - 位索引 r*8+c 的约定必须和 hw/entity_drawer.sv 一致：
 *       那边定义 plant_idx = {cell_row, cell_col}（row 高位、col 低位）
 *       拼接成 5 位 = row*8+col，跟我们这里一样。
 *   - 4x8=32 个 bit 刚好塞进一个 32 位字 —— 这也是网格选 8 列的原因之一。
 *
 * 两个 write_reg 的硬件后果：
 *   PVZ_REG_PLANTS (word 0) -> hw/pvz_top.sv 里 plant_present <= writedata。
 *     entity_drawer.sv 在每个像素检查当前格的 plant_present 位，
 *     如果为 1 就从 peashooter_idx.mem 的 sprite ROM 取像素来画。
 *   PVZ_REG_SUNFLOWER (word 1) -> sunflower_present，sprite 是 sunflower_idx.mem。
 *   两个位图独立写 —— 同一格子两位都为 1 时，entity_drawer.sv 的最终 mux
 *   里 sunflower 在 peashooter 之后画，所以会覆盖（但游戏逻辑保证不冲突）。
 */
static void render_plants(const game_state_t *gs)
{
    /* pea_bits 累积 Peashooter 的位图，sun_bits 累积 Sunflower 的位图 */
    uint32_t pea_bits = 0;
    uint32_t sun_bits = 0;
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int t = gs->grid[r][c].type;
            /* 位号 r*8+c 是约定（见 pvz.h 注释、hw/entity_drawer.sv plant_idx 拼接）*/
            if (t == PLANT_PEASHOOTER)
                pea_bits |= (1u << (r * 8 + c));
            else if (t == PLANT_SUNFLOWER)
                sun_bits |= (1u << (r * 8 + c));
            /* PLANT_NONE 的格子两位都保持 0 —— 硬件就不画 sprite */
        }
    }
    /* word 0：hw/pvz_top.sv 里写解码 plant_present <= writedata */
    write_reg(PVZ_REG_PLANTS,    pea_bits);
    /* word 1：sunflower_present <= writedata */
    write_reg(PVZ_REG_SUNFLOWER, sun_bits);
}

/*
 * render_selected —— 把"当前选中的植物"告诉硬件 HUD。
 *
 * 高层：屏幕左上角有两个图标方框，一个绿色（Peashooter），一个橙色
 *      （Sunflower）。玩家按 TAB 切换 gs->selected_plant_type；这里把
 *      它编码成 0/1 写给硬件，硬件就把黄色边框画在对应的方框上。
 *
 * 低层：
 *   - game.h 里 PLANT_PEASHOOTER=1, PLANT_SUNFLOWER=2，跟硬件期望的 0/1
 *     编码不一样，所以这里做个三元运算转换。
 *   - PVZ_REG_SELECTED = word 50。
 *   - 硬件 hw/pvz_top.sv: selected_plant <= writedata[1:0]
 *     hw/entity_drawer.sv: 在 sel0_border / sel1_border 的 mux 里
 *     "if selected_plant == 0 时画 box0 的黄边、== 1 时画 box1 的黄边"。
 *     两个方框的填充色（绿/橙）一直都画，只有黄边随 selected 切换。
 *
 *   注意：这是个 14-bit-of-padding + 2-bit 值的写。硬件只读 [1:0]，
 *   高位写啥都没事。
 */
static void render_selected(const game_state_t *gs)
{
    int sel = (gs->selected_plant_type == PLANT_SUNFLOWER) ? 1 : 0;
    write_reg(PVZ_REG_SELECTED, sel);
}

/*
 * render_zombies —— 把游戏侧的 8 个僵尸槽位 push 到硬件的 8 个寄存器槽位。
 *
 * 高层：FPGA 只有 8 个僵尸寄存器槽（word 32..39），所以总有 PVZ_MAX_ZOMBIES=8
 *      个写。即使游戏侧某些槽位是 inactive，也要写一次"alive=0"把硬件那个
 *      槽位关掉，否则它会画上一帧的残影。
 *
 * 低层：
 *   - 默认 alive=0/row=0/x=0；只有当 gs->zombies[i] 还 active 时才填真实值。
 *   - i < MAX_ZOMBIES 这个判断当 PVZ_MAX_ZOMBIES <= MAX_ZOMBIES 时永远真；
 *     留在这里是防御性的 —— game.h 把 MAX_ZOMBIES 设成等于 PVZ_MAX_ZOMBIES，
 *     但如果以后软件侧扩到比硬件多，也不会越界访问数组。
 *   - pvz_pack_entity(alive, row, x) 在 pvz.h 里 inline，编码为:
 *       bit 31    = alive
 *       bit 11:10 = row (0..3)
 *       bit 9:0   = x_pixel (0..639)
 *
 *   每次 write_reg 的硬件后果：
 *     PVZ_REG_ZOMBIE(i) = word (32+i) -> hw/pvz_top.sv 里:
 *       zombie_alive[i] <= writedata[31];
 *       zombie_x[i]     <= writedata[9:0];
 *       zombie_row[i]   <= writedata[11:10];
 *     hw/entity_drawer.sv 在每条扫描线扫每个像素时，遍历 8 个 zombie 槽位，
 *     如果像素落在 [zombie_x[i], zombie_x[i]+64) x [grid_y+row*64, ...+64)
 *     这个矩形里就发起 zombie sprite ROM 读（zombie_idx.mem），1 周期后
 *     用读回的 8 位调色板索引覆盖背景色。透明像素（COL_TRANSPARENT=0xFF）
 *     不覆盖。
 */
static void render_zombies(const game_state_t *gs)
{
    for (int i = 0; i < PVZ_MAX_ZOMBIES; i++) {
        int alive = 0, row = 0, x = 0;
        /* 软件槽位活着才填真实数据，否则后面写"alive=0"关闭硬件槽位 */
        if (i < MAX_ZOMBIES && gs->zombies[i].active) {
            alive = 1;
            row   = gs->zombies[i].row;
            x     = gs->zombies[i].x_pixel;
        }
        write_reg(PVZ_REG_ZOMBIE(i), pvz_pack_entity(alive, row, x));
    }
}

/*
 * render_peas —— 把游戏侧的 16 个豌豆子弹槽位"压缩"到硬件的 8 个槽位。
 *
 * 高层：和僵尸不同，游戏侧 MAX_PROJECTILES=16 但硬件只有 PVZ_MAX_PEAS=8
 *      个寄存器槽。我们采用"按出现顺序前 8 个"的策略 —— 多出来的豌豆子弹
 *      这一帧不显示。在实际玩法里很少同时有超过 8 颗子弹飞，所以可以接受。
 *
 * 低层：
 *   1. 第一个 for 循环：扫游戏侧 16 个槽，遇到 active 的就压进硬件槽
 *      slot++（紧凑打包，没有"洞"）。
 *   2. 第二个 for 循环：硬件剩下的槽位写 0（alive=0，关闭显示），
 *      防止上一帧留下的旧子弹位置形成残影。
 *
 *   pvz_pack_entity(1, row, x_pixel) 编码和僵尸完全一样的 bit 布局。
 *
 *   PVZ_REG_PEA(slot) = word (40+slot) -> hw/pvz_top.sv 里:
 *     pea_alive[slot] <= writedata[31];
 *     pea_x[slot]     <= writedata[9:0];
 *     pea_row[slot]   <= writedata[11:10];
 *
 *   硬件画法（entity_drawer.sv）：豌豆不用 sprite ROM，直接画 PEA_SIZE=8
 *   像素的小亮绿色 (COL_BRIGHT_GREEN) 方块，垂直方向在 row 中央居中
 *   (py_top = grid_y + row*64 + 28)。比 sprite 简单很多，因为子弹只是一个小点。
 */
static void render_peas(const game_state_t *gs)
{
    /* Walk active projectiles in order, stop after PVZ_MAX_PEAS.
     * Hide hardware slots that don't get filled. */
    /*
     * 中文：按顺序扫游戏侧的子弹数组，紧凑塞进硬件的 8 个槽位。
     * 没塞满的硬件槽位下面用 0 清掉，免得画残影。
     */
    int slot = 0;
    for (int i = 0; i < MAX_PROJECTILES && slot < PVZ_MAX_PEAS; i++) {
        const projectile_t *p = &gs->projectiles[i];
        if (!p->active) continue;
        /* alive=1 是写死的：这条路径上一定是 active */
        write_reg(PVZ_REG_PEA(slot), pvz_pack_entity(1, p->row, p->x_pixel));
        slot++;
    }
    /* 剩下的硬件槽位写 0：alive=0、x=0、row=0，硬件就不画了 */
    for (; slot < PVZ_MAX_PEAS; slot++)
        write_reg(PVZ_REG_PEA(slot), 0);
}

/*
 * render_cursor —— 玩家光标（黄色边框）的位置和可见性。
 *
 * 高层：游戏进行中时光标可见，玩家用方向键移动它；输/赢画面隐藏光标。
 *      光标用"格子坐标"(row, col)，不是像素坐标 —— 硬件那边只需要画
 *      格子大小的边框，不需要像素精度。
 *
 * 低层：
 *   - visible = (state == STATE_PLAYING) ? 1 : 0
 *     在 STATE_WIN/STATE_LOSE 时设为 0；硬件 cursor_visible=0 直接不画边框。
 *   - pvz_pack_cursor(visible, row, col) 在 pvz.h 里定义，bit 布局:
 *       bit 31    = visible
 *       bit 4..2  = col (0..7)
 *       bit 1..0  = row (0..3)
 *
 *   PVZ_REG_CURSOR = word 48 -> hw/pvz_top.sv 里:
 *     cursor_visible <= writedata[31];
 *     cursor_col     <= writedata[4:2];
 *     cursor_row     <= writedata[1:0];
 *
 *   hw/entity_drawer.sv 在像素阶段算:
 *     cur_left = GRID_X + cursor_col*64 = 64 + col*64
 *     cur_top  = GRID_Y + cursor_row*64 = 112 + row*64
 *   然后判断像素是否落在这个 64x64 矩形的"边框"上（距离任一边 < 4 像素），
 *   是的话覆盖成黄色 (COL_YELLOW=4)。所以最终视觉是一个空心黄框。
 */
static void render_cursor(const game_state_t *gs)
{
    int visible = (gs->state == STATE_PLAYING) ? 1 : 0;
    write_reg(PVZ_REG_CURSOR,
              pvz_pack_cursor(visible, gs->cursor_row, gs->cursor_col));
}

/*
 * render_sun —— 当前阳光值，14 位有效。
 *
 * 高层：玩家种植物要花阳光（PLANT_COST=50），向日葵和时间自然增长会
 *      回填。屏幕顶端有一排黄色方块作为 HUD 指示，每块代表 50 阳光
 *      （hw/entity_drawer.sv 里 SUN_PER_BLOCK = 50）。
 *
 * 低层：
 *   - & 0x3FFF 把值钳到 14 位（最大 16383）。pvz.h 里寄存器声明就是 14 位有效。
 *     游戏里阳光不会涨这么高，但万一有 bug 也不会污染高位（虽然硬件也只读
 *     低 14 位，纯防御性掩码）。
 *   - PVZ_REG_SUN = word 49 -> hw/pvz_top.sv 里: sun_value <= writedata[13:0]。
 *
 *   注释里说"HW currently doesn't draw" 已经过时 —— hw/entity_drawer.sv
 *   里 sun_hit_comb 那段已经实现了：每个 i ∈ [0,9] 的方块，如果
 *   sun_value >= (i+1)*50 就点亮一个 16x24 像素的黄色块，最多 10 个块
 *   横排在屏幕右上角 (SUN_X=440, SUN_Y=24)。
 */
static void render_sun(const game_state_t *gs)
{
    /* HW currently doesn't draw the sun count; the register is reserved
     * so a future HUD module can pick it up. */
    write_reg(PVZ_REG_SUN, gs->sun & 0x3FFF);
}

/*
 * hide_all_entities —— 把硬件这边所有实体寄存器清零。
 *
 * 高层：用在 WIN/LOSE 画面 —— 游戏结束后屏幕上不应该再有植物、僵尸、
 *      豌豆、光标在动，只留背景方格和顶部 HUD（阳光数 + 植物图标）。
 *
 * 低层：
 *   - PVZ_REG_PLANTS / SUNFLOWER 写 0：所有 32 个位图位清零，没植物。
 *   - 8 个 zombie 槽位每个写 0：alive 位 = 0 -> hw/entity_drawer.sv 里
 *     zombie_alive[i] = 0，命中检测就跳过那个 i，不画僵尸。
 *   - 8 个 pea 槽位同理。
 *   - cursor 写 0：visible 位 = 0，硬件不画黄边框。
 *
 *   注意没碰 PVZ_REG_SUN 和 PVZ_REG_SELECTED —— 这两个在 render_frame
 *   里 WIN/LOSE 分支单独写（保留 HUD），所以不在这里清。
 */
static void hide_all_entities(void)
{
    write_reg(PVZ_REG_PLANTS,    0);
    write_reg(PVZ_REG_SUNFLOWER, 0);
    for (int i = 0; i < PVZ_MAX_ZOMBIES; i++)
        write_reg(PVZ_REG_ZOMBIE(i), 0);
    for (int i = 0; i < PVZ_MAX_PEAS; i++)
        write_reg(PVZ_REG_PEA(i), 0);
    write_reg(PVZ_REG_CURSOR, 0);
}

/*
 * render_frame —— 一帧渲染的顶层调度。
 *
 * 高层：被 sw/main.c 每帧调一次（60Hz）。根据当前游戏状态分两个分支：
 *      游戏中正常画一切；游戏结束就清空实体只留 HUD。
 *
 * 低层：
 *   STATE_PLAYING 路径：6 个 phase 顺序写满 51 个寄存器（无 dirty 检查）
 *     1. render_plants    -> word 0, 1（植物位图）
 *     2. render_zombies   -> word 32..39（8 个僵尸）
 *     3. render_peas      -> word 40..47（8 个豌豆子弹）
 *     4. render_cursor    -> word 48（光标）
 *     5. render_sun       -> word 49（阳光数）
 *     6. render_selected  -> word 50（选中的植物类型）
 *
 *   STATE_WIN / STATE_LOSE 路径：
 *     hide_all_entities() 把实体相关寄存器全清零 -> 屏幕上没植物、僵尸、
 *     豌豆、光标。但 HUD 仍然保留：sun（玩家最后剩多少）和 selected
 *     （TAB 选到哪个植物）。main.c 这时会 sleep(5) 显示画面后退出。
 *
 *   各 phase 间没有同步点 —— 硬件不锁存到 vsync，所以理论上 entity_drawer.sv
 *   扫描中途某 phase 写到一半会有"半新半旧"的画面。但 51 次 ioctl 通常
 *   在几百微秒内做完，远快于 16.67ms 帧周期；偶尔的 tearing 60Hz 下肉眼
 *   几乎察觉不到。如果以后要严格无 tearing，需要硬件加 vsync latch。
 */
void render_frame(const game_state_t *gs)
{
    if (gs->state == STATE_PLAYING) {
        /* 正常游戏：6 个 phase 把 51 个寄存器全写一遍 */
        render_plants(gs);
        render_zombies(gs);
        render_peas(gs);
        render_cursor(gs);
        render_sun(gs);
        render_selected(gs);
    } else {
        /* WIN / LOSE: clear everything; main loop prints a banner and
         * exits.  A future HUD pass can render a result indicator. */
        /*
         * 中文：游戏结束分支。
         *   - hide_all_entities：清掉植物/僵尸/豌豆/光标
         *   - render_sun + render_selected：HUD 保留
         *   main.c 在调完这次 render_frame 后会打印"YOU WIN"/"GAME OVER"
         *   字样，sleep(5) 让玩家看清画面，然后退出主循环。
         *   将来加结算画面（"You defeated X zombies"）的话直接在这里多 push
         *   一些寄存器（需要硬件那边先支持画文字）。
         */
        hide_all_entities();
        render_sun(gs);
        render_selected(gs);
    }
}
