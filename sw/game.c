/*
 * PvZ game logic
 *
 * Manages the 4x8 grid, zombie spawning/movement, peashooter firing,
 * projectile movement, collision detection, sun economy, and
 * win/lose conditions.
 *
 * ===================================================================
 * 中文总览 —— game.c 是"纯软件世界模型"
 * ===================================================================
 *
 * 这个文件里完全没有硬件相关的代码 —— 不开 /dev/pvz，不调 ioctl，
 * 不读键盘。它纯粹维护 game_state_t 这个 struct，让外面的人（main.c
 * 跑游戏循环、render.c 翻译成寄存器、test_game.c 跑单元测试）随便用。
 * 这种"逻辑跟 IO 分离"的好处是 test_game.c 可以在任何 Linux 机器上跑
 * （不需要 FPGA），直接验证规则正确性。
 *
 * 文件结构：
 *   1. random_range          —— 小工具，rand() 的 [min, max] 包装。
 *   2. game_init             —— 把 gs 清零并设默认。
 *   3. game_place_plant /
 *      game_remove_plant      —— 玩家输入触发的两个动作。
 *   4. 一堆 static helpers   —— zombie_in_row、zombie_col、spawn_pea。
 *   5. update_*              —— 7 个每帧子阶段（spawn / fire / projectile /
 *                                zombie / collision / sun / win check）。
 *   6. game_update            —— 顺序串起所有 update_*。
 *
 * 阶段顺序很重要（在 game_update 里）：
 *   sun -> spawn -> firing -> projectiles -> zombies -> collisions -> win
 *   - firing 在 projectiles 之前：本帧射出的豌豆本帧就能往前飞 2 像素。
 *   - projectiles 在 zombies 之前：让豌豆先飞，再让僵尸移动，避免同一帧
 *     里豌豆"跳过"僵尸（如果僵尸先移动到豌豆位置碰撞会算的；但反过来
 *     如果豌豆先飞过僵尸再算撞，会漏判 —— 所以 collisions 是最后一步收尾）。
 *   - check_win 放最后：保证所有死亡判定已经发生。
 */

#include <stdlib.h>
#include <string.h>
#include "game.h"

/* Simple pseudo-random using the C library rand() */
/*
 * random_range —— 返回 [min, max] 闭区间的随机整数。
 * 高层：游戏里所有"几秒后下一只僵尸"和"僵尸出生在第几行"都靠这个。
 * 低层：rand() 来自 C 标准库，返回 [0, RAND_MAX] 的值。
 *      rand() % (max - min + 1) 把它映射到 [0, range)，再 +min 平移。
 *      注意 rand() 不是密码学安全的，但游戏里够用。
 *      种子由 main.c 在启动时 srand(time(NULL)) 设置一次；test_game.c 则
 *      用 srand(42) 来获得确定性的僵尸出生序列，方便复现单元测试。
 */
