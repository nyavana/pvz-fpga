#ifndef _PVZ_H
#define _PVZ_H

#include <linux/ioctl.h>

/*
 * Shared header for PvZ GPU kernel driver and userspace programs.
 *
 * The hardware exposes a flat 32-bit register file via Avalon-MM.
 * Software writes one word at a time using the PVZ_WRITE_REG ioctl.
 *
 * Register map (word index -> meaning):
 *    0       PLANTS               32 bits, bit i = peashooter at cell i
 *    1       SUNFLOWERS           32 bits, bit i = sunflower at cell i
 *   32..39   ZOMBIE[i]            bit 31 = alive
 *                                 bits [9:0]   = x_pixel (0..639)
 *                                 bits [11:10] = row (0..3)
 *   40..47   PEA[i]               same encoding as ZOMBIE
 *   48       CURSOR               bit 31 = visible
 *                                 bits [4:2] = col (0..7)
 *                                 bits [1:0] = row (0..3)
 *   49       SUN                  bits [13:0] = sun count
 *   50       SELECTED             bits [1:0]  = selected plant (0=pea, 1=sunflower)
 *
 * Layout constants must match hw/bg_grid.sv and hw/entity_drawer.sv.
 *
 * ===================================================================
 * 中文说明 —— 这是整个项目里最重要的一个头文件，请认真读一下。
 * ===================================================================
 *
 * 为什么这个文件这么关键？
 *   它定义了"硬件 (FPGA SystemVerilog) 跟软件 (Linux C) 之间的 ABI"。
 *   一边是 hw/pvz_top.sv 那个 Avalon-MM 从设备的写解码逻辑，
 *   另一边是 sw/render.c 每帧打包的 51 个 32 位字。如果两边对寄存器
 *   编号或字段位置的理解对不上，FPGA 上画出来的画面就会一团乱。
 *   所以这个文件被 sw/pvz_driver.c、sw/render.c、sw/test/test_shapes.c
 *   全都 #include；同时 hw/pvz_top.sv 顶上的注释也照抄了一份相同的
 *   寄存器表 —— 改这个文件时务必同步改 SystemVerilog 那边！
 *
 * 数据流（一帧的完整路径，看清楚很有帮助）：
 *   game.c 维护实体状态
 *     -> render.c 每帧把状态打包成 51 个 32 位字
 *     -> 通过 ioctl(fd, PVZ_WRITE_REG, &arg) 调用进入内核
 *     -> pvz_driver.c 的 pvz_ioctl 用 iowrite32 写到 virtbase+word*4
 *     -> Avalon-MM bridge 把这次写传到 FPGA
 *     -> hw/pvz_top.sv 的 always_ff 把 writedata 拆进对应寄存器
 *     -> hw/entity_drawer.sv 每条扫描线上读这些寄存器决定每个像素颜色
 *
 * 寄存器表的"中文翻译"（用人话再讲一遍）：
 *
 *   word 0  PLANTS（豌豆射手位图）
 *     32 个 bit 对应 4x8=32 个格子。bit i 等于 1 表示位置
 *     (row = i/8, col = i%8) 上种了一棵豌豆射手。
 *     例子：如果在 (row=1, col=3) 种豌豆，那么 bit (1*8+3)=11 会被置 1。
 *
 *   word 1  SUNFLOWERS（向日葵位图）
 *     和 word 0 完全一样的编码，只不过表示的是向日葵。
 *     两个位图独立，所以一个格子理论上可以同时被两个位图点亮，
 *     但游戏逻辑会保证同一个格子只长一种植物。
 *
 *   word 32..39  ZOMBIE[0..7]   一行一个僵尸槽位，共 8 个。
 *     bit 31     = alive，1 表示这个槽位上有活着的僵尸
 *     bit 11:10  = row（0..3，因为 4 行只需要 2 位）
 *     bit 9:0    = x_pixel（屏幕 x 坐标 0..639，10 位刚好够）
 *     注意 y 坐标不存：硬件根据 row * cell_size + grid_y 推算出来。
 *
 *   word 40..47  PEA[0..7]      豌豆子弹槽位，共 8 个，编码同僵尸。
 *
 *   word 48  CURSOR             玩家光标（黄色高亮框）
 *     bit 31     = visible（输/赢画面时设为 0 把光标藏起来）
 *     bit 4:2    = col（0..7，3 位）
 *     bit 1:0    = row（0..3，2 位）
 *     注意光标用的是格子坐标，不是像素坐标 —— 跟僵尸/豌豆不一样。
 *
 *   word 49  SUN                当前阳光值，14 位（最大 16383）
 *     MVP 阶段硬件还没画 HUD，所以这位写了暂时没人画，但保留在
 *     寄存器表里供以后做 HUD 模块直接拿。
 *
 *   word 50  SELECTED           当前选中的植物类型（HUD 黄色框指向哪个）
 *     0 = Peashooter, 1 = Sunflower
 *
 * 网格几何（必须和 hw/bg_grid.sv、hw/entity_drawer.sv 一致）：
 *   - 4 行 x 8 列，每格 64x64 像素
 *   - 整个网格左上角锚点在屏幕 (64, 112)
 *   - 所以格子 (row=r, col=c) 的左上角像素是 (64 + c*64, 112 + r*64)
 *   - 屏幕本身 640x480，剩下的空间用来放 HUD（顶上那一条阳光数+植物图标）
 */

