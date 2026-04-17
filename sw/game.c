/*
 * PvZ game logic
 *
 * Manages the 4x8 grid, zombie spawning/movement, peashooter firing,
 * projectile movement, collision detection, sun economy, and
 * win/lose conditions.
 */

#include <stdlib.h>
#include <string.h>
#include "game.h"

/* Simple pseudo-random using the C library rand() */
static int random_range(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

void game_init(game_state_t *gs)
{
    memset(gs, 0, sizeof(*gs));

    gs->sun = INITIAL_SUN;
    gs->sun_timer = SUN_INTERVAL;
    gs->state = STATE_PLAYING;
    gs->cursor_row = 0;
    gs->cursor_col = 0;
    gs->zombies_spawned = 0;
    gs->spawn_timer = random_range(ZOMBIE_SPAWN_MIN, ZOMBIE_SPAWN_MAX);
    gs->frame_count = 0;
}

int game_place_plant(game_state_t *gs)
{
    int r = gs->cursor_row;
    int c = gs->cursor_col;

    if (gs->grid[r][c].type != PLANT_NONE)
        return 0;
    if (gs->sun < PLANT_COST)
        return 0;

    gs->grid[r][c].type = PLANT_PEASHOOTER;
    gs->grid[r][c].fire_cooldown = PLANT_FIRE_COOLDOWN;
    gs->grid[r][c].hp = PLANT_HP;
    gs->sun -= PLANT_COST;
    return 1;
}

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
static int zombie_in_row(game_state_t *gs, int row)
{
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (gs->zombies[i].active && gs->zombies[i].row == row)
            return 1;
    }
    return 0;
}

/* Spawn a new pea projectile at the given grid cell */
static void spawn_pea(game_state_t *gs, int row, int col)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!gs->projectiles[i].active) {
            gs->projectiles[i].active = 1;
            gs->projectiles[i].row = row;
            /* Start at the right edge of the plant's cell */
            gs->projectiles[i].x_pixel = (col + 1) * CELL_WIDTH;
            return;
        }
    }
    /* No free slot; pea is lost */
}

/* Update zombie positions, eating, and check for lose condition */
static void update_zombies(game_state_t *gs)
{
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        zombie_t *z = &gs->zombies[i];
        if (!z->active)
            continue;

        if (z->eating) {
            /* Re-check that the plant still exists (another zombie may
             * have destroyed it) */
            int col = z->x_pixel / CELL_WIDTH;
            if (col < 0 || col >= GRID_COLS ||
                gs->grid[z->row][col].type == PLANT_NONE) {
                z->eating = 0;
                z->eat_timer = 0;
                /* Fall through to movement below */
            } else {
                /* Continue eating: deal damage on timer */
                z->eat_timer--;
                if (z->eat_timer <= 0) {
                    gs->grid[z->row][col].hp--;
                    if (gs->grid[z->row][col].hp <= 0) {
                        gs->grid[z->row][col].type = PLANT_NONE;
                        gs->grid[z->row][col].fire_cooldown = 0;
                        gs->grid[z->row][col].hp = 0;
                        z->eating = 0;
                    } else {
                        z->eat_timer = ZOMBIE_EAT_COOLDOWN;
                    }
                }
                continue; /* Don't move while eating */
            }
        }

        /* Movement */
        z->move_counter++;
        if (z->move_counter >= ZOMBIE_SPEED_FRAMES) {
            z->move_counter = 0;
            z->x_pixel--;

            /* Lose condition: zombie reached left edge */
            if (z->x_pixel <= 0) {
                gs->state = STATE_LOSE;
                return;
            }

            /* Check for plant collision after moving */
            int col = z->x_pixel / CELL_WIDTH;
            if (col >= 0 && col < GRID_COLS &&
                gs->grid[z->row][col].type != PLANT_NONE) {
                z->eating = 1;
                z->eat_timer = ZOMBIE_EAT_COOLDOWN;
            }
        }
    }
}

/* Update projectile positions */
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
            int z_left = z->x_pixel;
            int z_right = z->x_pixel + ZOMBIE_WIDTH;
            int p_left = p->x_pixel;
            int p_right = p->x_pixel + PEA_SIZE;

            if (p_right >= z_left && p_left <= z_right) {
                /* Hit! */
                z->hp -= PEA_DAMAGE;
                p->active = 0;

                if (z->hp <= 0)
                    z->active = 0;

                break; /* Each pea hits only one zombie */
            }
        }
    }
}

/* Spawn zombies on a timer */
static void update_spawning(game_state_t *gs)
{
    if (gs->zombies_spawned >= TOTAL_ZOMBIES)
        return;

    gs->spawn_timer--;
    if (gs->spawn_timer <= 0) {
        /* Find a free zombie slot */
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

/* Update sun economy */
static void update_sun(game_state_t *gs)
{
    gs->sun_timer--;
    if (gs->sun_timer <= 0) {
        gs->sun += SUN_INCREMENT;
        gs->sun_timer = SUN_INTERVAL;
    }
}

/* Check win condition */
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
