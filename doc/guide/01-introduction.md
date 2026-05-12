# 01 — Introduction

## What this project is

Plants vs Zombies on the Terasic DE1-SoC — a CSEE4840 Embedded Systems
final project. The board has a dual-core ARM Cortex-A9 (the "HPS") and
an Altera Cyclone V FPGA on the same chip. We run Linux on the ARM side
and host a custom GPU peripheral on the FPGA side. The two halves talk
through a memory-mapped Avalon-MM register file at physical address
`0xff200000`.

The end result is a playable lawn-defense game rendered at
640 × 480 @ 60 Hz on a VGA monitor, controlled from a USB keyboard or
Xbox 360-compatible gamepad plugged into the board.

## The game, in one paragraph

The lawn is a 4-row × 8-column grid (`hw/bg_grid.sv:28-31`,
`sw/pvz.h:29-33`). You start with 100 sun (`sw/game.h:41`). Zombies
spawn from the right edge every 8–15 seconds (`sw/game.h:30-31`) and
shamble leftward; each one needs 3 pea-shots to kill
(`sw/game.h:25`). You spend sun to plant either a Peashooter (50 sun,
fires every 2 s) or a Sunflower (50 sun, boosts passive sun income).
Cursor moves with the arrow keys / D-pad; **Space / A** plants;
**D / B** removes; **Tab / shoulder buttons** flips between the two
plant types; **Esc / Start** quits. You win when all 5 zombies are
dead and lose when any zombie reaches the left edge.

## What is on this branch (v5-cursor-controller)

This guide documents **only the code currently committed on this
branch**. That code is:

| Area | Files |
| ---- | ----- |
| FPGA RTL | `hw/pvz_top.sv`, `entity_drawer.sv`, `vga_counters.sv`, `bg_grid.sv`, `sprite_rom.sv`, `color_palette.sv` |
| FPGA art | `hw/peashooter_idx.mem`, `sunflower_idx.mem`, `zombie_idx.mem`, `peas_idx.mem` (`peas_idx.mem` is committed but the engine draws peas procedurally, not from this ROM) |
| FPGA integration | `hw/pvz_top_hw.tcl`, `soc_system.qsys`, `soc_system_top.sv`, `Makefile` |
| Linux kernel | `sw/pvz_driver.c`, `sw/pvz_driver.h` (compiles to `pvz_driver.ko`) |
| Userspace game | `sw/main.c`, `sw/game.c`, `sw/render.c`, `sw/input.c` plus headers (compiles to `pvz`) |
| Shared between kernel and userspace | `sw/pvz.h` |

Things mentioned in `CLAUDE.md`, `README.md`, `doc/proposal/proposal.md`,
and `doc/milestone1.md` that **do not exist on this branch** and are
therefore out of scope for this guide:

- `deploy.sh`, `worktree.sh`, `.github/workflows/build.yml` — no CI/CD
  in this branch; you build by hand
- `sw/test/test_shapes`, `sw/test/test_input`, `sw/test/test_game` —
  no test programs in `sw/`
- The earlier "shape-table" hardware engine — replaced by the
  sprite-based engine documented here
- Audio output via the Wolfson codec — no audio modules
- libusb gamepad polling — input is via Linux evdev (`/dev/input/eventX`)
- Double-buffered frame buffer — replaced by the "racing the beam"
  combinational compositor in `entity_drawer.sv`

If you find a reference to one of those elsewhere in the repo, treat
it as historical or aspirational, not as the current design.

## How to read this guide

You can read it cover to cover (recommended for new members), or jump
to a specific chapter:

1. **02 — Background** if you have not worked with FPGAs or Linux on
   embedded boards before. Defines every term used later.
2. **03 — Architecture** for the big picture: who owns what between
   HPS and FPGA, how they communicate, and how a frame is produced.
3. **04 — Hardware deep dive** for the FPGA half, one source file at
   a time, with line citations.
4. **05 — Software deep dive** for the HPS half, same treatment.
5. **06 — Register map reference** when you need exact bit layouts.
6. **07 — Frame walkthrough** when you want to see how a button press
   becomes a pixel.
7. **08 — Build and run** when you want to make a change and try it
   on the board.
8. **09 — References and glossary** for outside materials, a flat file
   index, and definitions.