/* Grid + screen layout */
/*
 * 网格几何常量。
 * 高层：游戏世界是 4 行 x 8 列的网格，每个格子 64x64 像素，
 *      网格左上角锚在屏幕 (64, 112)，整个屏幕 640x480。
 * 低层：这些数字必须跟 hw/bg_grid.sv（背景方格颜色 LUT）和
 *      hw/entity_drawer.sv（实体绘制逻辑）里硬编码的值一致，
 *      否则 SW 算出来的 (row, col) 跟 FPGA 画的格子位置会错位。
 *
 * 算个例子：格子 (row=2, col=5) 的左上角像素坐标 =
 *   x = PVZ_GRID_X + col * PVZ_CELL_SIZE = 64 + 5*64 = 384
 *   y = PVZ_GRID_Y + row * PVZ_CELL_SIZE = 112 + 2*64 = 240
 * 整个网格在屏幕上覆盖 x ∈ [64, 576), y ∈ [112, 368)。
 */
#define PVZ_GRID_ROWS    4   /* 4 行 */
#define PVZ_GRID_COLS    8   /* 8 列 —— 也是为什么位图能塞进一个 32 位字 */
#define PVZ_CELL_SIZE    64  /* 每个格子 64x64 像素 */
#define PVZ_GRID_X       64  /* 网格左上角的屏幕 x 坐标 */
#define PVZ_GRID_Y       112 /* 网格左上角的屏幕 y 坐标，留出顶部 HUD 空间 */
#define PVZ_SCREEN_W     640 /* VGA 分辨率宽 */
#define PVZ_SCREEN_H     480 /* VGA 分辨率高 */

/* Per-entity capacity exposed by hardware */
/*
 * 硬件允许的实体上限。
 * FPGA 里 zombie/pea 各自有 8 个固定的寄存器槽位（word 32..39 和 40..47），
 * 所以 SW 一次最多只能告诉硬件 8 个活僵尸和 8 个活豌豆子弹。
 * game.c 里的 MAX_ZOMBIES / MAX_PROJECTILES 可以大于这个值，
 * 但 render.c 在写寄存器时只会取前 8 个 active 的，多余的会被截掉。
 */
#define PVZ_MAX_ZOMBIES  8
#define PVZ_MAX_PEAS     8

