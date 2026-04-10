# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Plants vs Zombies on the Terasic DE1-SoC (Cyclone V) — a CSEE4840 Embedded Systems final project. The full design is described in `doc/proposal/proposal.md` (read this first for gameplay rules, balance numbers, and the HW/SW split). Target: 640x480 VGA @ 60 Hz with hardware sprite blitting, USB gamepad input, and audio over the Wolfson codec.

**Current state:** Milestone 1 (MVP) is complete — working FPGA display engine, kernel driver, and game loop with primitive graphics. CI/CD via GitHub Actions cross-compiles software for ARM on every push and creates releases on `v*` tags. Board deployment is automated via `deploy.sh`.

## Repository layout

- `hw/` — FPGA design (SystemVerilog, Quartus/Platform Designer flow)
- `sw/` — HPS software (kernel driver, game loop, tests). Makefile supports cross-compilation via `CC`, `ARCH`, `CROSS_COMPILE`, `KERNEL_SOURCE` variables.
- `.github/workflows/build.yml` — CI: cross-compiles SW for ARM on push, creates GitHub Releases on `v*` tags.
- `deploy.sh` — board-side deployment script (setup/download/install/run/test/status).
- `worktree.sh` — git worktree helper for parallel feature development.
- `doc/proposal/proposal.md` — the authoritative spec for the project (game rules, architecture split, major tasks).
- `doc/manual/DE1-SoC_User_manual.md` — DE1-SoC board reference (pinouts, peripherals, codec, etc.).
- `doc/reference-design/lab3/` — the class-provided Lab 3 skeleton (VGA ball peripheral + Linux driver). Key files:
  - `hw/vga_ball.sv` — minimal SystemVerilog Avalon-MM peripheral example.
  - `hw/soc_system_top.sv`, `hw/soc_system.qsys`, `hw/Makefile` — the Quartus/Platform Designer flow.
  - `sw/vga_ball.c`, `sw/vga_ball.h`, `sw/hello.c`, `sw/Makefile` — kernel-module driver + userspace test program.
  - `description/description.md` — full write-up of how to wire a new peripheral into Platform Designer, generate the dtb, and load the module.
- `doc/reference-design/bubble-bobble/` — a prior team's full project (sprite/tile engine, audio mixer, USB gamepad game loop). Use this as the architectural reference for our sprite engine, line-buffer strategy, and HPS↔FPGA register layout. See `report.md` §2.1 (VGA/linebuffer), §4 (interfaces), and `code/` for working SystemVerilog, kernel modules, and a libusb game loop.

The outer directory (`../proposal/`, `../references/`, `../reports/`, `../scratch/`, `../worktrees/`) sits *outside* this git repo and is the user's personal workspace. Do not put project source files there.

## Architecture (from proposal)

Hard split between FPGA and HPS, communicating via memory-mapped registers on the lightweight HPS-to-FPGA bridge (base `0xff200000`):

- **FPGA (SystemVerilog):** VGA timing generator, double-buffered frame buffer (or line buffer + tile/sprite ROMs, bubble-bobble style), sprite blitting engine that reads a sprite table populated by software at vsync, audio sample streaming to the Wolfson codec, and an Avalon-MM agent exposing the register interface. Frame buffer bit depth (8-bit indexed vs 4-bit vs tile-based) is an open design decision — see proposal §Display.
- **HPS (Linux, C):** game loop, 5×9 grid state, plant/zombie/projectile logic, grid-level collision detection, sun economy, USB gamepad polling via libusb, and writes to the sprite table / audio registers each frame. The kernel driver exposes the Avalon registers as a misc device with `ioctl`s (follow the `vga_ball.c` pattern).

Every new hardware peripheral needs a matching Platform Designer component, a `set_module_assignment embeddedsw.dts.*` block in its `_hw.tcl` so it shows up in the generated dts as `compatible = "csee4840,<name>-1.0"`, and a kernel module whose `of_device_id` table matches that compatible string. This three-way tie (Verilog top-level, `_hw.tcl`, kernel driver) is the thing most likely to silently break.

