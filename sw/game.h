#ifndef _GAME_H
#define _GAME_H

/*
 * ===================================================================
 * 中文总览 —— game.h 是"世界模型"的对外接口
 * ===================================================================
 *
 * 这个头文件做三件事：
 *   1. 把硬件常量（PVZ_GRID_ROWS 等，来自 pvz.h）起别名成游戏侧用的名字
 *      （GRID_ROWS 等）。这样 game.c 写起来更短，但同时确保和 FPGA 的
 *      bg_grid.sv / entity_drawer.sv 是"同一份真理"。一旦硬件改了格子大小，
 *      只需要改 pvz.h，game.h 自动跟着变。
 *   2. 列出所有"游戏数值参数"。这些数字大多是按 60 fps 推出来的，
 *      比如 PLANT_FIRE_COOLDOWN=120 = 2 秒射一次。
 *   3. 定义实体的 struct（plant_t / zombie_t / projectile_t）和总体
 *      game_state_t；以及 4 个对外函数 game_init / game_update /
 *      game_place_plant / game_remove_plant。
 *
 * 上下游关系：
 *   - main.c 每帧调用 game_update(&gs)，然后把 &gs 喂给 render.c。
 *   - render.c 把 game_state_t 翻译成 51 个 32 位寄存器字，
 *     通过 ioctl 推到 FPGA 的 pvz_top.sv。
 *   - input.c 间接通过 main.c 的 process_input 改 gs->cursor_row/col
 *     和 gs->selected_plant_type。
 */

#include "pvz.h"

/* Grid dimensions (must match hw/entity_drawer.sv) */
/*
 * 网格尺寸 —— 这一组别名让 game.c 可以写 GRID_ROWS 而不是 PVZ_GRID_ROWS。
 * 高层：所有"格子有多少、每格多大、左上角在哪"的常量都来自 pvz.h（硬件那边
 *      也用同一份）。"single source of truth" —— 不要在这里硬编码 4 或 8，
 *      因为 hw/entity_drawer.sv 是按 pvz.h 的值画的。
 * 低层：GRID_ROWS=4, GRID_COLS=8, CELL_SIZE=64。
 *      格子 (r, c) 覆盖屏幕像素 x ∈ [64+c*64, 64+(c+1)*64),
 *                              y ∈ [112+r*64, 112+(r+1)*64)。
 *      GAME_AREA_X=64 是左上角 x，GAME_AREA_Y=112 是左上角 y（顶部空出
 *      112 像素放 HUD：阳光数 + 植物图标）。
 */
#define GRID_ROWS     PVZ_GRID_ROWS
#define GRID_COLS     PVZ_GRID_COLS
#define CELL_SIZE     PVZ_CELL_SIZE
#define GAME_AREA_X   PVZ_GRID_X
#define GAME_AREA_Y   PVZ_GRID_Y

/* Screen dimensions */
/*
 * 屏幕尺寸 640x480，VGA@60Hz 的标准分辨率。
 * SCREEN_W 主要用在两个地方：
 *   1. 僵尸出生时 x_pixel = SCREEN_W - 1（从最右边缘走进来）
 *   2. 豌豆飞出屏幕右边时（p->x_pixel > SCREEN_W）回收
 */
#define SCREEN_W      PVZ_SCREEN_W
#define SCREEN_H      PVZ_SCREEN_H

/* Plant constants */
/*
 * 植物相关常量。
 * 高层：定义"种一棵植物花多少阳光、多久打一次豌豆、能挨几口"。
 * 低层：
 *   PLANT_COST=50         —— 种豌豆射手要 50 阳光，刚好等于初始阳光 INITIAL_SUN/2，
 *                            也就是开局可以种 2 棵。
 *   SUNFLOWER_COST=50     —— 向日葵也是 50，方便玩家选。
 *   PLANT_FIRE_COOLDOWN=120 —— 帧数。120/60fps = 每 2 秒射一颗豌豆。
 *                            game.c 的 update_firing 每帧把这个值减 1，到 0 就发射。
 *   PLANT_HP=3            —— 一棵植物能被僵尸咬 3 次，每次 ZOMBIE_EAT_COOLDOWN=60 帧
 *                            （1 秒）一口，所以从被咬到死大概 3 秒。
 */
#define PLANT_COST          50
#define SUNFLOWER_COST      50
#define PLANT_FIRE_COOLDOWN 120  /* frames (2 seconds at 60fps) */
#define PLANT_HP            3    /* hits before a plant is destroyed */

