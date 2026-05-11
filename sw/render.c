/*
 * Game state -> FPGA register file
 *
 * Each frame we walk the game state and write one 32-bit value per
 * entity into the hardware register file via the PVZ_WRITE_REG ioctl.
 * No commit handshake: the entity_drawer reads whatever is in the
 * registers as it scans each line.
 *
 * Hardware capacity: 32 plant cells, 8 zombies, 8 peas, 1 cursor.
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "render.h"
#include "pvz.h"

static int fd;

static void write_reg(unsigned int word_index, unsigned int value)
{
    pvz_write_arg_t w = { .word_index = word_index, .value = value };
    ioctl(fd, PVZ_WRITE_REG, &w);
}

int render_init(int fpga_fd)
{
    fd = fpga_fd;
    return 0;
}

static void render_plants(const game_state_t *gs)
{
    uint32_t pea_bits = 0;
    uint32_t sun_bits = 0;
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int t = gs->grid[r][c].type;
            if (t == PLANT_PEASHOOTER)
                pea_bits |= (1u << (r * 8 + c));
            else if (t == PLANT_SUNFLOWER)
                sun_bits |= (1u << (r * 8 + c));
        }
    }
    write_reg(PVZ_REG_PLANTS,    pea_bits);
    write_reg(PVZ_REG_SUNFLOWER, sun_bits);
}

static void render_selected(const game_state_t *gs)
{
    int sel = (gs->selected_plant_type == PLANT_SUNFLOWER) ? 1 : 0;
    write_reg(PVZ_REG_SELECTED, sel);
}

static void render_zombies(const game_state_t *gs)
{
    for (int i = 0; i < PVZ_MAX_ZOMBIES; i++) {
        int alive = 0, row = 0, x = 0;
        if (i < MAX_ZOMBIES && gs->zombies[i].active) {
            alive = 1;
            row   = gs->zombies[i].row;
            x     = gs->zombies[i].x_pixel;
        }
        write_reg(PVZ_REG_ZOMBIE(i), pvz_pack_entity(alive, row, x));
    }
}

static void render_peas(const game_state_t *gs)
{
    /* Walk active projectiles in order, stop after PVZ_MAX_PEAS.
     * Hide hardware slots that don't get filled. */
    int slot = 0;
    for (int i = 0; i < MAX_PROJECTILES && slot < PVZ_MAX_PEAS; i++) {
        const projectile_t *p = &gs->projectiles[i];
        if (!p->active) continue;
        write_reg(PVZ_REG_PEA(slot), pvz_pack_entity(1, p->row, p->x_pixel));
        slot++;
    }
    for (; slot < PVZ_MAX_PEAS; slot++)
        write_reg(PVZ_REG_PEA(slot), 0);
}

static void render_cursor(const game_state_t *gs)
{
    int visible = (gs->state == STATE_PLAYING) ? 1 : 0;
    write_reg(PVZ_REG_CURSOR,
              pvz_pack_cursor(visible, gs->cursor_row, gs->cursor_col));
}

static void render_sun(const game_state_t *gs)
{
    /* HW currently doesn't draw the sun count; the register is reserved
     * so a future HUD module can pick it up. */
    write_reg(PVZ_REG_SUN, gs->sun & 0x3FFF);
}

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

void render_frame(const game_state_t *gs)
{
    if (gs->state == STATE_PLAYING) {
        render_plants(gs);
        render_zombies(gs);
        render_peas(gs);
        render_cursor(gs);
        render_sun(gs);
        render_selected(gs);
    } else {
        /* WIN / LOSE: clear everything; main loop prints a banner and
         * exits.  A future HUD pass can render a result indicator. */
        hide_all_entities();
        render_sun(gs);
        render_selected(gs);
    }
}