### Gotchas worth remembering up front

- **Avalon address units follow `writedata` width.** If you widen `writedata` from 8 → 16 → 32 bits, the Avalon address port switches to word offsets, but the bus is still byte-addressed from the CPU side — the kernel driver must use byte offsets (`iowrite32` at `base + 4*N`), not word offsets. The bubble-bobble report §6 explicitly calls this out as something that cost them debugging time.
- **Shadow/active register latching at vsync** is how the lab3 ball avoids tearing (`vga_ball.sv` lines 36–63). Any register that affects what's drawn must be latched at `vcount == VACTIVE && hcount == 0`, not on the Avalon write.
- **BRAM reads are 1–2 cycles.** Disable registered output on memory IP or account for the extra cycle in pipelines — another bubble-bobble lesson (§6).
- **Linebuffer approach** (bubble-bobble): draw line N+1 into buffer B while displaying line N from buffer A, swap on hsync. Gives 1600 cycles per line to produce pixels and makes sprite transparency trivial. A full 640×480×8bpp frame buffer in on-chip SRAM is also viable but tighter on memory — pick deliberately, see proposal §Display.

## Build & deploy workflow

### CI/CD (GitHub Actions)

`.github/workflows/build.yml` runs on every push:
- **`build-sw`**: cross-compiles `pvz_driver.ko`, `pvz`, and test binaries for ARM using `arm-linux-gnueabihf-gcc` + kernel headers 4.19.0
- **`test-host`**: compiles and runs `test_game` natively (the only test with no hardware dependency)
- **`release`** (on `v*` tag push only): creates a GitHub Release with all ARM binaries + `deploy.sh` attached

FPGA artifacts (`.rbf`, `.dtb`) are built on Columbia workstations and uploaded manually: `gh release upload <tag> output_files/soc_system.rbf soc_system.dtb`

### Hardware (Quartus / Platform Designer), on a workstation:
```bash
cd hw/
make qsys      # regenerate Verilog from soc_system.qsys
make quartus   # full synthesis → output_files/soc_system.sof
make rbf       # convert .sof → .rbf for SD card
make dtb       # sopc2dts + dtc → soc_system.dtb (requires embedded_command_shell.sh in PATH)
```

### Software (cross-compile or on-board)

`sw/Makefile` supports cross-compilation via `CC`, `ARCH`, `CROSS_COMPILE`, and `KERNEL_SOURCE` variables:
```bash
cd sw/
make                          # on-board native build (default)
make CC=arm-linux-gnueabihf-gcc \
     ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \
     KERNEL_SOURCE=/path/to/headers   # cross-compile
```

### Board deployment

`deploy.sh` runs on the DE1-SoC via serial console:
```bash
./deploy.sh setup      # first-time: install kernel headers
./deploy.sh download   # download latest release from GitHub
./deploy.sh install    # load kernel module
./deploy.sh run        # start the game
./deploy.sh test <n>   # run a test (test_shapes, test_input, test_game)
./deploy.sh status     # show current state
```

### Development workflow (worktrees)

`worktree.sh` manages parallel feature branches:
```bash
./worktree.sh new <name>     # create ../worktrees/<name> + branch feature/<name>
./worktree.sh list           # list active worktrees
./worktree.sh clean <name>   # remove worktree + branch
```

### Useful on-board debugging:
```bash
echo 8 > /proc/sys/kernel/printk   # show kernel module pr_info output on console
dmesg | tail                        # or read it from dmesg
cat /proc/iomem                     # confirm the peripheral is mapped at ff20xxxx
ls /proc/device-tree/sopc@0/...     # confirm the dtb entry is present
```
The board's serial console is `screen /dev/ttyUSB0 115200` from the workstation over the mini-USB cable.
