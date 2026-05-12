# 06 — Avalon-MM register map reference

This is the complete contract between `sw/render.c` (writer) and
`hw/pvz_top.sv` + `hw/entity_drawer.sv` (readers). Treat this chapter
as the source of truth when any part of the system disagrees about
what a bit means.

![Register map](diagrams/08-register-map.svg)

## Layout

51 32-bit words live in the FPGA register file. The Avalon slave is
configured `addressUnits = WORDS` (`hw/pvz_top_hw.tcl:52`), so the
`address` port carries word indices 0..63. The kernel driver
multiplies its word index by 4 to get a byte offset for `iowrite32`
(`sw/pvz_driver.c:40`), since CPU memory is always byte-addressed.

| word | byte offset | name | who writes | who reads in hw |
| ---: | -----------:| ---- | ---------- | --------------- |
| 0  | 0x00 | `PVZ_REG_PLANTS`     | `render.c:render_plants`    | `pvz_top.sv:115`, `entity_drawer.sv:162` |
| 1  | 0x04 | `PVZ_REG_SUNFLOWER`  | `render.c:render_plants`    | `pvz_top.sv:119`, `entity_drawer.sv:163` |
| 2..31 | 0x08..0x7c | *(unused / reserved)* | — | — |
| 32..39 | 0x80..0x9c | `PVZ_REG_ZOMBIE(0..7)` | `render.c:render_zombies` | `pvz_top.sv:122-126`, `entity_drawer.sv:175-194` |
| 40..47 | 0xa0..0xbc | `PVZ_REG_PEA(0..7)`    | `render.c:render_peas`    | `pvz_top.sv:128-132`, `entity_drawer.sv:199-211` |
| 48 | 0xc0 | `PVZ_REG_CURSOR`     | `render.c:render_cursor`   | `pvz_top.sv:134-138`, `entity_drawer.sv:213-230` |
| 49 | 0xc4 | `PVZ_REG_SUN`        | `render.c:render_sun`      | `pvz_top.sv:140-142`, `entity_drawer.sv:266-280` |
| 50 | 0xc8 | `PVZ_REG_SELECTED`   | `render.c:render_selected` | `pvz_top.sv:144-146`, `entity_drawer.sv:354-357` |

## Bit layouts

### `PVZ_REG_PLANTS` (word 0)

A 32-bit bitmap. Bit `i = row * 8 + col` is high when a peashooter is
present at grid cell (row, col), with `row ∈ [0..3]` and
`col ∈ [0..7]`. All 32 bits are meaningful; nothing is reserved.

```
bit:  31 30 29 ... | 23 22 ... | 15 14 ... |  7  6  5  ...  1  0
cell: r3c7 r3c6 ... r2c7 r2c6 ... r1c7 r1c6 ... r0c7 r0c6 ... r0c1 r0c0
```

Software builds this directly: `pea_bits |= 1u << (r * 8 + c)`
(`render.c:40`). Hardware indexes it with the 5-bit cell index
`{cell_row, cell_col}` (`entity_drawer.sv:161-162`).

### `PVZ_REG_SUNFLOWER` (word 1)

Same encoding as `PVZ_REG_PLANTS`. A given cell can be in either
bitmap but not both — `game_place_plant` (`sw/game.c:34-51`) refuses
to plant onto an occupied cell, so this invariant is preserved.

### `PVZ_REG_ZOMBIE(i)` (words 32..39)

```
bit 31      [11:10]    [9:0]
alive       row        x_pixel
            0..3       0..639
```

Software builds this with `pvz_pack_entity(alive, row, x_pixel)`
(`render.c:64`, `pvz.h:52-57`):

```c
((alive & 1) << 31) | ((row & 3) << 10) | (x_pixel & 0x3FF)
```

Hardware unpacks at `pvz_top.sv:123-125`:

```systemverilog
zombie_alive[i] <= writedata[31];
zombie_x[i]     <= writedata[9:0];
zombie_row[i]   <= writedata[11:10];
```

`alive = 0` hides the slot (`entity_drawer.sv:184` skips inactive
zombies in the priority encoder). The on-screen bounding box is
64 × 64 pixels at `(x_pixel, GRID_Y + row * 64)`. Bits 12..30 are
unused; write them as zero.

