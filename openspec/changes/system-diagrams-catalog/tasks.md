## 0. Conventions for every task

- **Dispatching:** Tasks marked **[dispatch]** spawn a subagent (use `Agent` with `subagent_type: general-purpose`) to keep the main session's context clean. Each dispatch task lists every diagram the subagent is responsible for. The subagent reads the design spec at `docs/superpowers/specs/2026-05-12-system-diagrams-design.md`, the implementation plan at `docs/superpowers/plans/2026-05-12-system-diagrams.md`, and the cited source files in `hw/` and `sw/` before drawing. The subagent writes the `.d2`, renders `.svg`+`.png`, commits, and returns a short summary.
- **Per-diagram cadence inside a dispatch:** write `.d2` → `d2 --layout=elk doc/diagrams/<sec>/<file>.d2 doc/diagrams/<sec>/<file>.svg` (fall back to dagre if ELK warns) → repeat for `.png` → `git add` the three files → `git -c commit.gpgsign=false commit -m "docs(diagrams): add <short-title>"`.
- **Commit messages: concise, conventional-commit style (`docs(diagrams): add <short-title>`). No `Co-Authored-By` line. No mention of Claude or any AI assistant. No emojis. One diagram per commit.**
- **Header comment required** in every `.d2`: `# Title:`, `# Section:`, `# Documents:`, `# Defends:`, `# Detail:`. First non-comment line is `...@../common.d2`.
- **Don't mass-update `README.md` inside dispatches** — that is finalization Task 9.1, done once at the end to avoid merge churn between parallel dispatches.

## 1. Scaffolding (main agent, sequential — no dispatch)

- [x] 1.1 Verify `d2 --version` reports v0.7.1+ and create the section folder tree under `doc/diagrams/` (`01-system/`, `02-hw/`, `03-sw/`, `04-interface/`, `05-cross-cutting/`, `06-flow/`).
- [x] 1.2 Write `doc/diagrams/common.d2` defining the shared `classes:` block (`hw_module`, `sw_module`, `kernel_module`, `register`, `signal`, `bus`, `fsm_state`, `external`, `note`, `layer`) and a comment documenting the `...@file` import gotcha for D2 0.7.1.
- [x] 1.3 Write `doc/diagrams/Makefile` with phony targets `all`, `clean`, and one per-section target. `all` must render every `.d2` under section folders to sibling `.svg` and `.png` via `d2 --layout=elk` (falling back to dagre on warning); skip `common.d2`. `clean` removes generated `.svg`/`.png` only. Missing-`d2` error MUST be helpful.
- [x] 1.4 Write `doc/diagrams/README.md` skeleton: prerequisites (`d2` v0.7.1+), folder layout, section legend, and a placeholder index that will be filled in by Task 9.1. Commit scaffolding as a single commit: `docs(diagrams): add common.d2, Makefile, README scaffold`.

## 2. [dispatch] Hero diagrams (single subagent, sequential within — establishes catalog style)

Run this dispatch **first and alone**; review the four resulting commits to lock down visual style before fanning out parallel dispatches.

- [x] 2.1 Dispatch one subagent to produce the four hero diagrams in this order, one commit per diagram:
  - `01-system/01_context.d2` — user + USB keyboard + DE1-SoC + VGA monitor + serial-console host.
  - `02-hw/02_pvz_top_block.d2` — inside `pvz_top`: Avalon slave port, 51-word register file, `vga_counters`, `bg_grid`, `sprite_rom`, `entity_drawer`, `color_palette`; which registers feed which children.
  - `04-interface/03_register_map_cheatsheet.d2` — tall grid-rows table of all 51 words (index, name, bit layout, semantics, SW writer, HW consumer). Annotate the `address < 6'd40` aliasing gotcha citing `hw/pvz_top.sv:122`.
  - `06-flow/01_one_frame.d2` — end-to-end frame: keyboard → `process_input(&gs)` → `game_update(&gs)` → `render_frame(&gs)` → ioctl×N → register file → next VGA scanout → entity drawer → palette → monitor pixel. Cite `sw/main.c:114,119,122`.

## 3. [dispatch] Parallel section batches (six subagents, one per coherent section group)

After hero review, dispatch tasks 3.1–3.6 **in parallel** (single message with multiple `Agent` calls). Each subagent handles a coherent group of diagrams sharing source-file context; per-diagram commits keep the history granular.

