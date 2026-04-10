# Milestone 1: MVP Implementation Report

## Overview

Milestone 1 delivers a playable single-level Plants vs Zombies demo on the DE1-SoC. The player places Peashooters on a 4x8 grid to defend against five incoming zombies, using keyboard controls and a sun-based economy. All graphics are drawn as primitive shapes (filled rectangles, filled circles, and 7-segment digits) rendered by the FPGA at 640x480@60 Hz with no tearing.

This covers the full stack: FPGA display engine, Avalon register interface, Linux kernel driver, and C userspace game loop. Sprites, audio, USB gamepad, and additional plant/zombie types are not included yet.

---

## System Architecture

The system is split between the FPGA fabric and the ARM HPS processor. The FPGA owns the display pipeline and does nothing else. The HPS owns all game logic and tells the FPGA what to draw each frame by writing shape descriptors through memory-mapped registers.

```
+-----------------------------+       Avalon-MM        +----------------------------+
|         HPS (ARM)           | <------ bridge ------> |       FPGA Fabric          |
|                             |   (0xFF200000 base)    |                            |
|  main.c    game loop 60Hz   |                        |  pvz_top.sv                |
|  game.c    entities/logic   |   5 word-aligned regs  |    vga_counters.sv         |
|  render.c  state -> shapes  | -----> BG_CELL         |    linebuffer.sv           |
|  input.c   keyboard poll    | -----> SHAPE_ADDR      |    bg_grid.sv              |
|  pvz_driver.c  ioctl layer  | -----> SHAPE_DATA0     |    shape_table.sv          |
|                             | -----> SHAPE_DATA1     |    shape_renderer.sv       |
|                             | -----> SHAPE_COMMIT    |    color_palette.sv        |
+-----------------------------+                        +----------------------------+
                                                              |          |
                                                          VGA RGB    Sync signals
                                                              |          |
                                                         [ VGA Monitor ]
```

### Why this split

The FPGA handles pixel-level rendering because the HPS cannot push 640x480x60 = 18.4 million pixels per second over the bridge. Instead, the HPS writes at most 48 shape descriptors per frame (roughly 240 register writes) and the FPGA turns those into pixels autonomously. The bridge bandwidth required drops by several orders of magnitude.

The HPS handles game logic because state machines for zombie AI, collision detection, and input parsing are far easier to write and debug in C than in RTL.

---

## FPGA Display Engine

### Module breakdown

| Module | File | Role |
|--------|------|------|
| `vga_counters` | `hw/vga_counters.sv` | Generates hcount/vcount, hsync/vsync, and a blanking signal for 640x480@60 Hz (25.175 MHz pixel clock, 800x525 total) |
| `linebuffer` | `hw/linebuffer.sv` | Two 640x8-bit SRAM banks. One is written by the renderer while the other is read for display. They swap every hsync |
| `bg_grid` | `hw/bg_grid.sv` | Stores a 4x8 array of 8-bit color indices (one per grid cell). Given any pixel coordinate, it outputs the color index of the cell that pixel falls in |
| `shape_table` | `hw/shape_table.sv` | 48-entry register file. Each entry holds: type (2 bits), visible flag, x, y, width, height, and an 8-bit color index. Shadow registers accept writes any time; active registers latch from shadow at vsync |
| `shape_renderer` | `hw/shape_renderer.sv` | Walks all 48 entries each scanline. For each visible shape whose vertical extent overlaps the current line, it rasterizes the horizontal span into the linebuffer's write bank |
| `color_palette` | `hw/color_palette.sv` | A 256-entry lookup table that maps the 8-bit color index read from the linebuffer to 24-bit RGB for the VGA DAC. Only about 13 entries are populated for the MVP |
| `pvz_top` | `hw/pvz_top.sv` | Top-level Avalon-MM agent. Decodes five registers, wires together all the modules above, and drives VGA output |
| `soc_system_top` | `hw/soc_system_top.sv` | Board-level wrapper connecting the Platform Designer system to physical VGA pins, clocks, and HPS I/O |

