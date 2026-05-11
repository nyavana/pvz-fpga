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
 *    0..31   PLANT[row*8 + col]   bit 0 = peashooter present
 *   32..39   ZOMBIE[i]            bit 31 = alive
 *                                 bits [9:0]   = x_pixel (0..639)
 *                                 bits [11:10] = row (0..3)
 *   40..47   PEA[i]               same encoding as ZOMBIE
 *   48       CURSOR               bit 31 = visible
 *                                 bits [4:2] = col (0..7)
 *                                 bits [1:0] = row (0..3)
 *   49       SUN                  bits [13:0] = sun count (reserved)
 *
 * Layout constants must match hw/bg_grid.sv and hw/entity_drawer.sv.
 */

/* Grid + screen layout */
#define PVZ_GRID_ROWS    4
#define PVZ_GRID_COLS    8
#define PVZ_CELL_SIZE    64
#define PVZ_GRID_X       64
#define PVZ_GRID_Y       112
#define PVZ_SCREEN_W     640
#define PVZ_SCREEN_H     480

/* Per-entity capacity exposed by hardware */
#define PVZ_MAX_ZOMBIES  8
#define PVZ_MAX_PEAS     8

/* Word indices in the register file */
#define PVZ_REG_PLANTS           0                     /* 0..31 */
#define PVZ_REG_ZOMBIE(idx)      (32 + (idx))          /* 32..39 */
#define PVZ_REG_PEA(idx)         (40 + (idx))          /* 40..47 */
#define PVZ_REG_CURSOR           48
#define PVZ_REG_SUN              49
#define PVZ_NUM_REGS             50

/* Pack a zombie/pea word: alive in bit 31, row in [11:10], x in [9:0] */
static inline unsigned int pvz_pack_entity(int alive, int row, int x_pixel)
{
    return ((unsigned)(alive & 1) << 31) |
           ((unsigned)(row   & 3) << 10) |
           ((unsigned)(x_pixel & 0x3FF));
}

/* Pack a cursor word: visible in bit 31, col in [4:2], row in [1:0] */
static inline unsigned int pvz_pack_cursor(int visible, int row, int col)
{
    return ((unsigned)(visible & 1) << 31) |
           ((unsigned)(col     & 7) << 2)  |
           ((unsigned)(row     & 3));
}

/* ioctl argument: write `value` to register `word_index` */
typedef struct {
    unsigned int word_index;   /* 0..PVZ_NUM_REGS-1 */
    unsigned int value;
} pvz_write_arg_t;

#define PVZ_MAGIC 'p'
#define PVZ_WRITE_REG  _IOW(PVZ_MAGIC, 1, pvz_write_arg_t)

#endif /* _PVZ_H */
