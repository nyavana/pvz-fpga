/*
 * Host-compilable unit tests for game logic
 *
 * Tests: collision detection, sun economy, zombie spawning,
 * win/lose conditions.
 *
 * Compile and run on host: gcc -o test_game test_game.c ../game.c && ./test_game
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
    if (cond) { \
        tests_passed++; \
    } else { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        tests_failed++; \
    } \
} while (0)

static void test_init(void)
{
    game_state_t gs;
    game_init(&gs);

    ASSERT(gs.sun == INITIAL_SUN, "initial sun should be 100");
    ASSERT(gs.state == STATE_PLAYING, "initial state should be PLAYING");
    ASSERT(gs.cursor_row == 0, "cursor row should start at 0");
    ASSERT(gs.cursor_col == 0, "cursor col should start at 0");
    ASSERT(gs.zombies_spawned == 0, "no zombies spawned initially");

    /* All grid cells should be empty */
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            ASSERT(gs.grid[r][c].type == PLANT_NONE, "grid cell should be empty");

    printf("  test_init: OK\n");
}

static void test_place_plant(void)
{
    game_state_t gs;
    game_init(&gs);

    /* Place a plant at (0,0) */
    gs.cursor_row = 0;
    gs.cursor_col = 0;
    int result = game_place_plant(&gs);

    ASSERT(result == 1, "plant placement should succeed");
    ASSERT(gs.grid[0][0].type == PLANT_PEASHOOTER, "cell should have peashooter");
    ASSERT(gs.sun == INITIAL_SUN - PLANT_COST, "sun should decrease by 50");

    /* Try to place another plant on same cell */
    gs.sun = 100;
    result = game_place_plant(&gs);
    ASSERT(result == 0, "placing on occupied cell should fail");

    /* Try to place with insufficient sun */
    gs.cursor_col = 1;
    gs.sun = 10;
    result = game_place_plant(&gs);
    ASSERT(result == 0, "placing with insufficient sun should fail");

    printf("  test_place_plant: OK\n");
}

static void test_remove_plant(void)
{
    game_state_t gs;
    game_init(&gs);

    /* Place and then remove */
    gs.cursor_row = 1;
    gs.cursor_col = 2;
    game_place_plant(&gs);
    int result = game_remove_plant(&gs);

    ASSERT(result == 1, "removal should succeed");
    ASSERT(gs.grid[1][2].type == PLANT_NONE, "cell should be empty after removal");

    /* Remove from empty cell */
    result = game_remove_plant(&gs);
    ASSERT(result == 0, "removing from empty cell should fail");

    printf("  test_remove_plant: OK\n");
}

static void test_sun_economy(void)
{
    game_state_t gs;
    game_init(&gs);

    int initial_sun = gs.sun;
    /* Run frames until sun increments */
    for (int i = 0; i < SUN_INTERVAL + 1; i++)
        game_update(&gs);

    ASSERT(gs.sun == initial_sun + SUN_INCREMENT,
           "sun should increase by 25 after 8 seconds");

    printf("  test_sun_economy: OK\n");
}

static void test_collision(void)
{
    game_state_t gs;
    game_init(&gs);

    /* Manually set up a zombie and a pea on the same row */
    gs.zombies[0].active = 1;
    gs.zombies[0].row = 0;
    gs.zombies[0].x_pixel = 300;
    gs.zombies[0].hp = 3;
    gs.zombies[0].move_counter = 0;

    gs.projectiles[0].active = 1;
    gs.projectiles[0].row = 0;
    gs.projectiles[0].x_pixel = 300; /* Overlapping */

    gs.zombies_spawned = 1; /* Prevent new spawns from interfering */
    gs.spawn_timer = 99999;

    game_update(&gs);

    ASSERT(gs.zombies[0].hp == ZOMBIE_HP - PEA_DAMAGE,
           "zombie HP should decrease by 1");
    ASSERT(gs.projectiles[0].active == 0,
           "pea should be deactivated after hit");

    printf("  test_collision: OK\n");
}

