#ifndef _GAME_H
#define _GAME_H

#include "pvz.h"

/* Grid dimensions (must match hw/entity_drawer.sv) */
#define GRID_ROWS     PVZ_GRID_ROWS
#define GRID_COLS     PVZ_GRID_COLS
#define CELL_SIZE     PVZ_CELL_SIZE
#define GAME_AREA_X   PVZ_GRID_X
#define GAME_AREA_Y   PVZ_GRID_Y

/* Screen dimensions */
#define SCREEN_W      PVZ_SCREEN_W
#define SCREEN_H      PVZ_SCREEN_H

/* Plant constants */
#define PLANT_COST          50
#define SUNFLOWER_COST      50
#define PLANT_FIRE_COOLDOWN 120  /* frames (2 seconds at 60fps) */
#define PLANT_HP            3    /* hits before a plant is destroyed */

/* Zombie constants (sprite size matches hardware: 32x64) */
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
#define MAX_PROJECTILES     16
#define PEA_SPEED           2   /* pixels per frame */
#define PEA_DAMAGE          1
#define PEA_SIZE            8

/* Sun economy */
#define INITIAL_SUN         100
#define SUN_INCREMENT       25
#define SUN_INTERVAL        (8 * 60)  /* 8 seconds in frames */

/* Game states */
#define STATE_PLAYING  0
#define STATE_WIN      1
#define STATE_LOSE     2

/* Plant types */
#define PLANT_NONE        0
#define PLANT_PEASHOOTER  1
#define PLANT_SUNFLOWER   2

typedef struct {
    int type;           /* PLANT_NONE or PLANT_PEASHOOTER */
    int fire_cooldown;  /* frames until next shot */
    int hp;             /* hit points remaining */
} plant_t;

typedef struct {
    int active;
    int row;
    int x_pixel;        /* screen pixel x; moves leftward */
    int hp;
    int move_counter;   /* counts frames until next pixel move */
    int eating;         /* 1 if currently eating a plant */
    int eat_timer;      /* frames until next bite */
} zombie_t;

typedef struct {
    int active;
    int row;
    int x_pixel;        /* screen pixel x; moves rightward */
} projectile_t;

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

void game_init(game_state_t *gs);
void game_update(game_state_t *gs);
int  game_place_plant(game_state_t *gs);
int  game_remove_plant(game_state_t *gs);

#endif /* _GAME_H */
