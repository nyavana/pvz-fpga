# 09 — References, file index, and glossary

## External reference designs in this repo

### `doc/reference-design/lab3/`

The CSEE4840 lab 3 skeleton — a minimal VGA peripheral
(`vga_ball.sv`) plus a Linux kernel module that drives it
(`vga_ball.c`). Our project descends from this design and inherits
several patterns:

- The VGA timing block (`hw/vga_counters.sv`) is a near-verbatim copy
  of `lab3/hw/vga_ball.sv` lines 70-ish onward. Same parameters, same
  sync expression.
- The board-level wrapper (`hw/soc_system_top.sv`) follows lab 3's
  pin-out pattern.
- The kernel driver pattern (`misc_register` + `of_iomap` +
  `iowrite32` inside an ioctl) comes straight from
  `lab3/sw/vga_ball.c`.

If you want to understand *why* `hw/vga_counters.sv` looks the way
it does, read the lab 3 writeup at
`doc/reference-design/lab3/description/description.md`.

One thing **we deliberately do not inherit** from lab 3: lab 3
latches its shadow registers to active registers on vsync to avoid
tearing. We don't — `hw/pvz_top.sv:25-27` accepts the tearing in
exchange for a simpler register file. If you add high-rate per-pixel
animation later, copy lab 3's shadow/active pattern.

### `doc/reference-design/bubble-bobble/`

A prior team's full CSEE4840 final project (Bubble Bobble, May
2024). It is several times the size of our project and has the
features we don't: linebuffer-based sprite engine, tile ROM,
multi-track audio with the WM8731 codec, libusb gamepad polling.

What is useful in it right now:

- **`bubble-bobble/report.md` §6 ("gotchas")**. Three hard-won
  lessons: (a) Avalon `addressUnits` confusion, exactly the same
  byte-vs-word issue we hit in `pvz_top.sv:22-24`; (b) M10K BRAM
  read latency requiring a registered output and a matching pipeline
  stage — which is why our `entity_drawer.sv` has the two-stage
  structure; (c) shadow/active register latching at vsync. Read
  this section if you start changing anything in the data path.
- **`bubble-bobble/report.md` §2.1 (linebuffer design)**. If we ever
  outgrow racing-the-beam (e.g. too many overlapping sprites in
  stage 1), this is the upgrade path. Their approach: while line N
  is displayed from buffer A, draw line N+1 into buffer B; swap on
  hsync.
- **`bubble-bobble/code/`**. Working SystemVerilog, kernel module,
  and userspace examples to crib from when adding audio or USB
  features.

## Documents elsewhere in `doc/`

Read these for context, but be aware that none of them currently
describes the code on this branch:

- **`doc/proposal/proposal.md`** — the original project pitch. Has
  the game rules and design intent. Talks about a 5 × 9 grid and
  audio over the Wolfson codec, neither of which is implemented on
  this branch (the grid here is 4 × 8 and there is no audio).
- **`doc/milestone1.md`** — describes a *shape-table* hardware
  engine that was the design at the time of milestone 1. That
  engine has since been replaced by the sprite-based engine
  documented in this guide. Treat `milestone1.md` as historical.
- **`doc/mvp_design_plan.md`** — early implementation plan,
  superseded.
- **`doc/bugfix-display-collision.md`** — post-MVP bugfix notes
  (April 2026) for the old shape-table engine. Same caveat.
- **`doc/v2-changes.md`** — introduces the first sprite-engine
  prototype (peashooter only). The current branch evolves that
  prototype.
- **`doc/manual/DE1-SoC_User_manual.md`** — official Terasic board
  manual. Authoritative for pin assignments, voltage levels, and
  peripheral specs.
- **`doc/design-document/`** — LaTeX source and PDF of the formal
  course design document.

## Top-level repo files

- **`CLAUDE.md`** — instructions for AI agents working in the repo.
  Mentions `deploy.sh`, `worktree.sh`, CI workflow, and on-board
  test programs — none of which exist on this branch. Treat the
  layout and architecture sections as accurate, the deployment
  workflow sections as aspirational.
- **`README.md`** — top-level summary. Similar caveat about
  aspirational tooling.
- **`openspec/`** — experimental task-tracking tool's configuration
  directory. Not load-bearing for any build.

## File index

### `hw/` (FPGA design)

| file | chapter | one-liner |
|------|---------|-----------|
| `hw/pvz_top.sv` | 04 | Avalon-MM peripheral; register file; instantiates everything else |
| `hw/entity_drawer.sv` | 04 | Two-stage compositor; "races the beam" to produce final pixel color |
| `hw/vga_counters.sv` | 04 | 640×480 @ 60 Hz timing generator (50 MHz in, 25 MHz pixel out) |
| `hw/bg_grid.sv` | 04 | Combinational lawn checker background |
| `hw/sprite_rom.sv` | 04 | Parameterized 64×64 palette-indexed sprite ROM, 1-cycle read |
| `hw/color_palette.sv` | 04 | 256-entry palette LUT (8-bit index → 24-bit RGB) |
| `hw/peashooter_idx.mem` | 04 | Peashooter sprite art, 4096 bytes |
| `hw/sunflower_idx.mem` | 04 | Sunflower sprite art, 4096 bytes |
| `hw/zombie_idx.mem` | 04 | Zombie sprite art, 4096 bytes |
| `hw/peas_idx.mem` | 04 | (unused on this branch; peas are procedural) |
| `hw/pvz_top_hw.tcl` | 04 | Platform Designer component descriptor; sets the `compatible` string |
| `hw/soc_system.qsys` | 04 | Platform Designer system: clk + HPS + pvz_top |
| `hw/soc_system_top.sv` | 04 | Board pin assignments and `soc_system` instantiation |
| `hw/Makefile` | 04, 08 | Quartus / Qsys / dtc build flow |