static int random_range(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

/*
 * game_init —— 把世界状态重置为开局默认。
 * 高层：main.c 启动时调一次。test_game 在每个 case 开头也调一次得到干净状态。
 * 低层步骤：
 *   1. memset 全 0 一次把所有字段清零 —— 包括 grid 里所有 plant_t.type =
 *      PLANT_NONE (0)、所有 zombie/pea 的 active = 0。这是为什么 PLANT_NONE
 *      必须等于 0 的原因（见 game.h 里的注释）。
 *   2. 阳光初始 INITIAL_SUN=100，sun_timer 设成 SUN_INTERVAL 表示"再等 8 秒
 *      自动加阳光"（也可以设 0 让开局立刻加一波，但当前选择是开局先等）。
 *   3. state=STATE_PLAYING 让 game_update 开始跑逻辑（不然 update 第一行
 *      会 return）。
 *   4. 光标默认 (0,0)，选中豌豆射手 —— 玩家第一次按 SPACE 默认种豌豆。
 *   5. spawn_timer 用 random_range 随机给一个 [MIN, MAX] 的初值，让第一只
 *      僵尸出生时间也是随机的（不然每局都一样不好玩）。
 */
void game_init(game_state_t *gs)
{
    memset(gs, 0, sizeof(*gs));

    gs->sun = INITIAL_SUN;
    gs->sun_timer = SUN_INTERVAL;
    gs->state = STATE_PLAYING;
    gs->cursor_row = 0;
    gs->cursor_col = 0;
    gs->selected_plant_type = PLANT_PEASHOOTER;
    gs->zombies_spawned = 0;
    gs->spawn_timer = random_range(ZOMBIE_SPAWN_MIN, ZOMBIE_SPAWN_MAX);
    gs->frame_count = 0;
}

/*
 * game_place_plant —— 在光标位置种一棵植物。
 * 高层：被 main.c 在玩家按 SPACE 时调用。返回 1 表示种成功，0 表示拒绝。
 * 低层流程：
 *   1. 读 cursor_row/col 算出目标格子 (r, c)。注意光标是格子坐标，
 *      不是像素坐标，所以直接当 grid 下标用。
 *   2. 根据 selected_plant_type 选 cost：向日葵 SUNFLOWER_COST，
 *      其它（豌豆射手）走 PLANT_COST。三元表达式让以后加新植物方便扩展。
 *   3. 检查 1：格子已经有植物则失败 —— 不允许覆盖种植（PvZ 原版也是这样）。
 *   4. 检查 2：阳光不够则失败。
 *   5. 通过检查后写入植物：type 置成选中类型，fire_cooldown 给 120 让它
 *      2 秒后才能首射（防止玩家用"种掉重种"来加速攻击），hp 给满 PLANT_HP=3。
 *   6. 阳光扣 cost。
 */
int game_place_plant(game_state_t *gs)
{
    int r = gs->cursor_row;
    int c = gs->cursor_col;
    int type = gs->selected_plant_type;
    int cost = (type == PLANT_SUNFLOWER) ? SUNFLOWER_COST : PLANT_COST;

    if (gs->grid[r][c].type != PLANT_NONE)
        return 0;
    if (gs->sun < cost)
        return 0;

    gs->grid[r][c].type = type;
    gs->grid[r][c].fire_cooldown = PLANT_FIRE_COOLDOWN;
    gs->grid[r][c].hp = PLANT_HP;
    gs->sun -= cost;
    return 1;
}

/*
 * game_remove_plant —— 把光标位置的植物清掉。
 * 高层：被 main.c 在玩家按 D（铲子）时调用。不退阳光（和 PvZ 原版一致）。
 * 低层：把 type 改回 PLANT_NONE，fire_cooldown 清 0。注意 hp 字段没显式清零，
 *      但下次种植物时会重新写满，所以无所谓。
 */
int game_remove_plant(game_state_t *gs)
{
    int r = gs->cursor_row;
    int c = gs->cursor_col;

    if (gs->grid[r][c].type == PLANT_NONE)
        return 0;

    gs->grid[r][c].type = PLANT_NONE;
    gs->grid[r][c].fire_cooldown = 0;
    return 1;
}

/* Check if any active zombie is in the given row */
/*
 * zombie_in_row —— 这一行上是否还有活僵尸。
 * 高层：豌豆射手只在"自己那行有僵尸"时才扣冷却开火，避免空射浪费豌豆槽位。
 * 低层：扫一遍 8 个槽位，命中就返回 1。这里不关心僵尸 x 在哪 —— 只要这一行
 *      还有就算"敌人在视野里"。注意这是 O(MAX_ZOMBIES) 的线性扫描，
 *      每帧每植物都扫一次（最坏 32 * 8 = 256 次比较），对 60fps 完全没压力。
 */
static int zombie_in_row(game_state_t *gs, int row)
{
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (gs->zombies[i].active && gs->zombies[i].row == row)
            return 1;
    }
    return 0;
}

/* Convert a zombie's screen x to a grid column.
 * Returns -1 if the zombie is not over the lawn. */
/*
 * zombie_col —— 把僵尸的屏幕 x 像素坐标换算成格子列。
 * 高层：判断"僵尸现在站在哪一列上面"，用来检查它脚下有没有植物可以啃。
 * 低层：
 *   - 先减去 GAME_AREA_X (=64) 得到相对网格的偏移 gx。
 *   - 如果 gx<0 表示已经走过网格左边缘（这种情况其实应该已经触发 LOSE，
 *     但保险起见返回 -1）；gx>=8*64=512 表示在网格右边外（刚出生时）。
 *   - 否则除以 CELL_SIZE=64 得到列号 0..7。
 * 注意：用的是 x_pixel 这一个点（僵尸贴图的左边缘），而不是中心或右边缘。
 *      意味着僵尸的左边一进入下一格就算"踩到"了那一格的植物。
 */
static int zombie_col(int x_pixel)
{
    int gx = x_pixel - GAME_AREA_X;
    if (gx < 0 || gx >= GRID_COLS * CELL_SIZE)
        return -1;
    return gx / CELL_SIZE;
}