static void test_zombie_death(void)
{
    game_state_t gs;
    game_init(&gs);

    gs.zombies[0].active = 1;
    gs.zombies[0].row = 0;
    gs.zombies[0].x_pixel = 300;
    gs.zombies[0].hp = 1; /* One hit from death */
    gs.zombies[0].move_counter = 0;

    gs.projectiles[0].active = 1;
    gs.projectiles[0].row = 0;
    gs.projectiles[0].x_pixel = 300;

    gs.zombies_spawned = 1;
    gs.spawn_timer = 99999;

    game_update(&gs);

    ASSERT(gs.zombies[0].active == 0, "zombie should be dead");

    printf("  test_zombie_death: OK\n");
}

static void test_lose_condition(void)
{
    game_state_t gs;
    game_init(&gs);

    gs.zombies[0].active = 1;
    gs.zombies[0].row = 0;
    gs.zombies[0].x_pixel = 1; /* Almost at left edge */
    gs.zombies[0].hp = 3;
    gs.zombies[0].move_counter = ZOMBIE_SPEED_FRAMES - 1; /* Will move next frame */

    gs.zombies_spawned = 1;
    gs.spawn_timer = 99999;

    game_update(&gs);

    ASSERT(gs.state == STATE_LOSE, "game should be in LOSE state");

    printf("  test_lose_condition: OK\n");
}

static void test_win_condition(void)
{
    game_state_t gs;
    game_init(&gs);

    /* All zombies spawned and dead */
    gs.zombies_spawned = TOTAL_ZOMBIES;
    gs.spawn_timer = 99999;
    for (int i = 0; i < MAX_ZOMBIES; i++)
        gs.zombies[i].active = 0;

    game_update(&gs);

    ASSERT(gs.state == STATE_WIN, "game should be in WIN state");

    printf("  test_win_condition: OK\n");
}

static void test_zombie_stops_at_plant(void)
{
    game_state_t gs;
    game_init(&gs);

    /* Place a plant at row 0, col 3 */
    gs.grid[0][3].type = PLANT_PEASHOOTER;
    gs.grid[0][3].hp = PLANT_HP;
    gs.grid[0][3].fire_cooldown = 99999;

    /* Place zombie in same row, just entering the plant's cell.
     * Plant at col 3 spans pixels [240, 320).  Put zombie at x=241
     * so col = 241/80 = 3, which has a plant. */
    gs.zombies[0].active = 1;
    gs.zombies[0].row = 0;
    gs.zombies[0].x_pixel = 241;
    gs.zombies[0].hp = ZOMBIE_HP;
    gs.zombies[0].move_counter = ZOMBIE_SPEED_FRAMES - 1; /* will move next frame */
    gs.zombies[0].eating = 0;
    gs.zombies[0].eat_timer = 0;

    gs.zombies_spawned = TOTAL_ZOMBIES;
    gs.spawn_timer = 99999;

    game_update(&gs);

    /* Zombie should have moved to x=240 and started eating */
    ASSERT(gs.zombies[0].eating == 1, "zombie should be eating the plant");
    ASSERT(gs.zombies[0].x_pixel == 240, "zombie should have stopped at x=240");

    /* Next frame: zombie should NOT move */
    int prev_x = gs.zombies[0].x_pixel;
    game_update(&gs);
    ASSERT(gs.zombies[0].x_pixel == prev_x,
           "zombie should not move while eating");

    printf("  test_zombie_stops_at_plant: OK\n");
}

static void test_zombie_eats_and_destroys_plant(void)
{
    game_state_t gs;
    game_init(&gs);

    /* Place a plant with 1 HP at row 1, col 2 */
    gs.grid[1][2].type = PLANT_PEASHOOTER;
    gs.grid[1][2].hp = 1;
    gs.grid[1][2].fire_cooldown = 99999;

    /* Zombie already eating with timer about to expire */
    gs.zombies[0].active = 1;
    gs.zombies[0].row = 1;
    gs.zombies[0].x_pixel = 2 * CELL_WIDTH; /* col 2 */
    gs.zombies[0].hp = ZOMBIE_HP;
    gs.zombies[0].move_counter = 0;
    gs.zombies[0].eating = 1;
    gs.zombies[0].eat_timer = 1; /* will bite next frame */

    gs.zombies_spawned = TOTAL_ZOMBIES;
    gs.spawn_timer = 99999;

    game_update(&gs);

    ASSERT(gs.grid[1][2].type == PLANT_NONE,
           "plant should be destroyed after last HP eaten");
    ASSERT(gs.zombies[0].eating == 0,
           "zombie should stop eating after plant destroyed");

    printf("  test_zombie_eats_and_destroys_plant: OK\n");
}

