# Plants vs Zombies on DE1-SoC

A simplified Plants vs. Zombies clone running on the Terasic DE1-SoC (Cyclone V). Final project for Columbia's CSEE 4840 Embedded Systems Design, Spring 2026.

The FPGA renders VGA output at 640x480@60 Hz using a hardware shape table, dual linebuffers, and a color-indexed palette. The HPS runs the game loop as a C program under Linux — managing the grid, plants, zombies, projectiles, sun economy, and keyboard input — and pushes shape descriptors to the FPGA each frame through a custom kernel driver and memory-mapped Avalon registers.

See [`doc/proposal/proposal.md`](doc/proposal/proposal.md) for the full design and [`doc/mvp_design_plan.md`](doc/mvp_design_plan.md) for the implementation plan.

## Current status

**Milestone 1 (MVP) — complete.** Single-level playable demo with primitive-shape graphics (rectangles, circles, 7-segment digits), keyboard control, one plant type (Peashooter), and five zombies. No sprites, audio, or USB gamepad yet. See [`doc/milestone1.md`](doc/milestone1.md) for a detailed writeup of what was built and how the system works.

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

## Building

See the [milestone 1 doc](doc/milestone1.md) for build instructions, or [`doc/reference-design/lab3/`](doc/reference-design/lab3/) for the underlying build flow.
