/*
 * Game state to FPGA shape table renderer
 *
 * Converts the game state into shape table entries and background cells,
 * then writes them to the FPGA via ioctls.
 *
 * Shape table index allocation:
 *   0-15:  Plants (2 shapes each, 8 slots = 16 shapes max)
 *          For 4 rows x 8 cols = 32 cells, but max ~8 plants expected
 *          Actually allocate based on grid position:
 *          index = (row * GRID_COLS + col) for body (0-31)
 *   0-31:  Plant shapes (body at even, stem at odd: 2 per cell, 32 cells)
 *   32-41: Zombies (2 shapes each: body + head, 5 zombies)
 *   42-47: Projectiles (1 shape each, up to 6 visible)
 *          Remaining projectiles not drawn if >6 active
 *
 * Revised allocation to fit 48 entries (higher index = drawn on top):
 *   0-31:  Plants: 32 grid cells, 1 shape each (simplified body)
 *   32-36: Zombies: 5 zombies, 1 shape each (simplified body)
 *   37-42: Projectiles: 6 peas, 1 shape each
 *   43-46: HUD digits (up to 4 digits for sun counter)
 *   47:    Cursor (1 shape)
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include "render.h"
#include "pvz.h"

static int fd;

/* Shape index allocation (higher index = drawn on top via painter's algorithm) */
#define IDX_PLANT_START   0
#define IDX_PLANT_COUNT   32  /* one per grid cell */
#define IDX_ZOMBIE_START  32
#define IDX_ZOMBIE_COUNT  5
#define IDX_PEA_START     37
#define IDX_PEA_COUNT     6
#define IDX_HUD_START     43
#define IDX_HUD_COUNT     4
#define IDX_CURSOR        47

/* Color indices (must match color_palette.sv) */
#define COL_BLACK       0
#define COL_DARK_GREEN  1
#define COL_LIGHT_GREEN 2
#define COL_BROWN       3
#define COL_YELLOW      4
#define COL_RED         5
#define COL_DARK_RED    6
#define COL_GREEN       7
#define COL_DARK_GREEN2 8
#define COL_BRIGHT_GREEN 9
#define COL_WHITE       10
#define COL_GRAY        11
#define COL_ORANGE      12

static void write_bg(int row, int col, int color)
{
    pvz_bg_arg_t bg = { .row = row, .col = col, .color = color };
    ioctl(fd, PVZ_WRITE_BG, &bg);
}

static void write_shape(int index, int type, int visible,
                        int x, int y, int w, int h, int color)
{
    pvz_shape_arg_t s = {
        .index   = index,
        .type    = type,
        .visible = visible,
        .x       = x,
        .y       = y,
        .w       = w,
        .h       = h,
        .color   = color
    };
    ioctl(fd, PVZ_WRITE_SHAPE, &s);
}

static void hide_shape(int index)
{
    write_shape(index, SHAPE_RECT, 0, 0, 0, 0, 0, 0);
}

int render_init(int fpga_fd)
{
    fd = fpga_fd;
    return 0;
}

static void render_background(const game_state_t *gs)
{
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            write_bg(r, c, ((r + c) % 2 == 0) ? COL_DARK_GREEN : COL_LIGHT_GREEN);
}

static void render_cursor(const game_state_t *gs)
{
    int x = gs->cursor_col * CELL_WIDTH;
    int y = GAME_AREA_Y + gs->cursor_row * CELL_HEIGHT;
    /* Draw cursor as a yellow border rectangle */
    write_shape(IDX_CURSOR, SHAPE_RECT, 1, x, y, CELL_WIDTH, CELL_HEIGHT, COL_YELLOW);
}

static void render_plants(const game_state_t *gs)
{
    /* Peashooter sprite: 32x32 source rendered at 2x -> 64x64 on screen.
     * Cell is CELL_WIDTH x CELL_HEIGHT (80x90). Center the 64x64 in the cell:
     *   x offset = (80 - 64) / 2 = 8
     *   y offset = (90 - 64) / 2 = 13 */
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int idx = IDX_PLANT_START + r * GRID_COLS + c;
            const plant_t *p = &gs->grid[r][c];

            if (p->type == PLANT_PEASHOOTER) {
                int cx = c * CELL_WIDTH + 8;
                int cy = GAME_AREA_Y + r * CELL_HEIGHT + 13;
                write_shape(idx, SHAPE_SPRITE, 1, cx, cy, 64, 64, 0);
            } else {
                hide_shape(idx);
            }
        }
    }
}

