# Plants vs Zombies on DE1-SoC

A simplified Plants vs Zombies clone running on the Terasic DE1-SoC (Cyclone V). Final project for Columbia's CSEE 4840 Embedded Systems Design, Spring 2026.

The FPGA handles VGA output, sprite blitting, and audio streaming to the Wolfson codec. The HPS runs the game loop (grid state, plants, zombies, sun economy, USB gamepad input) as a C program under Linux, talking to the hardware through a memory-mapped register interface and a custom kernel driver.

See [`doc/proposal/proposal.md`](doc/proposal/proposal.md) for the full design.

## Repository layout

- `doc/proposal/` — project proposal (authoritative spec)
- `doc/manual/` — DE1-SoC user manual
- `doc/reference-design/lab3/` — class-provided Lab 3 skeleton (VGA ball peripheral + Linux driver), used as the build-system template
- `doc/reference-design/bubble-bobble/` — a prior team's full project, referenced for the sprite engine and HPS/FPGA interface design