/* Spawn a new pea projectile at the given grid cell */
/*
 * spawn_pea —— 从植物所在格子的右边缘发射一颗豌豆。
 * 高层：被 update_firing 在某个豌豆射手冷却到 0 时调用。
 * 低层：
 *   1. 线性扫 MAX_PROJECTILES=16 个槽位找一个 active=0 的空位。
 *   2. 给它填 row=植物行，x_pixel = GAME_AREA_X + (col+1)*CELL_SIZE
 *      —— 也就是植物格子的右边缘像素。这样豌豆"从植物嘴里飞出来"。
 *   3. 如果 16 个槽位全占满，本帧就把这颗豌豆悄悄丢掉。实战中几乎不会
 *      发生（豌豆飞过屏幕只要 (640-x)/2 帧，约 5 秒，槽位回收很快）。
 */
static void spawn_pea(game_state_t *gs, int row, int col)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!gs->projectiles[i].active) {
            gs->projectiles[i].active = 1;
            gs->projectiles[i].row = row;
            /* Start at the right edge of the plant's cell */
            gs->projectiles[i].x_pixel =
                GAME_AREA_X + (col + 1) * CELL_SIZE;
            return;
        }
    }
    /* No free slot; pea is lost */
}

/* Update zombie positions, eating, and check for lose condition */
/*
 * update_zombies —— 僵尸的"行走 + 吃植物"状态机。每帧每个僵尸跑一次。
 *
 * 高层：僵尸只有两种状态 —— 走路 (eating=0) 或 啃植物 (eating=1)。
 *      啃的时候不走；走的时候每挪 1 像素检查脚下有没有植物，有就切到啃。
 *      被啃的植物 hp 归 0 时切回走路。
 *
 * 低层流程（每个 active 僵尸）：
 *   分支 A：eating == 1
 *     - 先重新检查"这一列还有植物吗" —— 因为可能两只僵尸同时啃同一棵，
 *       另一只刚好这一帧把它啃死了，那本只就该立刻恢复走路（不要再 -1 eat_timer
 *       否则下一帧又会咬一口"虚空植物"）。
 *     - 如果植物还在：eat_timer-- → 到 0 时咬一口 plant->hp--。如果咬死了，
 *       把格子彻底清空 (type=NONE、fire_cooldown=0、hp=0) 并 eating=0；
 *       没咬死就重置 eat_timer = 60 帧（下一口 1 秒后）。
 *     - 啃的时候 continue 跳过下面的走路逻辑（保持原地）。
 *
 *   分支 B：eating == 0（走路）
 *     - move_counter++，到 ZOMBIE_SPEED_FRAMES=3 时才 x_pixel-- 一次。
 *       这样实际速度 = 60fps/3 = 20 像素/秒。
 *     - 移动后立刻检查 LOSE：x_pixel <= GAME_AREA_X=64 即"走到了草坪左边缘
 *       (玩家家门口)"，整局立即输。注意这里 return 是给 STATE_LOSE 留干净
 *       的状态，不再处理后面的僵尸。
 *     - 检查"脚下是否有植物"：zombie_col 算出当前格子，如果有植物就切到
 *       eating 状态，eat_timer 给一个完整 ZOMBIE_EAT_COOLDOWN 让玩家有反应时间。
 *
 * 一个微妙点：eating 切回 0 后通过"fall through"接到下面 movement 块，
 * 但 movement 还要等 move_counter 累到 3 才挪一格。也就是说植物被啃死的
 * 那一帧僵尸还不会立刻往前挪，要等几帧 —— 这是 PvZ 本来的"啃完一棵停一下"
 * 的视觉感受，挺自然。
 */
