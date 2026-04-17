#ifndef _GAME_H
#define _GAME_H

/* Grid dimensions */
#define GRID_ROWS     4
#define GRID_COLS     8

/* Game area pixel coordinates */
#define GAME_AREA_Y   60
#define CELL_WIDTH    80
#define CELL_HEIGHT   90

/* Screen dimensions */
#define SCREEN_W      640
#define SCREEN_H      480

/* Plant constants */
#define PLANT_COST         50
#define PLANT_FIRE_COOLDOWN 120  /* frames (2 seconds at 60fps) */
#define PLANT_HP            3    /* hits before a plant is destroyed */

/* Zombie constants */
#define MAX_ZOMBIES          5
#define ZOMBIE_HP            3
#define ZOMBIE_SPEED_FRAMES  3   /* move 1 pixel every N frames (~20 px/s) */
#define ZOMBIE_WIDTH         30
#define ZOMBIE_HEIGHT        70
#define TOTAL_ZOMBIES        5
#define ZOMBIE_SPAWN_MIN    (8 * 60)  /* 8 seconds in frames */
#define ZOMBIE_SPAWN_MAX    (15 * 60) /* 15 seconds in frames */
#define ZOMBIE_EAT_COOLDOWN 60       /* frames between bites (~1 sec at 60fps) */

/* Projectile constants */
#define MAX_PROJECTILES     16
#define PEA_SPEED           2   /* pixels per frame (~120 px/s) */
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

typedef struct {
    int type;           /* PLANT_NONE or PLANT_PEASHOOTER */
    int fire_cooldown;  /* frames until next shot */
    int hp;             /* hit points remaining */
} plant_t;

typedef struct {
    int active;
    int row;
    int x_pixel;        /* pixel x position (moves leftward) */
    int hp;
    int move_counter;   /* counts frames until next pixel move */
    int eating;         /* 1 if currently eating a plant */
    int eat_timer;      /* frames until next bite */
} zombie_t;

typedef struct {
    int active;
    int row;
    int x_pixel;        /* pixel x position (moves rightward) */
} projectile_t;

typedef struct {
    /* Grid */
    plant_t grid[GRID_ROWS][GRID_COLS];

    /* Entities */
    zombie_t zombies[MAX_ZOMBIES];
    projectile_t projectiles[MAX_PROJECTILES];

    /* Cursor */
    int cursor_row;
    int cursor_col;

    /* Economy */
    int sun;
    int sun_timer;      /* frames until next sun increment */

    /* Spawn */
    int zombies_spawned;
    int spawn_timer;    /* frames until next zombie spawn */

    /* Game state */
    int state;          /* STATE_PLAYING, STATE_WIN, STATE_LOSE */
    int frame_count;
} game_state_t;

/*
 * Initialize game state to starting values.
 */
void game_init(game_state_t *gs);

/*
 * Process one frame of game logic.
 * Call after input has been processed.
 */
void game_update(game_state_t *gs);

/*
 * Attempt to place a peashooter at the cursor position.
 * Returns 1 on success, 0 if blocked.
 */
int game_place_plant(game_state_t *gs);

/*
 * Remove the plant at the cursor position.
 * Returns 1 if a plant was removed, 0 if cell was empty.
 */
int game_remove_plant(game_state_t *gs);

#endif /* _GAME_H */