- [x] 3.1 Dispatch — **Section 1 remaining** (3 diagrams):
  - `01-system/02_hw_sw_boundary.d2` — two swim lanes (HPS-Linux userspace + kernel on left, FPGA fabric on right, bridge in middle), each named module placed in its lane.
  - `01-system/03_soc_block.d2` — Cyclone V SoC internals: Cortex-A9 cores, L2, DDR3 controller, HPS peripherals, HPS↔FPGA bridges, FPGA fabric tile.
  - `01-system/04_memory_map.d2` — address ranges: DDR3, lightweight HPS-to-FPGA bridge at `0xFF20_0000`, `pvz_top` slave window, per-word offsets (51 × 4 B).

- [x] 3.2 Dispatch — **Section 2 block-level** (6 diagrams; covers `02-hw/` except hero `02_pvz_top_block` and the two signal-level hot spots `07` and `09`):
  - `02-hw/01_fpga_top.d2` — containment tree: `soc_system_top` → `soc_system` → `pvz_top` → submodules; VGA + HPS pin groups as opaque bundles.
  - `02-hw/03_vga_counters.d2` — `hcount[10:0]`, `vcount[9:0]`, sync windows, blanking, pixel-clock derivation.
  - `02-hw/04_bg_grid.d2` — (px,py) → cell-index → checker color; `GRID_X=64`, `GRID_Y=112`, `CELL=64`, `GRID_COLS=8`, `GRID_ROWS=4`.
  - `02-hw/05_sprite_rom.d2` — `sprite_rom` + `peashooter_idx.mem`, `sunflower_idx.mem`, `zombie_idx.mem`; address = `row*64+col`; 1-cycle read latency.
  - `02-hw/06_entity_drawer_pipeline.d2` — 7-layer merge (bg → plant → pea → zombie → cursor → sun HUD → selector); 2-stage pipeline; `color_out` is combinational (`hw/entity_drawer.sv:330`).
  - `02-hw/08_color_palette.d2` — 256-entry LUT, 8-bit index → 24-bit RGB; named palette entries called out.

- [x] 3.3 Dispatch — **Section 2 signal-level hot spots** (2 diagrams ⚙ — separate dispatch because qualitatively different work: deep source reading + bit-width verification):
  - `02-hw/07_entity_drawer_signals.d2` ⚙ — all input ports, layer muxes, sprite-ROM address generators, final color signal, every bit-width consistent with `hw/entity_drawer.sv`.
  - `02-hw/09_pvz_top_regfile_signals.d2` ⚙ — Avalon `address[5:0]`, `writedata[31:0]`, `write`, `chipselect` → decoder → field-level register updates. Annotate (a) absence of vsync latching → one-frame tearing window, and (b) `address < 6'd40` aliasing (words 2..31 → zombie slot) citing `hw/pvz_top.sv:122`.

- [x] 3.4 Dispatch — **Section 3 software** (7 diagrams):
  - `03-sw/01_process_architecture.d2` — `pvz` userspace binary (main.c, game.c, render.c, input.c), `pvz_driver.ko` in kernel column, `/dev/pvz` and `/dev/input/eventN` as open FDs.
  - `03-sw/02_kernel_driver.d2` — `pvz_init` → `platform_driver_probe(&pvz_driver, pvz_probe)` (`sw/pvz_driver.c:130`); probe order `misc_register` → `of_address_to_resource` → `request_mem_region` → `of_iomap` (`sw/pvz_driver.c:61`); ioctl path → `iowrite32(virtbase + word*4)`; `pvz_remove`.
  - `03-sw/03_game_state_er.d2` — ER picture of `game_state` from `game.h`.
  - `03-sw/04_game_loop_fsm.d2` — INIT → PLAYING → (PAUSED?) → WIN/LOSE → exit with transitions from `game.c`.
  - `03-sw/05_game_update_flow.d2` — phases inside `game_update()` (`sw/game.c:288` entry, order at `:295-301`): sun → spawning → firing → projectiles → zombies → collisions → win. Note that input is handled separately by `process_input` in `sw/main.c`.
  - `03-sw/06_input_pipeline.d2` — `open("/dev/input/eventN")` → poll loop → decode `struct input_event` → map to {UP, DOWN, LEFT, RIGHT, SPACE, D, TAB, ESC} → game state.
  - `03-sw/07_render_mapping.d2` — `render.c` mapping table: game-state field → FPGA register word.