/* Zombie constants (sprite size matches hardware: 32x64) */
/*
 * 僵尸相关常量。
 * 高层：决定僵尸"几个、多血、多快、几秒来一个、咬一口多久"。
 * 低层：
 *   MAX_ZOMBIES=PVZ_MAX_ZOMBIES=8 —— 同屏最多 8 个槽位。硬件只有 8 个寄存器
 *                                     (word 32..39)，所以这个上限是硬件强制的。
 *   ZOMBIE_HP=3              —— 一个僵尸吃 3 颗豌豆死（PEA_DAMAGE=1，所以 3 颗）。
 *   ZOMBIE_SPEED_FRAMES=3    —— 每 3 帧才往左挪 1 像素。60fps/3 = 20 px/s。
 *                                走完整个屏幕 640 px 大约要 32 秒。
 *   ZOMBIE_WIDTH=32          —— 僵尸贴图宽度（硬件里 entity_drawer.sv 画的是 32x64 矩形）。
 *                                碰撞箱用 [x_pixel, x_pixel+32)。
 *   ZOMBIE_HEIGHT=64         —— 一格高，刚好填满一行的 64 像素。
 *   TOTAL_ZOMBIES=5          —— 这一关总共会出 5 个僵尸，杀完就赢。
 *   ZOMBIE_SPAWN_MIN=8*60    —— 两个僵尸之间至少隔 8 秒（480 帧）。
 *   ZOMBIE_SPAWN_MAX=15*60   —— 最多 15 秒，random_range 在 [MIN, MAX] 之间均匀取。
 *   ZOMBIE_EAT_COOLDOWN=60   —— 每 1 秒咬植物一口，配合 PLANT_HP=3 → 3 秒咬死一棵。
 */
#define MAX_ZOMBIES          PVZ_MAX_ZOMBIES
#define ZOMBIE_HP            3
#define ZOMBIE_SPEED_FRAMES  3   /* move 1 pixel every N frames (~20 px/s) */
#define ZOMBIE_WIDTH         32
#define ZOMBIE_HEIGHT        64
#define TOTAL_ZOMBIES        5
#define ZOMBIE_SPAWN_MIN    (8 * 60)  /* 8 seconds in frames */
#define ZOMBIE_SPAWN_MAX    (15 * 60) /* 15 seconds in frames */
#define ZOMBIE_EAT_COOLDOWN 60       /* frames between bites */

/* Projectile constants */
/*
 * 豌豆子弹相关常量。
 * 高层：豌豆是从豌豆射手向右飞的"子弹"，撞到同行僵尸就掉血。
 * 低层：
 *   MAX_PROJECTILES=16  —— SW 内部最多同时有 16 颗豌豆。注意硬件那边只有 8 个槽位
 *                          (word 40..47)，所以 render.c 在打包时只取前 8 个 active 的，
 *                          多出来的肉眼看不到但碰撞依然计算。
 *   PEA_SPEED=2         —— 每帧右移 2 像素 = 120 px/s，比僵尸快 6 倍。
 *   PEA_DAMAGE=1        —— 一颗扣 1 血。配合 ZOMBIE_HP=3，三颗豌豆秒一个僵尸。
 *   PEA_SIZE=8          —— 碰撞箱宽度，[x_pixel, x_pixel+8) 与僵尸 [zx, zx+32) 重叠就算撞上。
 */
#define MAX_PROJECTILES     16
#define PEA_SPEED           2   /* pixels per frame */
#define PEA_DAMAGE          1
#define PEA_SIZE            8

/* Sun economy */
/*
 * 阳光经济系统。
 * 高层：阳光是种植物的货币。开局给一笔，之后每隔一段时间自动加 + 场上向日葵
 *      会额外加成。这模仿了 PvZ 原版的"白天日照 + 向日葵"机制。
 * 低层：
 *   INITIAL_SUN=100   —— 开局 100，正好够种 2 棵（每棵 50）。
 *   SUN_INCREMENT=25  —— 每次到点自动 +25。
 *                        update_sun 里如果场上有 N 棵向日葵，实际加 25*(1+N)。
 *   SUN_INTERVAL=8*60 —— 帧数。480 帧 / 60fps = 每 8 秒触发一次。
 *                        game.c 的 update_sun 用 sun_timer 倒数到 0 触发。
 */
#define INITIAL_SUN         100
#define SUN_INCREMENT       25
#define SUN_INTERVAL        (8 * 60)  /* 8 seconds in frames */

