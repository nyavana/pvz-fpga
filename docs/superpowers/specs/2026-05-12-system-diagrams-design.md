# PvZ-on-DE1-SoC — D2 System Diagram Catalog Design

**Date:** 2026-05-12
**Author:** nyavana (with Claude Code via /superpowers:brainstorming)
**Status:** Design (spec). Implementation plan to follow via writing-plans.
**Code target:** `v5-cursor-controller/` (final-review version with sprite-based renderer)

## 1. Purpose and audience

The deliverable is a comprehensive set of D2 diagrams that documents the
Plants vs Zombies on DE1-SoC system end-to-end and supports a multi-hour
final design review where the professor may probe any line of code.

The diagrams must:

- cover both software (HPS Linux: userspace game + kernel driver) and
  hardware (FPGA: VGA pipeline + Avalon-MM register file);
- include high-level overview diagrams (so the prof can ground himself
  in the architecture) and module-internal diagrams (so the prof can
  zoom into the part he is questioning);
- be selectable on demand — each diagram answers a specific likely
  question, so the presenter can pull up the relevant one without
  digging;
- reflect the system **as actually built** in `v5-cursor-controller/`,
  not planned or hypothetical features.

## 2. Scope and constraints

- **Code under documentation:** `v5-cursor-controller/`. Notable: this
  version uses a combinational "racing the beam" `entity_drawer.sv`
  (no linebuffer, no FSM-driven shape table), a flat 51-word Avalon-MM
  register file, sprite ROMs (`peashooter_idx.mem`, `sunflower_idx.mem`,
  `zombie_idx.mem`), and two plant types (Peashooter, Sunflower) with
  TAB-cycled selection.
- **Detail level:** mixed — block / datapath level for most modules;
  signal-level (with bit-widths) for a small number of hot-spot
  modules likely to attract deep questions.
- **Hot spots for signal-level treatment:**
  - `entity_drawer.sv` datapath (which layer wins per pixel; how
    sprite ROM addresses are computed; bit-widths of all inter-stage
    signals; where the 2-cycle latency lives).
  - `pvz_top.sv` register-file decoder (how a 6-bit word address
    + 32-bit writedata becomes the right field update; the absence
    of vsync latching and resulting one-frame tearing window).
- **Output format:** `.d2` source committed to git, plus rendered
  `.svg` and `.png` for each diagram, also committed.
- **Location:** `v5-cursor-controller/doc/diagrams/`.
- **Out of scope:** planned-but-unimplemented features (USB gamepad,
  audio, additional plant types), rejected/alternative designs, and
  diagrams of the legacy `main/` (Milestone 1 primitive-shape) version.

## 3. Folder layout and conventions

```
v5-cursor-controller/doc/diagrams/
  README.md          one-page index: every diagram with one-line description and source-file refs
  common.d2          shared classes (hw_module, sw_module, register, signal, fsm_state, bus, …)
  Makefile           `make all` renders every .d2 to SVG + PNG; `make clean`; `make <section>`
  01-system/         system / context overview (4 diagrams)
  02-hw/             FPGA module diagrams (9 diagrams)
  03-sw/             HPS userspace + kernel diagrams (7 diagrams)
  04-interface/      HW↔SW boundary (4 diagrams)
  05-cross-cutting/  timing, build, deploy, test (6 diagrams)
  06-flow/           end-to-end scenario diagrams (6 diagrams)
```

**Filename pattern:** `NN_short-name.d2` inside each section folder
(e.g. `02-hw/06_entity_drawer_pipeline.d2`). The two-digit prefix
defines the recommended presentation order within a section.

**Per-file header comment** (in every `.d2`):

```
# Title:       <short title>
# Section:     <01-system | 02-hw | …>
# Documents:   <source files: e.g., hw/entity_drawer.sv, sw/render.c>
# Defends:     <likely question(s) this diagram answers>
# Detail:      <block | signal>
```

**D2 conventions:**

- `direction: down` for layered/containment diagrams; `direction:
  right` for pipelines and flows.
- `shape: sequence_diagram` for boot, ioctl path, insmod-probe.
- Tables use D2's `grid-rows`/`grid-columns` styling (register map,
  test architecture).
- All node styles come from `common.d2` so every module, register,
  signal, FSM state, and bus is consistent across the catalog.