### `PVZ_REG_PEA(i)` (words 40..47)

Same encoding as `PVZ_REG_ZOMBIE`. The visual is an 8 × 8 solid
bright-green square, centered vertically in the row by adding a
+28 px offset (`entity_drawer.sv:205`). Software writes the *top*
of the row, not the centered top — hardware adjusts.

### `PVZ_REG_CURSOR` (word 48)

```
bit 31      [4:2]    [1:0]
visible     col      row
            0..7     0..3
```

Software uses `pvz_pack_cursor(visible, row, col)` (`pvz.h:60-65`,
`render.c:87`). Hardware reads at `pvz_top.sv:135-137`. When
`visible = 0`, the cursor border draws nothing
(`entity_drawer.sv:220`). The 4-pixel-thick yellow border surrounds
the cell `(col, row)` regardless of any plant or zombie under it.

### `PVZ_REG_SUN` (word 49)

```
bits [13:0] sun count (0..16383)
```

Used by the sun HUD logic at the top-right of the screen
(`entity_drawer.sv:266-280`). One yellow block lights up per 50 sun
units, up to 10 blocks (so 500 sun maxes the display, but the
internal count keeps rising). Bits 14..31 are unused; software masks
to 14 bits in `render_sun` (`render.c:94`).

### `PVZ_REG_SELECTED` (word 50)

```
bits [1:0]
  0  → peashooter
  1  → sunflower
```

Toggled by Tab / shoulder buttons (`main.c:58-62`). The hardware
uses this in `entity_drawer.sv:354-357` to decide which of the two
plant-selector boxes gets the yellow cursor border this frame.

## The byte-vs-word gotcha, in one place

When you debug this, you will inevitably ask: "what byte address is
this register at, really?"

- The CPU writes to byte address `virtbase + word_index * 4`. That's
  what `iowrite32` consumes (`sw/pvz_driver.c:40`).
- The Avalon bus delivers that as an `address` value of `word_index`
  to the slave, because `addressUnits = WORDS`
  (`hw/pvz_top_hw.tcl:52`). One bus transaction per word.
- The slave decoder compares `address` against literal word indices:
  `if (address == 6'd0) ... else if (address < 6'd40) ...`
  (`hw/pvz_top.sv:114-146`).

So in the kernel, byte arithmetic. In the SystemVerilog, word
arithmetic. Don't mix the two when reading dmesg / /proc/iomem
output.

## Why the gap at words 2..31

The bitmap split (peashooter in word 0, sunflower in word 1) plus
the zombie/pea layouts (8 each at words 32+) leaves words 2..31
intentionally unused. The hardware decoder simply never matches any
write to those addresses (`hw/pvz_top.sv:118-122`: after word 1, the
next branch is `address < 6'd40`, which means addresses 2..31 fall
through into the zombie branch and would write to `zombie_*` slot
`address[2:0]` — which **also** has unintended side effects). In
practice the software never writes to those words, so the
unreachable branch is harmless. If you add new plant types or HUD
elements, consider using those low addresses to keep the layout
dense.

## Adding a new register

If you want to expose a new piece of state to the FPGA (say, a new
HUD counter):

1. Pick the next free word index (e.g. 51). Update `PVZ_NUM_REGS`
   in `sw/pvz.h:49`.
2. Add a `PVZ_REG_*` macro to `sw/pvz.h:42-48`.
3. In `hw/pvz_top.sv`, add a register `logic [...]`
   declaration, initialize it in the reset block (`pvz_top.sv:96-111`),
   and decode the new address in `pvz_top.sv:112-148`. Pipe the
   value out to `entity_drawer` via a new port if it needs to be
   drawn.
4. In `hw/entity_drawer.sv`, add the port and any hit-detection /
   compositing logic.
5. Update `sw/render.c` with a `render_*` helper and call it from
   `render_frame`.
6. If your address would exceed 63, widen the `address` port
   (`pvz_top.sv:35` and `pvz_top_hw.tcl:69`) — currently 6 bits.

The compatible string in `pvz_top_hw.tcl:23` and `pvz_driver.c:112`
does not need to change; the kernel ioctl is generic.