/* Game states */
/*
 * 游戏状态机的三个状态。
 * STATE_PLAYING=0 —— 正常游戏中，game_update 才会真正跑逻辑。
 * STATE_WIN=1     —— 所有僵尸都被打死后切到这个状态，main.c 打印胜利信息退出。
 * STATE_LOSE=2   —— 任一僵尸走到 x <= GAME_AREA_X（左边缘）触发，主循环也会退出。
 *
 * 另外 main.c 还会把 gs->state 设为 -1 表示 ESC 退出（不是游戏内部状态，
 * 是 main 循环 while (gs.state >= 0) 的退出信号）。
 */
#define STATE_PLAYING  0
#define STATE_WIN      1
#define STATE_LOSE     2

/* Plant types */
/*
 * 植物类型枚举。
 * PLANT_NONE=0 这一步很关键 —— game_init 用 memset(gs, 0, ...) 一次清空整个
 * game_state_t，把所有 plant_t.type 设成 0 也就是 PLANT_NONE，
 * 这样不用写一个循环挨个赋值。如果哪天加新植物，记得 PLANT_NONE 必须保持 0。
 *
 * PLANT_PEASHOOTER=1 —— 豌豆射手，绿色，会向右发射豌豆。
 * PLANT_SUNFLOWER=2  —— 向日葵，橙色，给阳光加成。
 */
#define PLANT_NONE        0
#define PLANT_PEASHOOTER  1
#define PLANT_SUNFLOWER   2

/*
 * plant_t —— 网格里一个格子的内容。
 * 高层：4x8 共 32 个格子，每个格子一个 plant_t。空格子的 type == PLANT_NONE。
 * 字段：
 *   type          —— PLANT_NONE / PLANT_PEASHOOTER / PLANT_SUNFLOWER。
 *                    决定这个格子的渲染贴图和行为（射豌豆或产阳光）。
 *   fire_cooldown —— 倒计时。每帧 -1，到 0 且同行有僵尸时 spawn_pea，
 *                    重置为 PLANT_FIRE_COOLDOWN (120)。向日葵不读这个字段。
 *   hp            —— 剩余血量，被僵尸咬就减 1，到 0 则 type 改回 PLANT_NONE。
 */
typedef struct {
    int type;           /* PLANT_NONE or PLANT_PEASHOOTER */
    int fire_cooldown;  /* frames until next shot */
    int hp;             /* hit points remaining */
} plant_t;

/*
 * zombie_t —— 一个僵尸槽位。
 * 高层：MAX_ZOMBIES=8 个固定槽位，active=0 表示空槽。这种"固定数组+active 标志"
 *      的设计比链表简单，也直接对应硬件的 8 个寄存器。
 * 字段：
 *   active        —— 0 = 槽位空，1 = 上面有活僵尸。死亡时直接置 0 重用槽位。
 *   row           —— 所在行 (0..GRID_ROWS-1)。僵尸只在一行内水平移动。
 *   x_pixel       —— 屏幕 x 坐标，从右边 SCREEN_W-1 出生，每次往左 -1。
 *                    碰撞箱是 [x_pixel, x_pixel+ZOMBIE_WIDTH)。
 *   hp            —— 剩余血量，初始 ZOMBIE_HP=3，每颗豌豆扣 PEA_DAMAGE=1。
 *   move_counter  —— "走路计时器"。每帧 +1，到 ZOMBIE_SPEED_FRAMES=3 时 x_pixel
 *                    才 -1。这样移动速度可以独立于帧率调。
 *   eating        —— 状态机标志：1 = 正在啃植物（暂停走路），0 = 在走路。
 *   eat_timer     —— "啃植物计时器"，独立于 move_counter。eating=1 时每帧 -1，
 *                    到 0 就咬一口 (plant->hp--)，重置为 ZOMBIE_EAT_COOLDOWN=60。
 * 为什么 move_counter 和 eat_timer 要分开？
 *   因为僵尸"走路节奏 (3 帧/像素)" 和 "啃植物节奏 (60 帧/口)" 是两套独立的循环，
 *   用一个 timer 没法同时表达。eating 标志决定当前帧走哪条分支。
 */
typedef struct {
    int active;
    int row;
    int x_pixel;        /* screen pixel x; moves leftward */
    int hp;
    int move_counter;   /* counts frames until next pixel move */
    int eating;         /* 1 if currently eating a plant */
    int eat_timer;      /* frames until next bite */
} zombie_t;