**Shared classes in `common.d2` (initial set):**

| Class           | Used for                                                     |
| --------------- | ------------------------------------------------------------ |
| `hw_module`     | SystemVerilog module boxes (blue family)                     |
| `sw_module`     | C-file / kernel-driver boxes (green family)                  |
| `register`      | Avalon-mapped register words                                 |
| `signal`        | Single-bit / multi-bit signals on signal-level diagrams      |
| `bus`           | Multi-wire bundle (Avalon, VGA, DDR)                         |
| `fsm_state`     | Game-loop / kernel-driver FSM states                         |
| `external`      | Off-chip parts (USB keyboard, VGA monitor, SD card, JTAG)    |
| `planned`       | Unused in v1 of catalog (kept for future "planned" overlays) |
| `note`          | Annotation callouts (constants, latency, caveats)            |

## 4. Catalog (36 diagrams)

### Section 1 — System / context (4)

| # | File | What it shows |
|---|------|---------------|
| 1 | `01-system/01_context.d2` | User, USB keyboard, DE1-SoC, VGA monitor, serial-console host — the whole world in one picture. |
| 2 | `01-system/02_hw_sw_boundary.d2` | Two swim lanes: HPS-Linux (userspace + kernel) on left, FPGA fabric on right, bridge in the middle, each named module placed in its lane. |
| 3 | `01-system/03_soc_block.d2` | Cyclone V SoC internals: Cortex-A9 cores, L2, DDR3 controller, HPS peripherals, HPS↔FPGA bridges, FPGA fabric tile. |
| 4 | `01-system/04_memory_map.d2` | Address ranges (DDR3, lightweight HPS-to-FPGA bridge at `0xFF20_0000`), `pvz_top` slave window, per-word offsets (51 × 4 B). |

### Section 2 — Hardware tier (9)

| # | File | What it shows |
|---|------|---------------|
| 5 | `02-hw/01_fpga_top.d2` | Containment tree: `soc_system_top` → qsys-generated `soc_system` → `pvz_top` → submodules. VGA and HPS pin groups as opaque bundles. |
| 6 | `02-hw/02_pvz_top_block.d2` | Inside `pvz_top`: Avalon-MM slave port, 51-word register file, `vga_counters`, `bg_grid`, `sprite_rom`, `entity_drawer`, `color_palette`. Which registers feed which children. |
| 7 | `02-hw/03_vga_counters.d2` | `hcount[10:0]`, `vcount[9:0]` counters, sync windows, blanking, pixel-clock derivation. |
| 8 | `02-hw/04_bg_grid.d2` | (px,py) → cell-index → checker color. Constants `GRID_X=64`, `GRID_Y=112`, `CELL=64`, `GRID_COLS=8`, `GRID_ROWS=4`. |
| 9 | `02-hw/05_sprite_rom.d2` | `sprite_rom` + the three `*_idx.mem` files; address = `row*64+col`; 1-cycle read latency. |
| 10 | `02-hw/06_entity_drawer_pipeline.d2` | 7-layer merge (bg → plant → pea → zombie → cursor → sun HUD → selector); 2-stage pipeline. |
| 11 | `02-hw/07_entity_drawer_signals.d2` ⚙ | **Signal-level hot spot.** All input ports, layer muxes, sprite-ROM address generators, registered final color, with bit-widths. |
| 12 | `02-hw/08_color_palette.d2` | 256-entry LUT, 8-bit index → 24-bit RGB, named palette entries called out. |
| 13 | `02-hw/09_pvz_top_regfile_signals.d2` ⚙ | **Signal-level hot spot.** Avalon `address[5:0]`, `writedata[31:0]`, `write`, `chipselect` → decoder → field-level register updates. Explains absence of vsync latching → one-frame tearing window. |

### Section 3 — Software tier (7)