### Rendering pipeline (per scanline)

The renderer works one line ahead of the display. While scanline N is being sent to the monitor from linebuffer A, the renderer fills linebuffer B with scanline N+1:

1. **Background fill** (640 cycles): `bg_grid` outputs a color index for each pixel based on its (x, y) coordinate. The game grid occupies rows 60-419 (four 90-pixel-tall rows, eight 80-pixel-wide columns) with a checkerboard of dark and light green. Pixels outside the grid get a fixed HUD or status bar color.

2. **Shape overdraw** (~960 cycles): `shape_renderer` iterates all 48 entries. For each visible shape whose y-range covers this scanline, it writes pixels into the linebuffer, overwriting the background. Shapes are processed in index order, so higher-numbered shapes draw on top (painter's algorithm).

3. **Buffer swap** (at hsync): The write and display banks swap roles. A two-cycle early swap accounts for BRAM read latency.

4. **Display output** (during active video): The display bank's 8-bit index feeds through `color_palette` to produce 24-bit RGB for the VGA DAC pins.

The total render budget is about 1600 pixel clocks per line (800 clocks per horizontal period, times two because the linebuffer gives one full line of lead time). Background fill takes 640 cycles and shape rendering takes about 20 cycles per shape, so 48 shapes would need 960 cycles in the worst case. In practice, only a fraction of shapes overlap any given scanline, so there is margin to spare.

### Shape types

The renderer supports three primitives:

- **Filled rectangle** (type 0): Any pixel within `[x, x+w) x [y, y+h)` is painted with the shape's color.
- **Filled circle** (type 1): The bounding box is checked first. Pixels inside the box are tested with `(px - cx)^2 + (py - cy)^2 <= r^2`, where the center and radius are derived from x, y, and width. This avoids square-root hardware.
- **7-segment digit** (type 2): The digit value (0-9) is encoded in the low 4 bits of the width field. Seven small rectangles are placed at fixed offsets from the shape's (x, y) origin, and each is enabled or disabled according to a segment lookup table. One shape entry produces one readable digit character.

### Double buffering and tearing prevention

The shape table and background grid both use shadow/active register pairs. Software writes go into the shadow registers at any time. At the start of vertical blanking (`vcount == 480, hcount == 0`), the active registers latch from shadow in a single cycle. This guarantees that the display engine reads a consistent snapshot for the entire frame, even if the HPS is partway through writing the next frame's data.

---

## Register Interface

The FPGA exposes five 32-bit registers on the lightweight HPS-to-FPGA bridge (base address `0xFF200000`). The Avalon bus uses word-aligned addressing, but the CPU sees byte offsets, so the kernel driver writes to `base + 4*N`.

| Offset | Register | Bit layout | Purpose |
|--------|----------|------------|---------|
| 0x00 | `BG_CELL` | `[12:8]` color, `[4:3]` row, `[2:0]` col | Write one background grid cell's color index |
| 0x04 | `SHAPE_ADDR` | `[5:0]` index | Select which of the 48 shape entries to write |
| 0x08 | `SHAPE_DATA0` | `[1:0]` type, `[2]` visible, `[12:3]` x, `[21:13]` y | Shape descriptor word 0 |
| 0x0C | `SHAPE_DATA1` | `[8:0]` width, `[17:9]` height, `[25:18]` color | Shape descriptor word 1 |
| 0x10 | `SHAPE_COMMIT` | write `1` | Commits the data in SHAPE_DATA0/DATA1 to the shadow table at the address set by SHAPE_ADDR |

Writing a single shape takes four register writes: set the address, write DATA0, write DATA1, then write COMMIT. The shadow-to-active latch happens automatically at vsync — there is no software trigger for that.

---

## Kernel Driver

**File:** `sw/pvz_driver.c`

The driver follows the lab3 `vga_ball.c` pattern exactly. It registers as a platform device matching the device tree compatible string `"csee4840,pvz_gpu-1.0"` (set in `pvz_top_hw.tcl`), maps the Avalon register region with `devm_ioremap_resource`, and exposes a misc device at `/dev/pvz`.

Three ioctl commands are defined in `sw/pvz.h`:

| ioctl | Argument struct | What it does |
|-------|-----------------|-------------|
| `PVZ_WRITE_BG` | `{row, col, color}` | Packs the fields into a single 32-bit word and writes to the BG_CELL register |
| `PVZ_WRITE_SHAPE` | `{index, type, visible, x, y, w, h, color}` | Writes SHAPE_ADDR, SHAPE_DATA0, SHAPE_DATA1, and SHAPE_COMMIT in sequence |
| `PVZ_COMMIT_SHAPES` | (none) | Writes 1 to the SHAPE_COMMIT register (used after batch updates) |

The driver validates inputs (index < 48, row < 4, col < 8) and returns `-EINVAL` for anything out of range. All register access uses `iowrite32` with byte offsets.

### Loading and verifying

```bash
insmod pvz_driver.ko           # module loads, /dev/pvz appears
echo 8 > /proc/sys/kernel/printk
dmesg | tail                    # should show probe success message
cat /proc/iomem                 # confirm ff20xxxx region is claimed
```

---

## Game Software

### Game state (`sw/game.c`, `sw/game.h`)

The game tracks the following state:

- **Grid**: 4 rows by 8 columns. Each cell is either empty or holds a Peashooter with its own fire cooldown timer and hit points (HP).
- **Zombies**: Up to 5 active at once. Each has a row, an x-position in pixels, HP (starts at 3), a movement counter, and an eating state (whether it is currently eating a plant and a bite timer).
- **Projectiles**: Up to 16 peas in flight. Each has a row and an x-position in pixels.
- **Cursor**: The player's current row and column selection.
- **Economy**: Sun counter starting at 100, with passive income of 25 sun every 8 seconds.
- **Spawn state**: Countdown timer, count of zombies spawned so far, and count alive.

### Update loop (`sw/main.c`)

The main loop runs at 60 Hz, paced by `usleep(16667)`:

1. **Input**: Poll `/dev/input/eventX` for key events. Arrow keys move the cursor, Space places a Peashooter (costs 50 sun), D removes a plant, ESC quits.
2. **Timers**: Increment frame counter. Award sun income when the sun timer elapses. Decrement spawn countdown.
3. **Spawn**: When the countdown hits zero and fewer than 5 zombies have been spawned, place a new zombie at a random row entering from the right edge (x = 639).
4. **Move zombies**: Every `ZOMBIE_SPEED_FRAMES` (3) frames, each zombie's x-position decreases by 1 pixel, giving roughly 20 px/sec of leftward movement. If a zombie enters a cell with a plant in the same row, it stops and starts eating. It deals 1 HP damage every 60 frames (~1 second). When the plant's HP hits 0, the plant is removed and the zombie resumes moving.
5. **Fire**: Each Peashooter checks whether a zombie exists in its row. If so and the cooldown has expired, it spawns a pea at the plant's column position.
6. **Move projectiles**: Each pea moves right by `PEA_SPEED` (2) pixels per frame.
7. **Collision**: For each pea, if it overlaps a zombie in the same row (bounding box check), the zombie loses 1 HP and the pea is deactivated.
8. **Cleanup**: Remove zombies with HP <= 0. Remove peas that have left the screen.
9. **Win/lose check**: Win if all 5 zombies have been spawned and all are dead. Lose if any zombie reaches x <= 0.
10. **Render**: Convert game state to shape table entries and write them to the FPGA.

### Rendering (`sw/render.c`)

The render step translates the game state into a set of shape descriptors:

| Entity | Shape slots | Representation |
|--------|------------|----------------|
| Peashooter | 2 per plant | Green rectangle (body) + dark green rectangle (stem) |
| Zombie | 2 per zombie | Red rectangle (body) + dark red rectangle (head) |
| Pea | 1 per projectile | Small bright green circle |
| Cursor | 1 | Yellow rectangle outline around the selected cell |
| Sun counter | 3 | Three 7-segment digits (white) showing current sun |

Unused shape entries have their visible flag set to 0. Shape indices are allocated in ascending z-order: plants in slots 0-31, zombies in 32-36, projectiles in 37-42, HUD digits in 43-46, and cursor in 47. Higher indices draw on top, so the HUD and cursor are always visible.

### Input (`sw/input.c`)

Keyboard input is read from `/dev/input/eventX` using non-blocking `read()` calls. The input module opens the device on init, and each frame it drains any pending `input_event` structs. Only key-press events (value == 1) are acted on; key-repeat and key-release are ignored. The cursor is clamped to the grid bounds (rows 0-3, columns 0-7).

---

## Screen Layout

The 640x480 display is divided into four horizontal bands:

```
  0 +-----------------------------------------------+
    |               Top HUD (60 px)                  |
    |  Plant card slot       Sun counter: 100        |
 60 +-----------------------------------------------+
    |                                                |
    |         Game Grid: 4 rows x 8 columns          |
    |         Each cell is 80 x 90 pixels            |
    |         Checkerboard dark/light green           |
    |                                                |
420 +-----------------------------------------------+
    |             Status Bar (40 px)                  |
460 +-----------------------------------------------+
    |           Control Hints (20 px)                 |
480 +-----------------------------------------------+
```

Grid cell `(r, c)` maps to pixel coordinates `x = c * 80` through `x = c * 80 + 79`, `y = 60 + r * 90` through `y = 60 + r * 90 + 89`.

---

## Color Palette (MVP)

| Index | RGB | Color | Used for |
|-------|-----|-------|----------|
| 0 | (0, 0, 0) | Black | Screen border, text |
| 1 | (34, 139, 34) | Dark green | Grid cell (dark) |
| 2 | (50, 205, 50) | Light green | Grid cell (light) |
| 3 | (139, 69, 19) | Brown | HUD background |
| 4 | (255, 255, 0) | Yellow | Cursor highlight |
| 5 | (255, 0, 0) | Red | Zombie body |
| 6 | (139, 0, 0) | Dark red | Zombie head |
| 7 | (0, 200, 0) | Green | Peashooter body |
| 8 | (0, 128, 0) | Dark green | Peashooter stem |
| 9 | (0, 255, 0) | Bright green | Pea projectile |
| 10 | (255, 255, 255) | White | Digits |
| 11 | (128, 128, 128) | Gray | Status bar background |
| 12 | (255, 165, 0) | Orange | Sun drops |

---

## Testing

### Hardware testbenches (ModelSim)

Five testbenches live in `hw/tb/`:

- `tb_vga_counters` — confirms hsync/vsync timing, hcount and vcount ranges, and blanking behavior across a full frame.
- `tb_linebuffer` — writes known patterns into one bank, swaps, and reads them back from the other bank. Tests boundary pixels at index 0 and 639.
- `tb_bg_grid` — sets cell colors via shadow registers, triggers a latch, and checks that pixel coordinates within each cell return the correct color index.
- `tb_shape_renderer` — verifies all three shape types render correctly. Tests overlapping shapes to confirm painter's algorithm z-ordering. Tests that shapes with visible=0 produce no output.
- `tb_pvz_top` — end-to-end test: performs Avalon register writes (background cells and shapes), then runs a full frame of VGA timing and checks that specific pixel coordinates produce the expected RGB output.

### On-board test programs

- `sw/test/test_shapes.c` — writes a checkerboard background and a sampling of rectangles, circles, and digits via ioctl. Used to visually confirm the display pipeline on a monitor.
- `sw/test/test_input.c` — prints raw keyboard events to the console to verify input device detection and key mapping.

### Software unit tests

- `sw/test/test_game.c` — compilable on any host machine (no FPGA required). Tests collision detection, sun economy calculations, zombie spawn logic, and win/lose conditions. Prints pass/fail for each test case.

---

## Build Instructions

### Hardware (on a workstation with Intel FPGA tools)

```bash
cd hw/
make qsys       # regenerate Verilog from soc_system.qsys
make quartus     # synthesize -> output_files/soc_system.sof
make rbf         # convert .sof to .rbf for SD card boot
make dtb         # generate device tree blob (requires embedded_command_shell.sh)
```

Copy `output_files/soc_system.rbf` and `soc_system.dtb` to the SD card's FAT boot partition, then reboot the board.

### Software (cross-compiled or built directly on the DE1-SoC)

```bash
cd sw/
make                          # builds pvz_driver.ko and the userspace programs
sudo insmod pvz_driver.ko     # load the driver, /dev/pvz appears
./pvz                         # run the game
sudo rmmod pvz_driver         # unload before rebuilding
```

### Running tests

```bash
# On the board, after loading the driver:
./test/test_shapes            # visual test on VGA monitor
./test/test_input             # keyboard event dump

# On any Linux host:
gcc -o test_game sw/test/test_game.c sw/game.c -I sw/
./test_game                   # unit tests, no hardware needed
```

---

## Future milestones

Planned additions beyond the MVP:

- **Sprites**: Replace primitive shapes with bitmap sprites loaded from ROM (tile/sprite engine following the bubble-bobble reference design).
- **Audio**: Stream sound effects and background music to the Wolfson WM8731 codec via I2S.
- **USB gamepad**: Replace keyboard input with USB HID gamepad polling via libusb.
- **More plant and zombie types**: Sunflower, Wall-nut, Chomper on the plant side; Cone-head and Buckethead zombies with higher HP.
- **Multiple levels**: Wave progression, difficulty scaling, and a level-select screen.

---

## File-by-File Reference

Each file is described below with its dependencies and where its outputs go.

### Hardware — FPGA Design (`hw/`)

#### `hw/vga_counters.sv` — VGA Timing Generator

Produces the horizontal and vertical counters (`hcount`, `vcount`), sync signals (`hsync`, `vsync`), and a blanking flag for 640x480 @ 60 Hz. The pixel clock is 25.175 MHz. The counters run from 0 to 799 horizontally and 0 to 524 vertically, with active video in the range `hcount < 640` and `vcount < 480`.

- **Depends on**: nothing (pure timing logic, no data inputs)
- **Used by**: `pvz_top.sv`, which passes hcount/vcount to the renderer and linebuffer, and routes hsync/vsync to the VGA connector
- **Derived from**: the `vga_counters` module inside `doc/reference-design/lab3/hw/vga_ball.sv`

#### `hw/linebuffer.sv` — Dual Line Buffer

Two 640-entry x 8-bit SRAM banks. At any given time, one bank is being read by the display output and the other is being written by the renderer. They swap roles on each hsync (with a two-cycle early trigger to handle BRAM read latency).

- **Write port**: the renderer writes an 8-bit color index at a given x-position
- **Read port**: the display side reads the color index at the current `hcount` position
- **Swap signal**: driven by `pvz_top.sv` based on hcount reaching the end of active video
- **Used by**: `pvz_top.sv` (connects write port to `bg_grid` and `shape_renderer`, read port to `color_palette`)
- **Design reference**: `doc/reference-design/bubble-bobble/code/vga_audio_integration/vga/linebuffer.sv`

#### `hw/color_palette.sv` — Color Index to RGB Lookup

A 256-entry ROM that maps an 8-bit index to a 24-bit RGB value. Only 13 entries are populated for the MVP (see the Color Palette table above). All other entries default to black.

- **Input**: 8-bit color index (from linebuffer read port)
- **Output**: 8-bit R, G, B channels (directly driving VGA DAC pins)
- **Used by**: `pvz_top.sv` (sits between the linebuffer's display output and the VGA pins)

#### `hw/bg_grid.sv` — Background Grid

Stores a 4-row x 8-column array of 8-bit color indices, one per grid cell. Given a pixel coordinate `(x, y)`, it outputs the color index of the cell containing that pixel. Pixels outside the game grid area (y < 60 or y >= 420) return a fixed color for the HUD or status bar.

- **Write interface**: shadow registers written via Avalon (BG_CELL register), latched to active at vsync
- **Read interface**: combinational lookup from `(x, y)` to color index
- **Used by**: `pvz_top.sv` (fills the linebuffer's write bank at the start of each scanline, before shape rendering begins)

#### `hw/shape_table.sv` — Shape Register File

48-entry table where each entry describes one drawable shape: type (2 bits), visible flag, x position (10 bits), y position (9 bits), width (9 bits), height (9 bits), and color index (8 bits). Uses shadow/active double-buffering — writes go to shadow registers, and the entire table latches to active at the vsync boundary.

- **Write path**: `pvz_top.sv` decodes SHAPE_ADDR, SHAPE_DATA0, SHAPE_DATA1, and SHAPE_COMMIT from Avalon writes. On COMMIT, the addressed shadow entry is updated.
- **Read path**: `shape_renderer.sv` iterates all 48 active entries each scanline
- **Latch trigger**: `vcount == 480 && hcount == 0` (start of vertical blanking)

#### `hw/shape_renderer.sv` — Scanline Shape Rasterizer

The rendering core. Each scanline, it walks all 48 active shape entries and, for each visible shape whose y-range overlaps the current line, writes colored pixels into the linebuffer's write bank. Shapes are processed in index order: later entries overwrite earlier ones (painter's algorithm).

- **Inputs**: current vcount (from `vga_counters`), the 48 active shape entries (from `shape_table`)
- **Output**: write-enable, write-address, and write-data signals to `linebuffer`
- **Shape types handled**:
  - Type 0 (rectangle): fills pixels from x to x+width-1 on lines from y to y+height-1
  - Type 1 (circle): bounding-box pre-check, then `dx^2 + dy^2 <= r^2` per pixel; radius = width/2
  - Type 2 (7-segment digit): value in width[3:0], seven segment rectangles at fixed offsets
- **Timing**: approximately 20 clock cycles per shape; 48 shapes fit within the roughly 960-cycle budget between background fill and buffer swap

#### `hw/pvz_top.sv` — Avalon-MM Agent (Top-Level FPGA Module)

Wires all display modules together and exposes them to the HPS through five Avalon-MM registers. This is the module that Platform Designer instantiates and connects to the lightweight HPS-to-FPGA bridge.

- **Avalon interface**: 32-bit writedata, active-low chipselect and write_n, 3-bit address (word-aligned)
- **Register decode**: translates Avalon writes into updates to `bg_grid` shadow registers and `shape_table` shadow entries
- **Rendering orchestration**: on each scanline, first tells `bg_grid` to fill the linebuffer, then tells `shape_renderer` to overdraw shapes, then triggers linebuffer swap
- **VGA output**: routes `vga_counters` sync signals and `color_palette` RGB output to top-level ports
- **Instantiates**: `vga_counters`, `linebuffer`, `bg_grid`, `shape_table`, `shape_renderer`, `color_palette`

#### `hw/soc_system_top.sv` — Board Top-Level

The outermost SystemVerilog file. Instantiates the Platform Designer-generated `soc_system` (which contains the HPS, bridges, and `pvz_top` as a component) and connects its ports to physical FPGA pins: VGA R/G/B, VGA hsync/vsync, CLOCK_50, and all HPS DDR/USB/UART pins.

- **Derived from**: `doc/reference-design/lab3/hw/soc_system_top.sv`
- **When to modify**: only if you add new physical I/O (audio codec pins, GPIO, etc.)

#### `hw/pvz_top_hw.tcl` — Platform Designer Component Descriptor

Tells Platform Designer how to instantiate `pvz_top`: which ports are Avalon-MM slave signals, which are VGA conduit exports, clock/reset connections, and the device-tree compatible string (`csee4840,pvz_gpu-1.0`).

- **Critical three-way tie**: the compatible string here must match `pvz_driver.c`'s `of_device_id` table and the `pvz_top.sv` module name in Platform Designer. If any of the three diverge, the driver will not bind to the device.
- **When to modify**: if you add registers (change the address span), add new conduit ports (audio, GPIO), or rename the component

#### `hw/Makefile` — Hardware Build System

Targets: `make qsys` (regenerate Verilog from Platform Designer), `make quartus` (full synthesis), `make rbf` (convert .sof to .rbf for SD boot), `make dtb` (generate device tree blob). Adapted from `doc/reference-design/lab3/hw/Makefile`.

### Hardware — Testbenches (`hw/tb/`)

All testbenches are self-checking and print pass/fail messages.

| File | Tests | How it works |
|------|-------|-------------|
| `tb_vga_counters.sv` | VGA timing correctness | Runs a full frame, checks that hcount wraps at 800, vcount at 525, sync pulses land at the right positions |
| `tb_linebuffer.sv` | Dual-buffer write/swap/read | Writes a pattern to bank A, swaps, reads back from display port, checks values. Tests boundary pixels 0 and 639 |
| `tb_bg_grid.sv` | Grid cell color lookup | Sets known colors via shadow registers, latches, queries pixel coordinates at cell centers and boundaries |
| `tb_shape_renderer.sv` | All shape types + z-order | Creates overlapping rectangle, circle, and digit shapes. Runs one scanline of rendering. Checks that the correct pixels were written to the linebuffer |
| `tb_pvz_top.sv` | Full pipeline end-to-end | Performs Avalon register writes (bg cells and shapes), runs a complete frame, checks RGB output at specific pixel coordinates |

### Software — Kernel Driver (`sw/`)

#### `sw/pvz.h` — Shared Definitions

Defines the ioctl command numbers (`PVZ_WRITE_BG`, `PVZ_WRITE_SHAPE`, `PVZ_COMMIT_SHAPES`) and the C structs passed as ioctl arguments. This header is included by both the kernel driver and userspace programs.

- **Used by**: `pvz_driver.c`, `render.c`, `test_shapes.c`

#### `sw/pvz_driver.h` — Driver Internals

Internal header for the kernel module. Defines the device struct (holding the `__iomem` pointer to the mapped registers) and register offset constants.

- **Used by**: `pvz_driver.c` only

#### `sw/pvz_driver.c` — Kernel Module

Platform device driver that binds to the device tree node matching `"csee4840,pvz_gpu-1.0"`. On probe, it maps the Avalon register region into kernel virtual address space. It registers a misc device at `/dev/pvz` and handles the three ioctl commands by packing arguments into 32-bit words and calling `iowrite32`.

- **Depends on**: `pvz.h` (ioctl definitions), `pvz_driver.h` (register offsets)
- **Loaded with**: `insmod pvz_driver.ko`
- **Device node**: `/dev/pvz` (automatically created by misc device framework)
- **Pattern reference**: `doc/reference-design/lab3/sw/vga_ball.c`

### Software — Game Logic (`sw/`)

#### `sw/game.h` — Game State Definitions

All struct definitions and constants for the game: `plant_t` (with HP), `zombie_t` (with eating state), `projectile_t`, `game_state_t`. Also defines gameplay tuning constants (grid size, HP values, speeds, costs, spawn intervals, eating cooldown). All tuning constants live here.

- **Used by**: `game.c`, `render.c`, `main.c`, `test_game.c`

#### `sw/game.c` — Game Mechanics

Contains `game_init()` (sets up initial state) and `game_update()` (one frame of game logic). The update function handles zombie movement, zombie-plant collision and eating, peashooter firing, projectile movement, pea-zombie collision detection, sun income, zombie spawning, and win/lose checks. All game rules live here.

- **Depends on**: `game.h`
- **Called by**: `main.c` (once per frame)
- **Tested by**: `test/test_game.c`

#### `sw/render.h` / `sw/render.c` — Game State to Shape Table

Translates the current `game_state_t` into a sequence of `PVZ_WRITE_SHAPE` ioctls. Maps each plant to 2 shape entries (body + stem), each zombie to 2 entries (body + head), each projectile to 1 circle entry, the cursor to 1 rectangle, and the sun counter to 3 digit entries. Clears unused entries by setting `visible = 0`.

- **Depends on**: `game.h` (reads game state), `pvz.h` (ioctl structs)
- **Called by**: `main.c` (after `game_update`, before the next frame)
- **Writes to**: `/dev/pvz` via ioctl (the file descriptor is passed in)

#### `sw/input.h` / `sw/input.c` — Keyboard Input

Opens `/dev/input/eventX` in non-blocking mode. Each frame, `input_poll()` reads any pending `struct input_event` records and translates key-press events into game actions (move cursor, place plant, dig plant, quit). Key-repeat and key-release events are filtered out.

- **Depends on**: Linux input subsystem headers (`<linux/input.h>`)
- **Called by**: `main.c` (at the start of each frame, before `game_update`)
- **Outputs**: updates cursor position and action flags in the game state

#### `sw/main.c` — Main Game Loop

The entry point. Opens `/dev/pvz`, initializes the game state and input system, sets up the background grid, then enters a 60 Hz loop: poll input, update game, render to FPGA. On exit (ESC or win/lose), closes the device and cleans up.

- **Depends on**: `game.h`, `input.h`, `render.h`, `pvz.h`
- **Timing**: `usleep(16667)` for ~60 Hz (with drift from processing time)

#### `sw/Makefile` — Software Build System

Two-pass Makefile following the kbuild pattern. When invoked normally, it calls `make -C /usr/src/linux-headers-$(uname -r)` to build the kernel module. It also has gcc targets for the userspace game binary and test programs. Adapted from `doc/reference-design/lab3/sw/Makefile`.

### Software — Tests (`sw/test/`)

#### `sw/test/test_shapes.c` — Visual Display Test

Standalone program that opens `/dev/pvz` and writes a series of shapes (rectangles, circles, digits) and background cell colors via ioctl. Intended to be run on the board with a VGA monitor attached. If shapes appear at the expected positions with the expected colors, the entire pipeline — driver, Avalon registers, FPGA rendering — is working.

- **Depends on**: `pvz.h`
- **Run on**: DE1-SoC with driver loaded

#### `sw/test/test_input.c` — Keyboard Event Test

Opens `/dev/input/eventX` and prints every key event to stdout. Used to verify that the correct input device is being read and that key codes are mapping properly.

- **Run on**: DE1-SoC with a USB keyboard connected

#### `sw/test/test_game.c` — Game Logic Unit Tests

Host-compilable test suite that exercises `game.c` without any hardware. Tests collision detection (pea hits zombie, HP decreases), sun economy (income timing, cost deduction, overdraft prevention), zombie spawning (count limits, row randomization), win/lose conditions, and zombie-plant collision (eating, damage, plant destruction, multi-zombie eating).

- **Depends on**: `game.c`, `game.h`
- **Run on**: any Linux machine with gcc — no FPGA, driver, or cross-compiler needed

### Data flow between files

To understand how a single frame moves through the system, trace these calls:

```
main.c:  input_poll(&state)            -->  input.c reads /dev/input/eventX
main.c:  game_update(&state)           -->  game.c modifies plants/zombies/peas
main.c:  render_frame(fd, &state)      -->  render.c converts state to shapes
  render.c: ioctl(fd, PVZ_WRITE_SHAPE) -->  pvz_driver.c writes Avalon registers
    pvz_driver.c: iowrite32(...)       -->  pvz_top.sv shadow registers updated
      (at vsync)                       -->  shape_table.sv latches shadow->active
      (next scanline)                  -->  shape_renderer.sv rasterizes shapes
      (display output)                 -->  linebuffer.sv -> color_palette.sv -> VGA pins
```