/* Word indices in the register file */
/*
 * 寄存器在"字 (word) 表"里的索引（不是字节偏移！）。
 *
 * 高层：调用 ioctl 时这些常量直接当 word_index 传进去，
 *      pvz_driver.c 里会把它乘 4 算成字节偏移，然后 iowrite32 写到
 *      virtbase + word_index*4。FPGA 那边 Avalon-MM 的 address 单位
 *      是 WORDS（在 hw/pvz_top_hw.tcl 里配过 addressUnits = WORDS），
 *      所以两边对得上。
 *
 * 低层：注意 ZOMBIE 槽位从 word 32 开始，PLANTS 之后的 word 2..31
 *      其实是空的。这是有意为之 —— 给将来扩展留位置（比如以后想加
 *      第二种植物类型或者豌豆冷却时间寄存器）。当前总共用到 word 0..50，
 *      所以 PVZ_NUM_REGS = 51，pvz_driver.c 里的边界检查也用这个值。
 *
 *      ZOMBIE 和 PEA 用宏 (32+idx) / (40+idx) 是为了让 render.c 里
 *      可以写 PVZ_REG_ZOMBIE(i) 这样的优雅形式，i 从 0 到 7。
 */
#define PVZ_REG_PLANTS           0                     /* peashooter bitmap */
                                                       /* 豌豆射手位图，对应 hw/pvz_top.sv 的 plant_present[31:0] */
#define PVZ_REG_SUNFLOWER        1                     /* sunflower bitmap */
                                                       /* 向日葵位图，对应 hw/pvz_top.sv 的 sunflower_present[31:0] */
#define PVZ_REG_ZOMBIE(idx)      (32 + (idx))          /* 32..39 */
                                                       /* 8 个僵尸槽位，idx ∈ [0,7]，编码用 pvz_pack_entity */
#define PVZ_REG_PEA(idx)         (40 + (idx))          /* 40..47 */
                                                       /* 8 个豌豆子弹槽位，idx ∈ [0,7]，编码同僵尸 */
#define PVZ_REG_CURSOR           48                    /* 玩家黄色光标，用 pvz_pack_cursor */
#define PVZ_REG_SUN              49                    /* 阳光值，14 位有效 */
#define PVZ_REG_SELECTED         50                    /* 当前选中的植物类型，0=pea 1=sunflower */
#define PVZ_NUM_REGS             51                    /* 寄存器总数，driver 边界检查用 */

/* Pack a zombie/pea word: alive in bit 31, row in [11:10], x in [9:0] */
/*
 * pvz_pack_entity —— 把"一个僵尸/豌豆"的状态打包成 32 位字。
 *
 * 高层：这是 render.c 渲染僵尸/豌豆时唯一用的"封装函数"。它直接
 *      塑造了 SW 和 HW 之间这个 32 位字的格式。HW 侧（hw/pvz_top.sv 的
 *      always_ff 里）会反过来按相同的位切片拆出来。
 *
 * 位布局（必须和硬件一致）：
 *   bit 31    = alive       —— 1 表示这个槽位上的实体活着，0 表示槽位空
 *   bit 30..12 (没用)        —— 留作未来扩展（比如僵尸 hp、豌豆贴图变种）
 *   bit 11..10 = row         —— 0..3，2 位刚好够 4 行
 *   bit 9..0   = x_pixel     —— 0..639，10 位最大 1023 覆盖 VGA 640 宽度
 *
 * 为什么 alive 放在 bit 31？
 *   1. 这是个"sign bit"的位置，在 SystemVerilog 里 writedata[31] 一行就能拿。
 *   2. 这样把"是否活着"这个最重要的标志放在一眼能看出的位置（最高位），
 *      调 dmesg/dump 寄存器值的时候 0x80000xxx vs 0x00000xxx 一眼分辨。
 *
 * 为什么每个字段都 & 一个掩码？
 *   防御性编程 —— 如果上层传了越界的值（比如 row=5），位与之后只保留
 *   合法位，不会污染相邻字段。alive & 1、row & 3 (= 0b11)、x_pixel & 0x3FF
 *   (= 10 位全 1) 各自把输入约束到正确宽度。
 *
 * 调用例：pvz_pack_entity(1, 2, 384) 表示"第 2 行 x=384 像素上有个活实体"
 *        = (1<<31) | (2<<10) | 384 = 0x80000980。
 */
static inline unsigned int pvz_pack_entity(int alive, int row, int x_pixel)
{
    return ((unsigned)(alive & 1) << 31) |
           ((unsigned)(row   & 3) << 10) |
           ((unsigned)(x_pixel & 0x3FF));
}