static void render_zombies(const game_state_t *gs)
{
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        int idx = IDX_ZOMBIE_START + i;
        const zombie_t *z = &gs->zombies[i];

        if (z->active) {
            int y = GAME_AREA_Y + z->row * CELL_HEIGHT + 10;
            /* Zombie body: red rectangle */
            write_shape(idx, SHAPE_RECT, 1, z->x_pixel, y,
                       ZOMBIE_WIDTH, ZOMBIE_HEIGHT, COL_RED);
        } else {
            hide_shape(idx);
        }
    }
}

static void render_projectiles(const game_state_t *gs)
{
    int drawn = 0;
    for (int i = 0; i < MAX_PROJECTILES && drawn < IDX_PEA_COUNT; i++) {
        const projectile_t *p = &gs->projectiles[i];
        if (p->active) {
            int idx = IDX_PEA_START + drawn;
            int y = GAME_AREA_Y + p->row * CELL_HEIGHT + (CELL_HEIGHT / 2) - (PEA_SIZE / 2);
            /* Pea: bright green circle */
            write_shape(idx, SHAPE_CIRCLE, 1, p->x_pixel, y,
                       PEA_SIZE, PEA_SIZE, COL_BRIGHT_GREEN);
            drawn++;
        }
    }
    /* Hide unused pea slots */
    for (int i = drawn; i < IDX_PEA_COUNT; i++)
        hide_shape(IDX_PEA_START + i);
}

static void render_hud(const game_state_t *gs)
{
    /* Display sun counter as 7-segment digits in the HUD area (y=10) */
    int sun = gs->sun;
    int digits[IDX_HUD_COUNT];
    int num_digits = 0;

    if (sun == 0) {
        digits[0] = 0;
        num_digits = 1;
    } else {
        int temp = sun;
        while (temp > 0 && num_digits < IDX_HUD_COUNT) {
            digits[num_digits++] = temp % 10;
            temp /= 10;
        }
        /* Reverse */
        for (int i = 0; i < num_digits / 2; i++) {
            int t = digits[i];
            digits[i] = digits[num_digits - 1 - i];
            digits[num_digits - 1 - i] = t;
        }
    }

    /* Draw digits left-to-right at top of screen.
     * The hardware draw loop uses s_w as the pixel width bound, but
     * decode_7seg reads the digit value from s_w[3:0].  Pass w = 32 + digit
     * so the draw loop covers the full 20-pixel glyph (32 > 20) while
     * s_w[3:0] still equals the digit value (32 & 0xF == 0). */
    for (int i = 0; i < IDX_HUD_COUNT; i++) {
        int idx = IDX_HUD_START + i;
        if (i < num_digits) {
            int x = 10 + i * 25;
            write_shape(idx, SHAPE_DIGIT, 1, x, 15,
                        32 + digits[i], 30, COL_WHITE);
        } else {
            hide_shape(idx);
        }
    }
}

static void render_game_over(const game_state_t *gs)
{
    /* Simple game-over indicator: large colored rectangle in center */
    if (gs->state == STATE_WIN) {
        /* Green "WIN" indicator */
        write_shape(IDX_CURSOR, SHAPE_RECT, 1, 220, 200, 200, 80, COL_BRIGHT_GREEN);
    } else if (gs->state == STATE_LOSE) {
        /* Red "LOSE" indicator */
        write_shape(IDX_CURSOR, SHAPE_RECT, 1, 220, 200, 200, 80, COL_RED);
    }
}

void render_frame(const game_state_t *gs)
{
    render_background(gs);

    if (gs->state == STATE_PLAYING) {
        render_plants(gs);
        render_zombies(gs);
        render_projectiles(gs);
        render_cursor(gs);
        render_hud(gs);
    } else {
        /* Hide all entity slots so they don't cover the panel */
        for (int i = 0; i < IDX_PLANT_COUNT; i++)
            hide_shape(IDX_PLANT_START + i);
        for (int i = 0; i < IDX_ZOMBIE_COUNT; i++)
            hide_shape(IDX_ZOMBIE_START + i);
        for (int i = 0; i < IDX_PEA_COUNT; i++)
            hide_shape(IDX_PEA_START + i);
        render_game_over(gs);
    }
}