static void update_zombies(game_state_t *gs)
{
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        zombie_t *z = &gs->zombies[i];
        if (!z->active)
            continue;

        if (z->eating) {
            /* Re-check that the plant still exists (another zombie may
             * have destroyed it) */
            /* 中文：重要的"防并发"检查 —— 另一只僵尸刚把这棵植物啃死，
             *      本只就别再啃了，落到下面的 movement 分支去走路。 */
            int col = zombie_col(z->x_pixel);
            if (col < 0 || gs->grid[z->row][col].type == PLANT_NONE) {
                z->eating = 0;
                z->eat_timer = 0;
                /* Fall through to movement below */
                /* 中文：注意这里不 continue，故意 fall through 让本帧也能
                 *      尝试推进 move_counter（虽然要累到 3 才真挪一格）。 */
            } else {
                /* Continue eating: deal damage on timer */
                /* 中文：植物还在，每帧 eat_timer-- 到 0 咬一口。 */
                z->eat_timer--;
                if (z->eat_timer <= 0) {
                    gs->grid[z->row][col].hp--;
                    if (gs->grid[z->row][col].hp <= 0) {
                        /* 中文：植物 hp 归 0：彻底清空格子，僵尸状态切回走路。 */
                        gs->grid[z->row][col].type = PLANT_NONE;
                        gs->grid[z->row][col].fire_cooldown = 0;
                        gs->grid[z->row][col].hp = 0;
                        z->eating = 0;
                    } else {
                        /* 中文：还没死，下一口 60 帧（1 秒）后。 */
                        z->eat_timer = ZOMBIE_EAT_COOLDOWN;
                    }
                }
                continue; /* Don't move while eating */
                /* 中文：啃的时候原地不动，跳过下面的移动块。 */
            }
        }

        /* Movement */
        /* 中文：到这里说明僵尸不在啃 —— 推进 move_counter，到 3 帧才挪 1 像素。 */
        z->move_counter++;
        if (z->move_counter >= ZOMBIE_SPEED_FRAMES) {
            z->move_counter = 0;
            z->x_pixel--;

            /* Lose condition: zombie reached the lawn's left edge */
            /* 中文：x 走到草坪最左 (GAME_AREA_X=64) 就算僵尸进家门，整局输掉。 */
            if (z->x_pixel <= GAME_AREA_X) {
                gs->state = STATE_LOSE;
                return;
            }

            /* Check for plant collision after moving */
            /* 中文：刚挪完看脚下这一格有没有植物，有就切到啃的状态。 */
            int col = zombie_col(z->x_pixel);
            if (col >= 0 && gs->grid[z->row][col].type != PLANT_NONE) {
                z->eating = 1;
                z->eat_timer = ZOMBIE_EAT_COOLDOWN;
            }
        }
    }
}

/* Update projectile positions */
/*
 * update_projectiles —— 把所有豌豆往右挪 PEA_SPEED=2 像素，飞出屏幕就回收。
 * 高层：纯粹的"位置更新"，碰撞检测放在另一个独立函数 check_collisions 里。
 *      分开写有两个好处：1) 顺序清晰（移动 -> 碰撞）；2) 测试时能单独验证。
 * 低层：x_pixel > SCREEN_W (=640) 即代表豌豆完全飞出屏幕右边缘，
 *      置 active=0 让槽位能被下一颗豌豆复用。
 */
static void update_projectiles(game_state_t *gs)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectile_t *p = &gs->projectiles[i];
        if (!p->active)
            continue;

        p->x_pixel += PEA_SPEED;

        /* Remove if off-screen */
        if (p->x_pixel > SCREEN_W)
            p->active = 0;
    }
}

/* Fire peas from peashooters that have zombies in their row */
/*
 * update_firing —— 每帧扫一遍所有豌豆射手，处理冷却 + 发射逻辑。
 * 高层："这一行有敌人才射"是 PvZ 的经典节能机制 —— 没敌人时豌豆射手冷却
 *      保持在 0，敌人一进入这一行立刻能开火 (打第一颗的反应时间 = 0)。
 * 低层流程（双层循环遍历 4x8 格子）：
 *   1. 跳过非豌豆射手的格子（向日葵也跳）。
 *   2. fire_cooldown > 0 就 -1（即便没敌人也减，确保 spawn 时的初始 120
 *      会一直减到 0，但下面的 if 又要求"等于 0 才射"，所以效果是冷却到 0
 *      就一直等待敌人）。
 *   3. 冷却 == 0 且本行有僵尸（zombie_in_row 检查）→ spawn_pea 发射，
 *      冷却重置为 PLANT_FIRE_COOLDOWN=120 (2 秒)。
 * 注意 update_firing 在 update_projectiles 之前调用，意思是新发射的豌豆
 * 本帧也会 +2 像素 —— 视觉上"豌豆瞬间从植物嘴里飞出 2 像素"，符合直觉。
 */
static void update_firing(game_state_t *gs)
{
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            plant_t *p = &gs->grid[r][c];
            if (p->type != PLANT_PEASHOOTER)
                continue;

            if (p->fire_cooldown > 0)
                p->fire_cooldown--;

            if (p->fire_cooldown == 0 && zombie_in_row(gs, r)) {
                spawn_pea(gs, r, c);
                p->fire_cooldown = PLANT_FIRE_COOLDOWN;
            }
        }
    }
}

