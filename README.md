# Plants vs Zombies on DE1-SoC

A simplified Plants vs Zombies clone running on the Terasic DE1-SoC (Cyclone V). Final project for Columbia's CSEE 4840 Embedded Systems Design, Spring 2026.

The FPGA renders VGA output at 640x480@60 Hz using a hardware shape table, dual linebuffers, and a color-indexed palette. The HPS runs the game loop as a C program under Linux (grid state, plants, zombies, projectiles, sun economy, keyboard input) and pushes shape descriptors to the FPGA each frame through a custom kernel driver and memory-mapped Avalon registers.

See [`doc/proposal/proposal.md`](doc/proposal/proposal.md) for the full design and [`doc/mvp_design_plan.md`](doc/mvp_design_plan.md) for the implementation plan.

## Current status

**Milestone 1 (MVP) complete.** Single-level playable demo with primitive-shape graphics (rectangles, circles, 7-segment digits), keyboard control, one plant type (Peashooter), and five zombies. No sprites, audio, or USB gamepad yet. See [`doc/milestone1.md`](doc/milestone1.md) for details.

## Repository layout

```
hw/                        FPGA design (SystemVerilog)
  vga_counters.sv            VGA 640x480@60Hz timing generator
  linebuffer.sv              Dual 640x8-bit line RAM with hsync swap
  color_palette.sv           256-entry 8-bit-to-24-bit RGB LUT
  bg_grid.sv                 4x8 background grid color array
  shape_table.sv             48-entry shape register file (shadow/active)
  shape_renderer.sv          Scanline rasterizer (rect, circle, 7-seg digit)
  pvz_top.sv                 Avalon-MM agent wiring display pipeline
  soc_system_top.sv          Board top-level (clocks, VGA pins, HPS)
  pvz_top_hw.tcl             Platform Designer component descriptor
  Makefile                   Quartus/Platform Designer build targets
  tb/                        ModelSim testbenches

sw/                        HPS software (C, Linux)
  pvz_driver.c               Kernel module (misc device at /dev/pvz)
  pvz_driver.h               Driver internals
  pvz.h                      Shared ioctl definitions and structs
  main.c                     60 Hz game loop, init, cleanup
  game.c / game.h            Game state, entity logic, collision
  render.c / render.h        Game state -> shape table conversion
  input.c / input.h          Keyboard input via /dev/input/eventX
  Makefile                   Kbuild driver + gcc userspace
  test/                      On-board test programs and unit tests

doc/                       Documentation
  proposal/                  Project proposal (authoritative spec)
  mvp_design_plan.md         MVP implementation plan
  milestone1.md              Milestone 1 architecture and status report
  manual/                    DE1-SoC user manual
  reference-design/lab3/     Lab 3 skeleton (build system template)
  reference-design/bubble-bobble/  Prior team reference project
```

## Building, running, and testing

### Prerequisites

FPGA synthesis runs on Columbia's `micro*.ee.columbia.edu` workstations, which have Quartus Prime and Platform Designer installed. Software compilation can happen either on the workstation (cross-compile) or directly on the DE1-SoC, which has GCC.

- Intel Quartus Prime + Platform Designer, for `make qsys`, `make quartus`, `make rbf`
- `embedded_command_shell.sh` sourced before running `make dtb` (provides `sopc2dts` and `dtc`; the Makefile prints an error if these are missing)
- Linux kernel headers at `/usr/src/linux-headers-$(uname -r)`, for building `pvz_driver.ko`
- GCC and pthread (the game binary links with `-lpthread`)

### Hardware build (FPGA)

All commands run from the `hw/` directory on a workstation:

```bash
cd hw/
make qsys       # regenerate Verilog from soc_system.qsys
make quartus    # full synthesis, fitting, timing analysis -> output_files/soc_system.sof
make rbf        # convert .sof to .rbf for SD card boot
make dtb        # generate device tree blob (sopc2dts -> .dts, then dtc -> .dtb)
```

| Target | Runs | Output |
|--------|------|--------|
| `make project` | `quartus_sh -t soc_system.tcl` | `.qpf`, `.qsf`, `.sdc` (one-time setup) |
| `make qsys` | `qsys-generate soc_system.qsys --synthesis=VERILOG` | `soc_system.sopcinfo`, `soc_system/` |
| `make quartus` | `quartus_sh --flow compile soc_system.qpf` | `output_files/soc_system.sof` |
| `make rbf` | `quartus_cpf -c .sof .rbf` | `output_files/soc_system.rbf` |
| `make dtb` | `sopc2dts` then `dtc -I dts -O dtb` | `soc_system.dtb` |
| `make clean` | removes all generated artifacts | |

### Software build (kernel module and userspace)

All commands run from the `sw/` directory:

```bash
cd sw/
make              # builds everything: driver, game binary, and test programs
```

| Target | What it builds | Compilation |
|--------|---------------|-------------|
| `make` (default) | All targets below | |
| `make module` | `pvz_driver.ko` | kbuild against `/usr/src/linux-headers-$(uname -r)` |
| `make pvz` | `pvz` game executable | `gcc -Wall -O2 -o pvz main.c game.c render.c input.c -lpthread` |
| `make test_shapes` | `test_shapes` | `gcc -Wall -O2 -o test_shapes test/test_shapes.c` |
| `make test_input` | `test_input` | `gcc -Wall -O2 -o test_input test/test_input.c input.c` |
| `make test_game` | `test_game` | `gcc -Wall -O2 -o test_game test/test_game.c game.c` |
| `make clean` | removes `.ko`, `.o`, and all binaries | |