/*
 * projectile_t —— 一颗豌豆子弹。
 * 高层：MAX_PROJECTILES=16 个固定槽位。豌豆从豌豆射手发射，向右匀速飞，
 *      撞同行僵尸或者飞出屏幕就死。
 * 字段：
 *   active   —— 槽位是否在用。
 *   row      —— 飞行所在行，必须和豌豆射手所在行一致（spawn_pea 里设）。
 *   x_pixel  —— 屏幕 x 坐标，每帧 += PEA_SPEED=2。出生于发射植物格子的右边缘。
 * 注：豌豆没有 hp / damage 字段，因为这些是常量；命中后直接 active=0 销毁。
 */
typedef struct {
    int active;
    int row;
    int x_pixel;        /* screen pixel x; moves rightward */
} projectile_t;

/*
 * game_state_t —— 整个游戏世界状态。
 * 高层：main.c 里只有一份 gs，每帧通过 game_update 改它，再传给 render_frame
 *      去画。所有可见 + 逻辑相关的状态都在这里，方便测试时直接 memcmp / dump。
 *
 * 字段分组讲解：
 *   网格 + 实体：
 *     grid[r][c]        —— 4x8 植物数组。空格子 type=PLANT_NONE。
 *     zombies[i]        —— 8 个僵尸槽位。
 *     projectiles[i]    —— 16 个豌豆槽位。
 *
 *   光标 (HUD 高亮框)：
 *     cursor_row/col    —— 玩家用方向键移动的格子坐标。和"格子里有没有植物"
 *                          完全独立 —— 哪怕格子空着，光标也可以指那里。
 *                          render.c 用这个画黄色边框；game_place_plant 用这个
 *                          决定把植物种到哪。
 *
 *   选择栏：
 *     selected_plant_type —— 当前选中要种的植物类型 (PLANT_PEASHOOTER 或
 *                            PLANT_SUNFLOWER)。两个用途：
 *                            1) 决定 game_place_plant 种哪一种；
 *                            2) 渲染时 HUD 上对应图标会画黄色描边。
 *                            main.c 的 TAB 键负责在两种之间切换。
 *
 *   阳光经济：
 *     sun               —— 当前阳光数。开局 INITIAL_SUN=100。
 *     sun_timer         —— 自动加阳光的倒计时。到 0 触发后重置为 SUN_INTERVAL。
 *
 *   僵尸出生控制：
 *     zombies_spawned   —— 累计出生过的僵尸数 (不是当前活着的)。到 TOTAL_ZOMBIES
 *                          就停止 spawn，全死光后判赢。
 *     spawn_timer       —— 下一只僵尸的倒计时。每次 spawn 完用 random_range
 *                          (MIN, MAX) 重置。
 *
 *   元信息：
 *     state             —— STATE_PLAYING / WIN / LOSE，main.c 也会用 -1 表示 ESC 退出。
 *     frame_count       —— 累计帧数，主要用于调试打印和测试。
 */
typedef struct {
    plant_t      grid[GRID_ROWS][GRID_COLS];
    zombie_t     zombies[MAX_ZOMBIES];
    projectile_t projectiles[MAX_PROJECTILES];

    int cursor_row;
    int cursor_col;

    int selected_plant_type;  /* PLANT_PEASHOOTER or PLANT_SUNFLOWER */

    int sun;
    int sun_timer;

    int zombies_spawned;
    int spawn_timer;

    int state;          /* STATE_PLAYING, STATE_WIN, STATE_LOSE */
    int frame_count;
} game_state_t;

/*
 * 对外的 4 个函数。
 *
 *   game_init(gs)
 *     一次性把 game_state_t 清零并设好默认值（阳光 100、光标在 (0,0)、
 *     选中豌豆射手等）。main.c 在游戏开始时调一次。
 *
 *   game_update(gs)
 *     推进 1 帧的世界状态。由 main.c 在 60fps 循环里每帧调用。
 *     内部按固定顺序跑 7 个阶段：sun -> spawn -> firing -> projectiles ->
 *     zombies -> collisions -> win check。这个顺序本身有一些含义，详见 game.c。
 *
 *   game_place_plant(gs)
 *     根据 cursor_row/col 和 selected_plant_type 在当前光标位置种一棵植物。
 *     检查阳光够不够、格子是不是空的；成功返回 1，失败返回 0。
 *     由 main.c 的 SPACE 键触发。
 *
 *   game_remove_plant(gs)
 *     把当前光标位置的植物清掉（不退阳光）。由 main.c 的 D 键触发。
 */
void game_init(game_state_t *gs);
void game_update(game_state_t *gs);
int  game_place_plant(game_state_t *gs);
int  game_remove_plant(game_state_t *gs);

#endif /* _GAME_H */
