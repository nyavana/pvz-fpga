# v2Dino changes

Summary of what's new on `v2Dino` relative to the v1 baseline
(`0a9eae0 fix hw`). Two upstream commits, plus this integration pass
that cleans artifacts and merges the later v1 fixes forward.

## 1. Sprite engine — `1306e3e "added pixel images"` (zz3410)

Introduces a real pixel-art path into the FPGA renderer. The existing
shape types (rect, circle, 7-seg digit) are joined by a fourth
`SHAPE_SPRITE = 3` type that reads 32×32 palette-indexed pixels from
on-chip ROM and draws them at 2× scale (64×64 on screen).

**New files**

- `hw/sprite_rom.sv` — 1024-byte ROM, inferred as M10K block RAM,
  initialized from `peas_idx.mem` via `$readmemh`. One-cycle read
  latency on `posedge clk`.
- `hw/peas_idx.mem` — 1024 bytes of peashooter sprite. Each byte is a
  palette index (`0x00`–`0x0C`, matching `color_palette.sv`) or `0xFF`
  for a transparent pixel.

**Hardware wiring**

- `hw/pvz_top.sv` — instantiates `sprite_rom` and exposes
  `sprite_rd_addr` / `sprite_rd_pixel` to the shape renderer.
- `hw/pvz_top_hw.tcl` — registers `sprite_rom.sv` and the
  `peas_idx.mem` data file with the Platform Designer fileset so qsys
  picks them up.
- `hw/shape_renderer.sv` — new branch for `s_type == 2'd3`:
  - Address is combinational: `{sprite_local_y[5:1],
    sprite_local_x[5:1]}`, i.e. drop the low bit of each local
    coordinate to achieve the 2× downscale (screen → ROM).
  - A 1-entry pipeline (`sprite_pending` / `sprite_pending_addr`)
    absorbs the 1-cycle ROM read latency: cycle N issues the fetch,
    cycle N+1 commits the write (unless the pixel is `0xFF`, in which
    case the write is suppressed — that's the transparency path).
  - When `draw_x` advances past the sprite width, the renderer stays
    one extra cycle to flush the last pending pixel before advancing
    `cur_shape`.
- `hw/tb/tb_shape_renderer.sv` — updated: stubs `sprite_rd_pixel` to
  `0xFF` since the existing TB doesn't exercise the sprite path yet.

**Software side**

- `sw/pvz.h` — defines `SHAPE_SPRITE 3` next to the other shape types.
- `sw/render.c::render_plants` — peashooters now emit `SHAPE_SPRITE`
  with `w=64, h=64`. The sprite is centered in the 80×90 grid cell:
  `cx = c * CELL_WIDTH + 8`, `cy = GAME_AREA_Y + r * CELL_HEIGHT + 13`
  (offsets `(80-64)/2 = 8` and `(90-64)/2 = 13`).

## 2. State-aware rendering — `0e4bd70 "Refactor rendering logic based on game state"` (tonyshtark)

Small but important fix in `sw/render.c::render_frame`: plant, zombie,
and projectile shapes are only pushed while `gs->state ==
STATE_PLAYING`. In any other state (menu, game-over, …), those 48
slots are explicitly hidden so previous frames' entities can't bleed
through the overlay. The HUD is also no longer drawn over the
game-over overlay.

## 3. Integration pass (this merge)

Two commits layered on top of v2Dino by this pass:

- `59feff5 clean compile artifacts per gitignore` — removes tracked
  Quartus build outputs (`hw/db/`, `hw/incremental_db/`,
  `hw/output_files/`, `hw/soc_system/`, `hw/hps_isw_handoff/`,
  generated `.dtb`/`.dts`/`.sopcinfo`/`.tcl`/`_board_info.xml`/pin
  dumps) plus the misplaced root-level `peas_idx.mem`, `peas_idx.mif`,
  `peas_mask.mif` duplicates. Syncs `.gitignore` with `main`. Only
  `hw/peas_idx.mem` is kept — that's the copy `pvz_top_hw.tcl`
  actually references.
- `2a478a4 Merge remote-tracking branch 'origin/v1-initial-run-fix'
  into v2Dino` — pulls forward everything v2Dino was missing:
  - PR #5's artifact cleanup (already done by `59feff5`, so merge is a
    no-op on those paths).
  - HUD z-order + digit-decode fix (`dc297d4`): shape indices
    renumbered so higher index = drawn on top (plants 0–31, zombies
    32–36, peas 37–42, HUD 43–46, cursor 47); digit value now passed
    as `32 + digits[i]` so `s_w[3:0]` still decodes the digit while
    the draw loop covers the full 20-pixel glyph.
  - `CLAUDE.md` added as tracked project instructions (`cbb379d`).
  - Design document and lab2 reference material added under `doc/`
    (`bc76361`, `97c0c0b`).

`sw/render.c` was the only file both sides touched. Git auto-merged
cleanly — v1's renumbering lives in the `#define` block at the top,
v2's sprite / state-gated rendering lives further down in
`render_plants` and `render_frame`, and they don't overlap.

## Post-merge verification

- `git ls-files -ci --exclude-standard` is empty (no tracked file
  matches an ignore rule).
- `sw/render.c` shows both `IDX_PLANT_START = 0` (v1) and
  `SHAPE_SPRITE` use in `render_plants` (v2).
- `hw/sprite_rom.sv`, `hw/peas_idx.mem`, `CLAUDE.md`,
  `doc/design-document/design-document.tex` all present.

The actual FPGA build (`make qsys && make quartus && make rbf`) and
cross-compile (`make CC=arm-linux-gnueabihf-gcc …`) still need to run
on a Columbia workstation — this pass is source-level only.