/* Pack a cursor word: visible in bit 31, col in [4:2], row in [1:0] */
/*
 * pvz_pack_cursor —— 把光标状态打包成 32 位字。
 *
 * 高层：光标和"实体"用的是不同的位布局，因为光标的 x 用的是格子坐标
 *      (col)，不像僵尸/豌豆要精确的像素 x。这样硬件画黄色边框的时候
 *      只需要简单算 col*64+64、row*64+112 就能得到边框矩形。
 *
 * 位布局：
 *   bit 31    = visible      —— 0 时硬件不画光标（用于赢/输画面隐藏光标）
 *   bit 30..5 (没用)
 *   bit 4..2  = col          —— 0..7，3 位
 *   bit 1..0  = row          —— 0..3，2 位
 *
 * 为什么和实体格式不一样？
 *   光标在硬件里被 entity_drawer.sv 当作"画一个黄色矩形边框"处理，
 *   不需要像素级 x，所以省下来的位可以留给以后扩展（比如光标颜色、动画相位）。
 *
 * 注意参数顺序是 (visible, row, col)，但位布局里 col 在高位 row 在低位 ——
 * 别被骗了，是因为 hw/pvz_top.sv 里读 writedata[4:2] 是 col、[1:0] 是 row。
 */
static inline unsigned int pvz_pack_cursor(int visible, int row, int col)
{
    return ((unsigned)(visible & 1) << 31) |
           ((unsigned)(col     & 7) << 2)  |
           ((unsigned)(row     & 3));
}

/* ioctl argument: write `value` to register `word_index` */
/*
 * pvz_write_arg_t —— 用户态 -> 内核态 ioctl 调用时传的参数包。
 *
 * 用户态填好这个 struct，把它的指针作为 ioctl 第三个参数：
 *   pvz_write_arg_t w = { .word_index = PVZ_REG_CURSOR, .value = packed };
 *   ioctl(fd, PVZ_WRITE_REG, &w);
 * 内核里的 pvz_ioctl 会用 copy_from_user 把这个 struct 复制到内核空间，
 * 然后用 word_index 和 value 做边界检查 + iowrite32。
 *
 * 为什么用 struct 而不是两个独立的 ioctl 命令？
 *   因为 ioctl 的"data"只允许一个指针参数。塞两个 unsigned int 进一个
 *   struct 是最简洁的方式。同时这样 ioctl 命令数量保持为 1，未来要扩展
 *   再加新命令也好对照。
 */
typedef struct {
    unsigned int word_index;   /* 0..PVZ_NUM_REGS-1 */
                               /* 寄存器索引，driver 会做 < PVZ_NUM_REGS 的边界检查 */
    unsigned int value;        /* 要写进去的 32 位值（一般来自上面那两个 pack 函数） */
} pvz_write_arg_t;

/*
 * ioctl 编号定义。
 *
 * Linux ioctl 编号是用 _IO/_IOR/_IOW/_IOWR 这些宏拼出来的，
 * 这样内核能从命令号本身解出"方向、大小、模块魔数、命令序号"，
 * 在不同设备之间避免误用。
 *
 *   PVZ_MAGIC 'p'           —— 给我们这个驱动起的"魔数字符"。
 *                              理论上应该跑去 Documentation/userspace-api/ioctl/ioctl-number.rst
 *                              注册一下，但作为课程项目就直接选了一个不容易冲突的字符 'p'。
 *
 *   _IOW(magic, nr, type)   —— 表示"用户态 -> 内核态 (Write 方向)"，
 *                              命令序号 nr=1，参数类型 type 用于 sizeof()
 *                              检查（防止用户传错大小的 struct）。
 *
 * 用户态调用时直接：ioctl(fd, PVZ_WRITE_REG, &arg);
 * 内核侧 pvz_ioctl 的 switch (cmd) 对 PVZ_WRITE_REG 这个 case 做匹配。
 */
#define PVZ_MAGIC 'p'
#define PVZ_WRITE_REG  _IOW(PVZ_MAGIC, 1, pvz_write_arg_t)

#endif /* _PVZ_H */
