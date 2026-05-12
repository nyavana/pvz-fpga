# PvZ-on-DE1-SoC — System Diagram Catalog

This directory holds the D2 source for every system diagram referenced during
the final design review for `v5-cursor-controller/`. Every `.d2` file has a
sibling `.svg` and `.png` checked into git so the catalog is viewable without
a local D2 install.

## Prerequisites

- **Reading the diagrams:** none. The committed `.svg` and `.png` files render
  in any browser or image viewer.
- **Rebuilding the diagrams:** [`d2`](https://d2lang.com) v0.7.1 or newer on
  `PATH`. The Makefile is pinned to D2 0.7.1 syntax (notably the unprefixed
  `...@../common.d2` import spread). GNU `make` is required.

## Folder layout

```
doc/diagrams/
  README.md         this file — index of every diagram
  common.d2         shared D2 classes (colors and shapes per role)
  Makefile          `make all` renders every .d2 to sibling .svg + .png
  01-system/        system / context overview
  02-hw/            FPGA module diagrams
  03-sw/            HPS userspace + kernel diagrams
  04-interface/     HW <-> SW boundary
  05-cross-cutting/ timing, build, boot
  06-flow/          end-to-end scenario flows
```

Filenames inside each section follow the pattern `NN_short-name.d2` where
`NN` is a two-digit decimal prefix that determines presentation order.

## Section legend

| Section            | What it answers                                                                            |
| ------------------ | ------------------------------------------------------------------------------------------ |
| `01-system/`       | "What does the system look like from the outside?" Context, swim lanes, SoC, memory map.   |
| `02-hw/`           | "What does the FPGA do?" Containment, VGA timing, sprite ROMs, entity drawer, palette.     |
| `03-sw/`           | "What does the software do?" Process layout, kernel driver, game state, update phases.     |
| `04-interface/`    | "How do they talk?" Avalon bus, ioctl path, register map cheat sheet, DT binding.          |
| `05-cross-cutting/`| "What about timing and the build?" Frame timing, VGA timing, build pipeline, boot.         |
| `06-flow/`         | "Walk me through scenario X." One-frame, place-plant, collision, eat-plant, sun, insmod.   |

Each `.d2` opens with a five-line header comment block:

```
# Title:     <short title>
# Section:   <01-system | 02-hw | …>
# Documents: <source files this diagram is grounded in>
# Defends:   <likely review question(s) this diagram answers>
# Detail:    <block | signal>
```

`# Documents:` is the source-truth pointer. To re-audit a diagram against
the code, open the files it lists.

Color and shape conventions live in `common.d2`. Every other `.d2` imports
it via `...@../common.d2` as its first non-comment statement.

## Rendering

```sh
make            # render everything in this catalog
make clean      # remove .svg / .png; leave .d2 alone
make 02-hw      # render only one section
```

If `d2` is not on `PATH` the Makefile prints a useful error and exits non-zero
rather than producing an opaque shell error.

## Diagram index

35 diagrams, organized by section. Hot spots marked ⚙ are signal-level (with bit-widths).

### 01-system — system / context (4)

- `01-system/01_context.d2` — **System context.** User, USB keyboard, DE1-SoC, VGA monitor, serial host PC in one picture.
  Sources: `sw/main.c`, `sw/pvz_driver.c`, `sw/input.c`, `hw/pvz_top.sv`, `hw/soc_system_top.sv`.
- `01-system/02_hw_sw_boundary.d2` — **HW/SW boundary swim lanes.** HPS userspace + kernel on one side, FPGA fabric on the other, lightweight bridge in the middle.
  Sources: `sw/main.c`, `sw/game.c`, `sw/render.c`, `sw/input.c`, `sw/pvz_driver.c`, `hw/pvz_top.sv`, `hw/soc_system_top.sv`, plus the other `hw/*.sv` modules.
- `01-system/03_soc_block.d2` — **Cyclone V SoC block diagram.** Cortex-A9 cores, L2, DDR3 controller, HPS peripherals, HPS↔FPGA bridges, FPGA fabric tile.
  Sources: `hw/soc_system_top.sv`, `hw/soc_system.qsys`, `hw/pvz_top.sv`, `sw/pvz_driver.c`.
- `01-system/04_memory_map.d2` — **HPS memory map and `pvz_top` register window.** Address ranges, lightweight bridge at `0xFF20_0000`, per-word offsets.
  Sources: `hw/pvz_top.sv`, `hw/soc_system.qsys`, `sw/pvz_driver.c`.

### 02-hw — FPGA tier (9)

- `02-hw/01_fpga_top.d2` — **FPGA top-level containment tree.** `soc_system_top` → `soc_system` → `pvz_top` → submodules.
  Sources: `hw/soc_system_top.sv`, `hw/soc_system.qsys`, `hw/pvz_top.sv`.
- `02-hw/02_pvz_top_block.d2` — **`pvz_top` block diagram.** 51-word register file plus children, with the register-to-child fanout.
  Sources: `hw/pvz_top.sv`, `hw/entity_drawer.sv`, `hw/bg_grid.sv`, `hw/sprite_rom.sv`, `hw/vga_counters.sv`, `hw/color_palette.sv`.
- `02-hw/03_vga_counters.d2` — **`vga_counters` pixel-clock + sync generator.** `hcount[10:0]`, `vcount[9:0]`, sync/porch windows.
  Sources: `hw/vga_counters.sv`, `hw/pvz_top.sv`.
- `02-hw/04_bg_grid.d2` — **`bg_grid` lawn checker.** `(px,py)` → cell index → checker color; layout constants.
  Sources: `hw/bg_grid.sv`, `hw/color_palette.sv`, `hw/pvz_top.sv`.
- `02-hw/05_sprite_rom.d2` — **`sprite_rom` 64×64 byte ROM, 1-cycle read.** Shared module instantiated for plant/sunflower/zombie.
  Sources: `hw/sprite_rom.sv`, `hw/pvz_top.sv`, `hw/entity_drawer.sv`, `hw/peashooter_idx.mem`.
- `02-hw/06_entity_drawer_pipeline.d2` — **`entity_drawer` 2-stage pipeline.** 7-layer merge order, where the 1-cycle latency really lives.
  Sources: `hw/entity_drawer.sv`, `hw/sprite_rom.sv`, `hw/pvz_top.sv`.
- `02-hw/07_entity_drawer_signals.d2` ⚙ — **`entity_drawer` datapath signal-level.** Every port, every internal signal, every bit-width.
  Sources: `hw/entity_drawer.sv`, `hw/sprite_rom.sv`, `hw/bg_grid.sv`.
- `02-hw/08_color_palette.d2` — **`color_palette` 8-bit index → 24-bit RGB.** Named palette entries used by the drawer.
  Sources: `hw/color_palette.sv`, `hw/entity_drawer.sv`, `hw/pvz_top.sv`.
- `02-hw/09_pvz_top_regfile_signals.d2` ⚙ — **`pvz_top` Avalon register-file decoder signal-level.** Includes the `address < 6'd40` aliasing gotcha and the no-vsync-latching note.
  Sources: `hw/pvz_top.sv`, `hw/entity_drawer.sv`.

### 03-sw — HPS software (7)

- `03-sw/01_process_architecture.d2` — **Process architecture.** Userspace `pvz` + kernel `pvz_driver.ko` with `/dev/pvz` and `/dev/input/eventN`.
  Sources: `sw/main.c`, `sw/game.c`, `sw/render.c`, `sw/input.c`, `sw/pvz_driver.c`, `sw/pvz.h`.
- `03-sw/02_kernel_driver.d2` — **`pvz_driver.ko` lifecycle.** `pvz_init` → `platform_driver_probe` → `pvz_probe`: `misc_register` → `of_address_to_resource` → `request_mem_region` → `of_iomap`; ioctl path; exit.
  Sources: `sw/pvz_driver.c`, `sw/pvz_driver.h`, `sw/pvz.h`.
- `03-sw/03_game_state_er.d2` — **`game_state` entity-relationship snapshot.** Grid + zombies + projectiles + cursor + sun.
  Sources: `sw/game.h`, `sw/game.c`, `sw/render.c`, `sw/pvz.h`.
- `03-sw/04_game_loop_fsm.d2` — **Game loop FSM.** `STATE_PLAYING` ↔ `WIN`/`LOSE` ↔ exit.
  Sources: `sw/main.c`, `sw/game.c`, `sw/game.h`.
- `03-sw/05_game_update_flow.d2` — **`game_update()` per-tick phase order.** sun → spawning → firing → projectiles → zombies → collisions → win.
  Sources: `sw/game.c`, `sw/game.h`, `sw/main.c`.
- `03-sw/06_input_pipeline.d2` — **Input pipeline.** `/dev/input/eventN` → `input_poll` → key map → game intents.
  Sources: `sw/input.c`, `sw/input.h`, `sw/main.c`.
- `03-sw/07_render_mapping.d2` — **`render.c` mapping table.** Each game-state field to its Avalon register word.
  Sources: `sw/render.c`, `sw/render.h`, `sw/pvz.h`, `sw/game.h`.

### 04-interface — HW/SW boundary (4)

- `04-interface/01_avalon_bus.d2` — **Avalon-MM path.** HPS → lightweight HPS-to-FPGA bridge → qsys interconnect → `pvz_top` slave; `addressUnits = WORDS` quirk.
  Sources: `hw/pvz_top.sv`, `hw/pvz_top_hw.tcl`, `hw/soc_system.qsys`, `sw/pvz_driver.c`.
- `04-interface/02_ioctl_path.d2` — **`ioctl(PVZ_WRITE_REG)` sequence.** Userland to kernel to `iowrite32` to Avalon.
  Sources: `sw/render.c`, `sw/pvz.h`, `sw/pvz_driver.c`, `hw/pvz_top.sv`.
- `04-interface/03_register_map_cheatsheet.d2` — **Avalon register map (51 words).** Reference card: index, name, bit layout, SW writer, HW consumer; aliasing gotcha annotated.
  Sources: `hw/pvz_top.sv`, `sw/pvz.h`, `sw/render.c`, `sw/pvz_driver.c`.
- `04-interface/04_device_tree_binding.d2` — **Device-tree binding flow.** `_hw.tcl` → `.sopcinfo` → `sopc2dts` → `.dts` → `dtc` → `.dtb` → driver `of_match_table`.
  Sources: `hw/pvz_top_hw.tcl`, `hw/soc_system.qsys`, `sw/pvz_driver.c`.

### 05-cross-cutting — timing / build / boot (5)

- `05-cross-cutting/01_frame_timing.d2` — **Per-frame software timing.** 16.67 ms tick: `process_input` / `game_update` / `render_frame` / `usleep`; tearing window vs VGA refresh.
  Sources: `sw/main.c`, `sw/game.c`, `sw/render.c`, `hw/vga_counters.sv`.
- `05-cross-cutting/02_vga_timing.d2` — **VGA 640×480 @ 60 Hz.** 25 MHz pixel clock, hsync/vsync porches, active vs blanking.
  Sources: `hw/vga_counters.sv`, `hw/pvz_top.sv`.
- `05-cross-cutting/03_pixel_pipeline_timing.d2` — **`entity_drawer` 2-cycle pipeline timing.** Stage 1 issues addresses, stage 2 merges via combinational `always_comb`.
  Sources: `hw/entity_drawer.sv`, `hw/sprite_rom.sv`, `hw/pvz_top.sv`.
- `05-cross-cutting/04_build_pipeline.d2` — **HW and SW build lanes.** Quartus + qsys on the workstation; on-board native or ARM cross-compile for SW.
  Sources: `hw/Makefile`, `hw/soc_system.qsys`, `sw/Makefile`, `CLAUDE.md`.
- `05-cross-cutting/05_boot_flow.d2` — **Cold boot.** Preloader → U-Boot → `fpga load` → bridge handoff → Linux → manual `insmod` → `./pvz`.
  Sources: `hw/pvz_top_hw.tcl`, `sw/pvz_driver.c`, `sw/main.c`, `hw/Makefile`.

### 06-flow — cross-layer scenarios (6)

- `06-flow/01_one_frame.d2` — **One frame, end-to-end.** Keyboard → main loop → ioctl×N → register file → VGA scanout → monitor.
  Sources: `sw/main.c`, `sw/game.c`, `sw/render.c`, `sw/pvz_driver.c`, `hw/pvz_top.sv`, `hw/entity_drawer.sv`, `hw/vga_counters.sv`, `hw/color_palette.sv`.
- `06-flow/02_place_plant.d2` — **Place-plant flow.** TAB toggles, SPACE places; cost check, grid update, HUD redraw.
  Sources: `sw/main.c`, `sw/game.c`, `sw/game.h`, `sw/render.c`, `hw/pvz_top.sv`, `hw/entity_drawer.sv`.
- `06-flow/03_pea_zombie_collision.d2` — **Pea-zombie collision.** Pea advance, bbox test, HP decrement, pea cleared.
  Sources: `sw/game.c`, `sw/game.h`, `sw/render.c`, `hw/pvz_top.sv`, `hw/entity_drawer.sv`.
- `06-flow/04_zombie_eats_plant.d2` — **Zombie eats plant.** Move → eat mode → eat timer → plant HP→0 → zombie resumes.
  Sources: `sw/game.c`, `sw/game.h`, `hw/pvz_top.sv`, `hw/entity_drawer.sv`.
- `06-flow/05_sun_economy.d2` — **Sun economy.** Frame counter → `sun += 25 × (1 + sunflowers)` every 480 frames → HUD update.
  Sources: `sw/game.c`, `sw/game.h`, `sw/render.c`, `sw/pvz.h`, `hw/pvz_top.sv`, `hw/entity_drawer.sv`.
- `06-flow/06_insmod_probe.d2` — **`insmod pvz_driver.ko` probe sequence.** Module loader → `platform_driver_probe` → DT match → `pvz_probe` registers `/dev/pvz`.
  Sources: `sw/pvz_driver.c`, `sw/pvz_driver.h`, `sw/pvz.h`.