| # | File | What it shows |
|---|------|---------------|
| 14 | `03-sw/01_process_architecture.d2` | `pvz` userspace binary (main.c, game.c, render.c, input.c), `pvz_driver.ko` in kernel column, `/dev/pvz` and `/dev/input/eventN` as open FDs. |
| 15 | `03-sw/02_kernel_driver.d2` | Module init (`platform_driver_register`, `probe`, `of_iomap`, `misc_register`); ioctl path (`pvz_ioctl(PVZ_WRITE_REG)` → `copy_from_user` → `iowrite32`); cleanup. |
| 16 | `03-sw/03_game_state_er.d2` | ER-style picture of `game_state` from `game.h`: `grid[4][8]`, `zombies[8]`, `peas[8]`, `cursor`, `sun`, `selected`, `frame_count`, `state`. |
| 17 | `03-sw/04_game_loop_fsm.d2` | INIT → PLAYING → (PAUSED?) → WIN/LOSE → exit, with transition conditions from `game.c`. |
| 18 | `03-sw/05_game_update_flow.d2` | Per-tick order inside `game_tick()`: advance zombies, eat-check, advance peas, pea/zombie collision, spawn check, sun accumulator, win/lose check. |
| 19 | `03-sw/06_input_pipeline.d2` | `open("/dev/input/eventN")` → poll loop → decode `struct input_event` → map to {UP, DOWN, LEFT, RIGHT, SPACE, D, TAB, ESC} → game state. |
| 20 | `03-sw/07_render_mapping.d2` | `render.c` mapping: game-state field → FPGA register word. |

### Section 4 — HW/SW interface (4)

| # | File | What it shows |
|---|------|---------------|
| 21 | `04-interface/01_avalon_bus.d2` | HPS → lightweight HPS-to-FPGA bridge → qsys interconnect → `pvz_top` slave. `addressUnits = WORDS` quirk; 6-bit word address. |
| 22 | `04-interface/02_ioctl_path.d2` | Sequence: userland `ioctl(fd, PVZ_WRITE_REG)` → VFS → `pvz_ioctl` → `copy_from_user` → bounds check → `iowrite32(base + word*4)` → Avalon write → register file. `shape: sequence_diagram`. |
| 23 | `04-interface/03_register_map_cheatsheet.d2` | Tall table-style diagram: all 51 words — index, name, bit layout, semantics, SW writer, HW consumer. The reference card. |
| 24 | `04-interface/04_device_tree_binding.d2` | `pvz_top_hw.tcl` → qsys `.sopcinfo` → `sopc2dts` → `.dts` → `dtc` → `.dtb` → kernel reads at boot → driver `of_match_table` matches `csee4840,pvz_gpu-1.0`. |

### Section 5 — Cross-cutting (6)

| # | File | What it shows |
|---|------|---------------|
| 25 | `05-cross-cutting/01_frame_timing.d2` | 16.67 ms (60 Hz) timeline: input poll, game update, register writes (≤51), wait, next tick. Tearing window relative to VGA refresh. |
| 26 | `05-cross-cutting/02_vga_timing.d2` | VGA 640×480@60Hz: 25 MHz pixel clock; hsync/vsync porches; hcount 0–799, vcount 0–524; active vs blanking. |
| 27 | `05-cross-cutting/03_pixel_pipeline_timing.d2` | `entity_drawer` 2-cycle pipeline: cycle 1 issues ROM addresses, cycle 2 merges + registers. Pixel-lead-by-one timing. |
| 28 | `05-cross-cutting/04_build_pipeline.d2` | Three lanes joining at the SD card / on-board install: (a) **HW** on workstation: `make qsys` → `quartus` → `.sof` → `.rbf`; `make dtb` → `.dtb`. (b) **SW native (on-board)**: `make module` → `pvz_driver.ko`; `make pvz`; `make test_*`. (c) **SW cross-compile via GitHub Actions** (`.github/workflows/build.yml`): push → `build-sw` (ARM cross-compile of driver, game, tests via `arm-linux-gnueabihf-gcc` + kernel headers 4.19.0); `test-host` (`test_game` natively); on `v*` tag → `release` job attaches ARM binaries + `deploy.sh` to a GitHub Release. |
| 29 | `05-cross-cutting/05_boot_deploy.d2` | Two layered flows. Cold boot: power on → preloader → U-Boot → `fatload mmc 0:1 soc_system.rbf` → `fpga load 0` → `run bridge_enable_handoff` → Linux + DTB → root FS → login. On-board install/run via `deploy.sh`: `setup` (kernel headers) → `download` (gh release) → `install` (`insmod pvz_driver.ko`) → `run` (`./pvz`) → `test <name>` / `status`. `shape: sequence_diagram`. |
| 30 | `05-cross-cutting/06_test_architecture.d2` | Three surfaces: ModelSim TBs (HW), on-board `test_shapes` / `test_input` (HW+driver), host-side `test_game` (game.c only). |

