#ifndef _PVZ_H
#define _PVZ_H

#include <linux/ioctl.h>

/*
 * Shared header for PvZ GPU kernel driver and userspace programs.
 *
 * Register byte offsets (from driver base):
 *   0x00  BG_CELL      - background grid cell color
 *   0x04  SHAPE_ADDR   - shape table entry select
 *   0x08  SHAPE_DATA0  - shape type/visible/x/y
 *   0x0C  SHAPE_DATA1  - shape w/h/color
 *   0x10  SHAPE_COMMIT - commit shape entry
 */

/* Avalon register byte offsets */
#define PVZ_BG_CELL       0x00
#define PVZ_SHAPE_ADDR    0x04
#define PVZ_SHAPE_DATA0   0x08
#define PVZ_SHAPE_DATA1   0x0C
#define PVZ_SHAPE_COMMIT  0x10

/* Shape types */
#define SHAPE_RECT    0
#define SHAPE_CIRCLE  1
#define SHAPE_DIGIT   2
#define SHAPE_SPRITE  3  /* 32x32 sprite ROM, rendered at 2x -> 64x64 on screen */

/* Background cell write argument */
typedef struct {
    unsigned char row;    /* 0-3 */
    unsigned char col;    /* 0-7 */
    unsigned char color;  /* palette index 0-255 */
} pvz_bg_arg_t;

/* Shape write argument */
typedef struct {
    unsigned char index;   /* shape table index 0-47 */
    unsigned char type;    /* SHAPE_RECT, SHAPE_CIRCLE, SHAPE_DIGIT */
    unsigned char visible; /* 1=visible, 0=hidden */
    unsigned short x;      /* x position 0-639 */
    unsigned short y;      /* y position 0-479 */
    unsigned short w;      /* width (or digit value in low 4 bits for SHAPE_DIGIT) */
    unsigned short h;      /* height */
    unsigned char color;   /* palette color index */
} pvz_shape_arg_t;

#define PVZ_MAGIC 'p'
#define PVZ_WRITE_BG       _IOW(PVZ_MAGIC, 1, pvz_bg_arg_t)
#define PVZ_WRITE_SHAPE    _IOW(PVZ_MAGIC, 2, pvz_shape_arg_t)
#define PVZ_COMMIT_SHAPES  _IO(PVZ_MAGIC, 3)

#endif /* _PVZ_H */