You must run `rmmod pvz_driver` before rebuilding the kernel module.

### Deploying to the board

1. Copy `hw/output_files/soc_system.rbf` and `hw/soc_system.dtb` to the SD card's FAT boot partition.

2. Insert the SD card and power on the board. You can also test without reflashing by using U-Boot:
   ```
   fatload mmc 0:1 $fpgadata soc_system.rbf
   fpga load 0 $fpgadata $filesize
   run bridge_enable_handoff
   ```

3. Connect to the board's serial console from a workstation:
   ```
   screen /dev/ttyUSB0 115200
   ```

4. Load the kernel driver:
   ```
   insmod pvz_driver.ko
   ```

5. Verify the driver bound correctly:
   ```bash
   dmesg | tail                          # should show probe success
   cat /proc/iomem                       # confirm ff20xxxx region is claimed
   ls /proc/device-tree/sopc@0/          # confirm device tree entry exists
   ```

6. Run the game:
   ```
   ./pvz
   ```
   Controls: arrow keys move the cursor, Space places a Peashooter (costs 50 sun), D removes a plant, ESC quits.

### Testing

Tests are split into three categories depending on where they run.

#### Hardware testbenches (ModelSim, on workstation)

Five self-checking testbenches in `hw/tb/`. They print pass/fail to the simulation console. Run with ModelSim:

```
vsim -do "run -all; quit" work.tb_<name>
```

| Testbench | What it verifies |
|-----------|-----------------|
| `tb_vga_counters` | hcount cycles 0--1599, vcount cycles 0--524, sync pulse positions, blanking only during active area, VGA_CLK matches hcount[0] |
| `tb_linebuffer` | Write to draw buffer while display buffer reads zero, swap toggles roles, boundary pixels (0 and 639) read back correctly, 1-cycle read latency |
| `tb_bg_grid` | Cell color lookup at known coordinates, shadow writes invisible until vsync latch, out-of-game-area queries return 0 |
| `tb_shape_renderer` | Rectangle pixels land at expected coordinates, z-order (later shapes overwrite earlier), invisible shapes skipped, write count covers full scanline |
| `tb_pvz_top` | Full pipeline: Avalon register writes for background cells and shapes produce correct non-zero VGA RGB output after a full frame; has a 200 ms timeout watchdog |

#### On-board test programs (DE1-SoC, driver must be loaded)

These run on the board after `insmod pvz_driver.ko` and are meant for manual verification:

```bash
./test_shapes                       # visual test: writes checkerboard + shapes to VGA
./test_input [/dev/input/eventN]    # keyboard test: prints key events at ~60 Hz
```

- `test_shapes` opens `/dev/pvz` and writes a checkerboard background with six shapes (green rectangle, green circle, white digit "5", red rectangle, yellow cursor, invisible rectangle) via ioctl. Check the VGA monitor. Runs until Ctrl+C.
- `test_input` polls the keyboard at 60 Hz and prints recognized keys (UP, DOWN, LEFT, RIGHT, SPACE, D, ESC). Defaults to `/dev/input/event0`. Press ESC to exit.

#### Software unit tests (any Linux host, no hardware needed)

`test_game` tests the game logic in `game.c` with no hardware dependency. Compile and run on any Linux machine:

```bash
make test_game && ./test_game
```

Or compile it standalone:

```bash
gcc -Wall -O2 -o test_game test/test_game.c game.c && ./test_game
```

The test suite covers eight areas:

| Test | What it checks |
|------|---------------|
| `test_init` | Initial state: sun = 100, state = PLAYING, cursor at (0, 0), grid empty |
| `test_place_plant` | Plant goes into the grid, sun decreases by 50, rejects duplicate placement, rejects when sun is too low |
| `test_remove_plant` | Plant removed from cell, rejects removal from an empty cell |
| `test_sun_economy` | Sun increments by 25 every 480 frames (8 seconds at 60 fps) |
| `test_collision` | Pea in the same row as a zombie reduces its HP by 1, pea is deactivated |
| `test_zombie_death` | Zombie with 1 HP left dies on the next collision hit |
| `test_lose_condition` | Game state becomes LOSE when a zombie reaches x <= 0 |
| `test_win_condition` | Game state becomes WIN after all 5 zombies have spawned and been killed |

Uses `srand(42)` for deterministic zombie spawns. Exits 0 if every test passes, 1 if any fail.

### Debugging tips

```bash
echo 8 > /proc/sys/kernel/printk     # show kernel pr_info messages on console
dmesg | tail                          # read driver probe and runtime messages
cat /proc/iomem                       # confirm the FPGA peripheral at ff20xxxx
ls /proc/device-tree/sopc@0/          # check the device tree entry
screen /dev/ttyUSB0 115200            # serial console from workstation
rmmod pvz_driver                      # unload driver before rebuilding .ko
```

If shapes do not appear on the VGA monitor, check that the shape's `visible` flag is set and its `type` is 0 (rectangle), 1 (circle), or 2 (digit). Later shape entries draw on top of earlier ones, so z-order matters. If the driver does not bind at all, make sure the compatible string in `pvz_top_hw.tcl`, the device tree, and `pvz_driver.c` all match (`"csee4840,pvz_gpu-1.0"`).