### Section 6 — Cross-layer flow (6)

| # | File | What it shows |
|---|------|---------------|
| 31 | `06-flow/01_one_frame.d2` | End-to-end: keyboard event → main loop → `input_get_action` → `game_tick` → `render_frame` → ioctl×N → register file → next VGA scanout → entity drawer → palette → monitor pixel. |
| 32 | `06-flow/02_place_plant.d2` | TAB → SELECTED toggles → SPACE → cost check (`sun ≥ 50`) → grid update (PLANTS/SUNFLOWERS bit) → `sun -= 50` → HUD redraw next frame. |
| 33 | `06-flow/03_pea_zombie_collision.d2` | Pea advance → bbox vs zombie bbox → `zombie.hp--` → if 0 clear `alive` bit → pea cleared. |
| 34 | `06-flow/04_zombie_eats_plant.d2` | Zombie enters plant cell → stops, enters eating mode → eat timer → plant HP→0 → bit cleared → zombie resumes. |
| 35 | `06-flow/05_sun_economy.d2` | Frame counter → every 480 frames → `sun += 25` → SUN register write → HUD draws another tile next frame. |
| 36 | `06-flow/06_insmod_probe.d2` | `insmod pvz_driver.ko` → init → `platform_driver_register` → kernel walks DT → finds `csee4840,pvz_gpu-1.0` → `probe` → `of_iomap` → `misc_register` → `/dev/pvz` appears. `shape: sequence_diagram`. |

## 5. Rendering and Makefile

`v5-cursor-controller/doc/diagrams/Makefile` shells out to `d2` for
each `.d2` under the section folders. Targets:

- `make all` — render every `.d2` to a sibling `.svg` and `.png`.
- `make clean` — remove all generated `.svg` and `.png` (keep sources).
- `make <NN-section>` — render only that section.
- `make watch` (optional) — D2 watch mode if useful.

Prerequisites: `d2` binary in `PATH`. Document this in
`v5-cursor-controller/doc/diagrams/README.md`.

## 6. Acceptance criteria

The catalog is complete when:

1. All 36 `.d2` source files exist under
   `v5-cursor-controller/doc/diagrams/` in the section folders above,
   each opening with the required header comment.
2. `common.d2` defines the shared classes listed in §3 and is imported
   by every diagram.
3. `make all` renders every `.d2` to a sibling `.svg` *and* `.png`,
   with no D2 syntax errors.
4. Each diagram is factually grounded in the actual code of
   `v5-cursor-controller/` — every named module, register, signal,
   function, or constant matches the source. Hot-spot signal-level
   diagrams agree with the bit-widths in the `.sv` files.
5. `v5-cursor-controller/doc/diagrams/README.md` lists every diagram
   with a one-line description and a pointer to the source files it
   documents.

## 7. Out of scope (explicit)

- Diagramming `main/` (Milestone 1 primitive-shape version).
- Diagramming planned-but-unimplemented features: USB gamepad, audio
  (Wolfson codec), additional plant types, levels beyond level 1.
- Diagrams of rejected / alternative designs.
- Animating the diagrams; producing slide decks; embedding into the
  LaTeX design document (a follow-up step, not part of this spec).
- A dedicated diagram for `worktree.sh` (developer workflow helper,
  not part of the system under test). Mentioned in a footnote on
  diagram 28 if space allows.

## 8. Risks / open assumptions

- **D2 availability:** assumes `d2` is installed on the workstation
  used to render. Mitigated by Makefile printing a helpful error if
  it is missing.
- **README staleness:** the existing `v5-cursor-controller/README.md`
  still references `shape_renderer.sv` and `linebuffer.sv`, which are
  absent in the actual `hw/` directory. Diagrams will track the
  actual code, not the stale README. (Fixing the README is out of
  scope here.)
- **Sub-folder count:** 36 diagrams is intentional ("comprehensive").
  If a subset is judged too verbose during implementation, the
  recommended cuts are: 27 (`pixel_pipeline_timing` — content overlaps
  with diagram 11) and 17 (`game_loop_fsm` — collapses to a small
  state machine that the prof may consider trivial). Both kept by
  default for completeness.