/* Check pea-zombie collisions */
/*
 * check_collisions —— 对每一颗豌豆扫每一只僵尸，判断 AABB 包围盒是否重叠。
 *
 * 高层：豌豆只能撞到同行的僵尸；命中后扣血并销毁豌豆，僵尸 hp<=0 就死。
 * 低层（AABB 1D 碰撞，因为同行所以 y 必然重叠）：
 *   - 僵尸的水平区间：[z_left, z_right] = [x_pixel, x_pixel + ZOMBIE_WIDTH=32]
 *   - 豌豆的水平区间：[p_left, p_right] = [x_pixel, x_pixel + PEA_SIZE=8]
 *   - 经典区间重叠判定：a_right >= b_left && a_left <= b_right。
 *   - 命中：z->hp -= 1，p->active = 0 销毁豌豆；hp 归 0 时 z->active = 0
 *     回收僵尸槽位（z->row/x_pixel 等字段不用清，下次复用时会被 spawn 覆盖）。
 *   - break 出内层：一颗豌豆撞到一个僵尸就停，不会"穿过"撞第二个 (单体伤害)。
 *
 * 复杂度：MAX_PROJECTILES * MAX_ZOMBIES = 16 * 8 = 128 次比较，60fps 下完全可控。
 */
static void check_collisions(game_state_t *gs)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectile_t *p = &gs->projectiles[i];
        if (!p->active)
            continue;

        for (int j = 0; j < MAX_ZOMBIES; j++) {
            zombie_t *z = &gs->zombies[j];
            if (!z->active || z->row != p->row)
                continue;

            /* Collision: pea overlaps zombie bounding box */
            /* 中文：1D AABB 重叠判定。同行 -> y 必然重叠，只看 x。 */
            int z_left = z->x_pixel;
            int z_right = z->x_pixel + ZOMBIE_WIDTH;
            int p_left = p->x_pixel;
            int p_right = p->x_pixel + PEA_SIZE;

            if (p_right >= z_left && p_left <= z_right) {
                /* Hit! */
                /* 中文：命中。扣僵尸血，销毁豌豆。 */
                z->hp -= PEA_DAMAGE;
                p->active = 0;

                if (z->hp <= 0)
                    z->active = 0;

                break; /* Each pea hits only one zombie */
                /* 中文：一颗豌豆只能命中一只，break 跳出僵尸循环。 */
            }
        }
    }
}

/* Spawn zombies on a timer */
/*
 * update_spawning —— 按 spawn_timer 间隔出僵尸。
 * 高层：本关一共出 TOTAL_ZOMBIES=5 个，全部出完就停（让玩家把剩下的打掉
 *      之后能赢）。每两个之间间隔在 [ZOMBIE_SPAWN_MIN, MAX] (= 8~15 秒) 随机。
 * 低层流程：
 *   1. 已经出够 5 个 → 直接 return，进入"清理阶段"等玩家打完。
 *   2. spawn_timer 每帧 -1，到 0 触发出生。
 *   3. 找第一个 active=0 的槽位填进去：行随机 0..3，x_pixel=SCREEN_W-1=639
 *      (从最右边走进来)，hp 满，move_counter / eat_timer / eating 都清 0。
 *   4. zombies_spawned++（累计计数，决定何时停止 spawn）。
 *   5. 重置 spawn_timer 为下一个随机间隔。
 * 注意：理论上同时活 8 个的话 (槽位满了)，本次 spawn 会跳过、但 timer
 *      还是会重置 —— 也就是这只僵尸"就这么没出生"。但 TOTAL_ZOMBIES=5 < 8
 *      所以现实里碰不到这种情况。
 */
static void update_spawning(game_state_t *gs)
{
    if (gs->zombies_spawned >= TOTAL_ZOMBIES)
        return;

    gs->spawn_timer--;
    if (gs->spawn_timer <= 0) {
        /* Find a free zombie slot */
        /* 中文：线性找一个 active=0 的槽位。 */
        for (int i = 0; i < MAX_ZOMBIES; i++) {
            if (!gs->zombies[i].active) {
                gs->zombies[i].active = 1;
                gs->zombies[i].row = random_range(0, GRID_ROWS - 1);
                gs->zombies[i].x_pixel = SCREEN_W - 1;
                gs->zombies[i].hp = ZOMBIE_HP;
                gs->zombies[i].move_counter = 0;
                gs->zombies[i].eating = 0;
                gs->zombies[i].eat_timer = 0;
                gs->zombies_spawned++;
                break;
            }
        }
        gs->spawn_timer = random_range(ZOMBIE_SPAWN_MIN, ZOMBIE_SPAWN_MAX);
    }
}