static void test_zombie_resumes_after_eating(void)
{
    game_state_t gs;
    game_init(&gs);

    /* Zombie was eating but plant is already gone (e.g. destroyed by
     * another zombie). */
    gs.zombies[0].active = 1;
    gs.zombies[0].row = 0;
    gs.zombies[0].x_pixel = 3 * CELL_WIDTH;
    gs.zombies[0].hp = ZOMBIE_HP;
    gs.zombies[0].move_counter = ZOMBIE_SPEED_FRAMES - 1;
    gs.zombies[0].eating = 1;
    gs.zombies[0].eat_timer = 10;

    /* No plant at col 3 */
    gs.grid[0][3].type = PLANT_NONE;

    gs.zombies_spawned = TOTAL_ZOMBIES;
    gs.spawn_timer = 99999;

    game_update(&gs);

    ASSERT(gs.zombies[0].eating == 0,
           "zombie should clear eating state when plant is gone");
    /* Zombie should have moved (move_counter was at threshold-1, then
     * incremented to threshold and moved) */
    ASSERT(gs.zombies[0].x_pixel == 3 * CELL_WIDTH - 1,
           "zombie should resume moving after plant is gone");

    printf("  test_zombie_resumes_after_eating: OK\n");
}

static void test_two_zombies_eat_same_plant(void)
{
    game_state_t gs;
    game_init(&gs);

    /* Place a plant with 2 HP at row 0, col 4 */
    gs.grid[0][4].type = PLANT_PEASHOOTER;
    gs.grid[0][4].hp = 2;
    gs.grid[0][4].fire_cooldown = 99999;

    /* Two zombies eating the same plant, both about to bite */
    for (int i = 0; i < 2; i++) {
        gs.zombies[i].active = 1;
        gs.zombies[i].row = 0;
        gs.zombies[i].x_pixel = 4 * CELL_WIDTH + i; /* both in col 4 */
        gs.zombies[i].hp = ZOMBIE_HP;
        gs.zombies[i].move_counter = 0;
        gs.zombies[i].eating = 1;
        gs.zombies[i].eat_timer = 1;
    }

    gs.zombies_spawned = TOTAL_ZOMBIES;
    gs.spawn_timer = 99999;

    game_update(&gs);

    /* Both bitten; plant had 2 HP so should be destroyed */
    ASSERT(gs.grid[0][4].type == PLANT_NONE,
           "plant should be destroyed by two simultaneous bites");
    /* Zombie 1 (processed second) destroyed the plant and cleared its
     * eating state.  Zombie 0 will discover the plant is gone next frame. */
    ASSERT(gs.zombies[1].eating == 0,
           "zombie 1 should stop eating after plant destroyed");

    /* Second frame: zombie 0 re-checks and finds plant gone */
    game_update(&gs);
    ASSERT(gs.zombies[0].eating == 0,
           "zombie 0 should stop eating once it notices plant is gone");

    printf("  test_two_zombies_eat_same_plant: OK\n");
}

int main(void)
{
    srand(42); /* Deterministic for testing */

    printf("Running game logic tests...\n\n");

    test_init();
    test_place_plant();
    test_remove_plant();
    test_sun_economy();
    test_collision();
    test_zombie_death();
    test_lose_condition();
    test_win_condition();
    test_zombie_stops_at_plant();
    test_zombie_eats_and_destroys_plant();
    test_zombie_resumes_after_eating();
    test_two_zombies_eat_same_plant();

    printf("\nResults: %d passed, %d failed\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