- [x] 3.5 Dispatch — **Section 4 remaining** (3 diagrams; the cheat sheet 03 already landed as a hero):
  - `04-interface/01_avalon_bus.d2` — HPS → lightweight HPS-to-FPGA bridge → qsys interconnect → `pvz_top` slave; call out the `addressUnits = WORDS` quirk and the 6-bit word address.
  - `04-interface/02_ioctl_path.d2` — `shape: sequence_diagram` of userland `ioctl(fd, PVZ_WRITE_REG)` → VFS → `pvz_ioctl` → `copy_from_user` → bounds check → `iowrite32(base + word*4)` → Avalon write → register file.
  - `04-interface/04_device_tree_binding.d2` — `pvz_top_hw.tcl` → qsys `.sopcinfo` → `sopc2dts` → `.dts` → `dtc` → `.dtb` → kernel reads at boot → driver `of_match_table` matches `csee4840,pvz_gpu-1.0`.

- [x] 3.6 Dispatch — **Section 5 cross-cutting** (5 diagrams):
  - `05-cross-cutting/01_frame_timing.d2` — 16.67 ms timeline: `process_input`, `game_update`, `render_frame` register writes (≤51), `usleep` to next tick; tearing window vs VGA refresh.
  - `05-cross-cutting/02_vga_timing.d2` — VGA 640×480@60Hz: 25 MHz pixel clock; hsync/vsync porches; hcount 0–799, vcount 0–524; active vs blanking.
  - `05-cross-cutting/03_pixel_pipeline_timing.d2` — `entity_drawer` 2-cycle pipeline; cycle 1 issues ROM addresses, cycle 2 merges via combinational `always_comb` mux; 1-cycle latency lives in stage-1 hit-signal flops + sprite ROM read; final `color_out` is combinational.
  - `05-cross-cutting/04_build_pipeline.d2` — two lanes: (a) HW workstation: `make qsys` → `quartus` → `.sof` → `.rbf`; `make dtb` → `.dtb`. (b) SW (native or cross-compile via `CC=arm-linux-gnueabihf-gcc` + `ARCH`/`CROSS_COMPILE`/`KERNEL_SOURCE`): `make module`, `make pvz`. **NO CI lane.**
  - `05-cross-cutting/05_boot_flow.d2` — `shape: sequence_diagram`. Cold boot: power on → preloader → U-Boot → `fatload mmc 0:1 soc_system.rbf` → `fpga load 0` → `run bridge_enable_handoff` → Linux + DTB → root FS → manual `insmod pvz_driver.ko` → `./pvz`. **NO `deploy.sh` lane.**

- [x] 3.7 Dispatch — **Section 6 remaining** (5 diagrams; one-frame flow 01 already landed as a hero):
  - `06-flow/02_place_plant.d2` — TAB → SELECTED toggles → SPACE → cost check (`sun ≥ 50`) → grid update (PLANTS/SUNFLOWERS bit) → `sun -= 50` → HUD redraws next frame.
  - `06-flow/03_pea_zombie_collision.d2` — pea advance → bbox vs zombie bbox → `zombie.hp--` → if 0 clear `alive` bit → pea cleared.
  - `06-flow/04_zombie_eats_plant.d2` — zombie enters plant cell → stops, enters eating mode → eat timer → plant HP→0 → bit cleared → zombie resumes.
  - `06-flow/05_sun_economy.d2` — frame counter → every 480 frames → `sun += 25` → SUN register write → HUD draws another tile next frame.
  - `06-flow/06_insmod_probe.d2` — `shape: sequence_diagram`. `insmod` → `pvz_init` → `platform_driver_probe(&pvz_driver, pvz_probe)` → kernel walks DT → finds `csee4840,pvz_gpu-1.0` → `pvz_probe`: `misc_register` → `of_address_to_resource` → `request_mem_region` → `of_iomap` → `/dev/pvz` ready.

## 4. Finalization (main agent)

- [x] 4.1 Update `doc/diagrams/README.md` index: every `.d2` listed with one-line description and `# Documents:` source-file pointers, organized by section in the same order as the section folders. One commit: `docs(diagrams): populate README index`.
- [x] 4.2 Run `make clean && make all` from `doc/diagrams/`; confirm exit 0 and that every `.d2` has sibling `.svg` and `.png`. Spot-check 3 hero diagrams open and render correctly in a browser/image viewer.
- [x] 4.3 Audit: `grep -rE 'shape_renderer|linebuffer\.sv|test_architecture|deploy\.sh|wolfson|usb_gamepad' doc/diagrams/` returns no matches (out-of-scope content check). Header-comment check: every diagram has all five required header fields with no placeholder text. Commit-message check: `git log --grep='Co-Authored-By' -- doc/diagrams/` returns nothing.
- [x] 4.4 Final per-diagram source-grounding spot check: for each ⚙ signal-level diagram and the four heroes, open the cited source file(s) and confirm names, bit-widths, and line numbers still match. Fix and amend (or follow-up commit) on any drift.