/* Count sunflowers currently on the field */
/*
 * count_sunflowers —— 数一下场上有几棵向日葵。
 * 高层：update_sun 用它做"每棵向日葵 +1 倍的阳光产出"加成。
 * 低层：扫一遍 4x8 网格累加。O(32)，每 8 秒才调一次，开销几乎为 0。
 */
static int count_sunflowers(const game_state_t *gs)
{
    int n = 0;
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            if (gs->grid[r][c].type == PLANT_SUNFLOWER)
                n++;
    return n;
}

/* Update sun economy.  Base rate plus one extra increment per sunflower. */
/*
 * update_sun —— 阳光经济。每 SUN_INTERVAL=480 帧 (8 秒) 触发一次自动产阳光。
 * 高层：基础产出 25 + 每棵向日葵 +25。这样设计是为了让"种向日葵"这个决定
 *      有真实的经济回报，鼓励玩家花前几口阳光铺向日葵。
 * 低层：sun_timer 倒计时，到 0 时 sun += 25*(1+N)，timer 重置为 480。
 *      注意公式是 25*(1+N) 而不是 25*N，意思是即便场上一棵向日葵都没有
 *      也会有 25 的基础"日照"产出 —— 玩家不会卡死在 0 阳光。
 */
static void update_sun(game_state_t *gs)
{
    gs->sun_timer--;
    if (gs->sun_timer <= 0) {
        gs->sun += SUN_INCREMENT * (1 + count_sunflowers(gs));
        gs->sun_timer = SUN_INTERVAL;
    }
}

/* Check win condition */
/*
 * check_win —— 判赢。
 * 高层：胜利条件 = "所有 TOTAL_ZOMBIES=5 个僵尸都出生过了" 而且 "现在场上
 *      没有活的僵尸"。两个条件同时满足才胜利，缺一不可：
 *      - 只有第一条满足、还有活的：继续打。
 *      - 只有第二条满足、还没出完：开局阶段就符合"没有活僵尸"但显然不能赢。
 * 低层：先看 spawn 是否完成，没完成直接 return；然后扫槽位，找到任一 active
 *      的就 return。两道关都过了就 state = STATE_WIN。
 */
static void check_win(game_state_t *gs)
{
    if (gs->zombies_spawned < TOTAL_ZOMBIES)
        return;

    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (gs->zombies[i].active)
            return;
    }

    gs->state = STATE_WIN;
}

/*
 * game_update —— 每帧入口。main.c 在 60fps 主循环里调用一次。
 *
 * 高层：把 7 个子阶段按特定顺序串起来。每个子阶段只关心一类实体或一类
 *      系统，互相之间通过 game_state_t 通信。这种设计便于单元测试 ——
 *      可以构造 game_state_t 直接调任一 update_* 验证一种规则。
 *
 * 阶段顺序（顺序本身有含义，乱了就出 bug）：
 *   0. 状态闸：如果不是 PLAYING（已经赢/输/退出）直接 return，防止冻结画面
 *      里 timer 还在跑。
 *   1. frame_count++：累计帧数（main.c 用它做 60 帧打印一次状态）。
 *   2. update_sun         —— 阳光经济。先于一切，确保即使输了的那一帧也
 *                            算完阳光（虽然这种边界没什么影响）。
 *   3. update_spawning    —— 生新僵尸。在 update_zombies 之前调，新僵尸
 *                            本帧的 move_counter=0 不会移动，但已经存在
 *                            于槽位里供后面统计。
 *   4. update_firing      —— 豌豆射手发射。在 update_projectiles 之前，
 *                            让新豌豆本帧也能往前飞 +2。
 *   5. update_projectiles —— 豌豆移动。
 *   6. update_zombies     —— 僵尸移动 / 啃植物 / 判 LOSE。
 *   7. check_collisions   —— 豌豆和僵尸都已就位，统一判碰撞最干净。
 *   8. check_win          —— 必须放最后：要等本帧所有 z->active=0 的死亡
 *                            都发生过才知道是不是全清。
 */
void game_update(game_state_t *gs)
{
    if (gs->state != STATE_PLAYING)
        return;

    gs->frame_count++;

    update_sun(gs);
    update_spawning(gs);
    update_firing(gs);
    update_projectiles(gs);
    update_zombies(gs);
    check_collisions(gs);
    check_win(gs);
}