### `sw/` (HPS software)

| file | chapter | one-liner |
|------|---------|-----------|
| `sw/pvz.h` | 05, 06 | Shared header; register map, ioctl ABI, packing helpers |
| `sw/pvz_driver.h` | 05 | Driver-private state |
| `sw/pvz_driver.c` | 05 | Kernel module; `/dev/pvz` misc device; `PVZ_WRITE_REG` ioctl |
| `sw/game.h` | 05 | Game state types and tunable constants |
| `sw/game.c` | 05 | The simulation: grid, zombies, peas, sun economy, win/lose |
| `sw/input.h` | 05 | Input API (`INPUT_*` action codes) |
| `sw/input.c` | 05 | evdev polling; keyboard + Xbox 360 button mapping |
| `sw/render.h` | 05 | Renderer API |
| `sw/render.c` | 05 | `game_state_t` → 50 ioctl writes per frame |
| `sw/main.c` | 05 | 60 Hz loop; input → update → render → sleep |
| `sw/Makefile` | 05, 08 | kbuild for the module; gcc for the userspace binary |

### `doc/guide/` (this guide)

| file | what it covers |
|------|----------------|
| `doc/guide/README.md` | Navigation / index |
| `doc/guide/01-introduction.md` | Project scope, gameplay, what's on this branch |
| `doc/guide/02-background.md` | DE1-SoC, FPGA, Linux on ARM, Avalon-MM, VGA, devicetree |
| `doc/guide/03-architecture.md` | HPS/FPGA split, communication model, hierarchy |
| `doc/guide/04-hardware.md` | Every file in `hw/`, with line citations |
| `doc/guide/05-software.md` | Every file in `sw/`, with line citations |
| `doc/guide/06-register-map.md` | Exhaustive Avalon-MM register reference |
| `doc/guide/07-frame-walkthrough.md` | End-to-end one-frame data flow |
| `doc/guide/08-build-and-run.md` | Build, deploy, run, debug on the board |
| `doc/guide/09-references-and-glossary.md` | This file |
| `doc/guide/diagrams/01..09-*.d2 / .svg` | All architecture diagrams (D2 source + rendered SVG) |

## Glossary

- **Avalon-MM** — Intel's on-chip memory-mapped bus. Our peripheral
  speaks it as a slave; the HPS-to-FPGA bridge presents it to the
  ARM CPU as a memory-mapped region at `0xff200000`.
- **Bitstream** — the bag of bits that configures the FPGA fabric.
  Files: `.sof` (Quartus output), `.rbf` (raw form for SD-card boot).
- **Blanking interval** — the period between successive lines or
  frames during which the VGA beam is invisible. `VGA_BLANK_n` is
  low during blanking; we drive the analog output to black during
  this time.
- **BRAM / M10K** — small embedded memory blocks in the FPGA fabric.
  Cyclone V has M10K blocks (10,240 bits each). Our sprite ROMs
  infer one M10K each.
- **Compatible string** — the textual identifier that ties a
  devicetree node to a Linux kernel driver. Ours is
  `csee4840,pvz_gpu-1.0`.
- **Cyclone V SoC** — the FPGA-plus-ARM chip on our board. "SoC"
  here means **system on a chip**, not the system module name.
- **Devicetree (dts/dtb)** — a static description of hardware that
  the Linux kernel reads at boot to discover non-discoverable
  peripherals. Generated from Platform Designer's `.sopcinfo` by
  `sopc2dts`, compiled to a binary blob by `dtc`.
- **DE1-SoC** — the specific Terasic development board this project
  targets.
- **evdev** — the Linux input subsystem. Userspace reads
  `struct input_event` records from `/dev/input/eventX` to get
  keyboard / gamepad events.
- **Frame buffer** — a memory region storing pre-rendered pixels.
  We *do not have one*. We compute pixels on demand in
  `entity_drawer.sv`.
- **HPS** — Hard Processor System. The ARM half of the Cyclone V SoC.
- **Line buffer** — a one-line-deep frame buffer. Lets you draw line
  N+1 while displaying line N. We don't use one; the bubble-bobble
  reference design does.
- **M10K** — see BRAM.
- **Misc device** — the simplest character device type in Linux. We
  expose `/dev/pvz` as one. Single major number (10), dynamic minor.
- **Palette-indexed** — each pixel is stored as a small index (8 bits
  here) into a separate color table. Cheaper than storing 24-bit RGB
  per pixel in a sprite ROM.
- **Platform Designer (Qsys)** — Intel's IP integration tool. State
  in `.qsys` (XML). Generates the Verilog that ties our peripheral to
  the HPS's bus fabric.
- **Probe** — the kernel driver function that runs when the kernel
  finds a matching devicetree node. Allocates resources and exposes
  the device to userspace.
- **Racing the beam** — generating pixels in real time, in sync with
  the VGA scan, rather than from a frame buffer. The strategy used
  by `entity_drawer.sv`.
- **`.rbf`** — Raw Binary File. The FPGA bitstream in a format
  U-Boot can load from the SD card's FAT partition.
- **`.sof`** — SRAM Object File. Quartus's native bitstream output,
  used by `quartus_pgm` for JTAG loading. Converted to `.rbf` for
  SD-card boot.
- **Sprite** — a 64×64 bitmap stored in a sprite ROM, drawn by the
  compositor wherever the corresponding entity register says it
  should appear.
- **VGA** — Video Graphics Array, an analog video standard. Our
  640×480 @ 60 Hz output drives the board's analog VGA DAC.
- **Vsync** — the vertical sync pulse that marks the start of a
  frame. Lab 3 latches new register values on vsync to avoid
  tearing; we don't on this branch.
