# Presentation diagrams

Simplified D2 diagrams for the final-presentation slides.
Each diagram targets a single 16:9 slide and shows only
the load-bearing pieces — no file:line refs, no bit-widths.

For the dense reference diagrams (signal-level, ER, full
signal cheatsheets) see `doc/diagrams/`. For the onboarding
guide diagrams see `doc/guide/diagrams/`.

## Index

01–09 mirror `doc/guide/diagrams/` (simplified); 10–15 are
picks from `doc/diagrams/` rewritten for slide use.

| # | File | Slide topic |
|--:|------|-------------|
| 01 | `01-system-block.d2`     | Board-level architecture (USB, HPS, bridge, FPGA, VGA) |
| 02 | `02-hw-hierarchy.d2`     | SystemVerilog module tree under `pvz_top` |
| 03 | `03-vga-timing.d2`       | VGA 640x480 @ 60 Hz scan-line + frame structure |
| 04 | `04-sprite-rom.d2`       | Pixel coord → ROM addr → palette index → RGB |
| 05 | `05-entity-layers.d2`    | `entity_drawer` compositing layers (low → high) |
| 06 | `06-sw-modules.d2`       | Userspace + kernel module dependencies |
| 07 | `07-frame-flow.d2`       | End-to-end data path for one frame |
| 08 | `08-register-map.d2`     | 51-word Avalon-MM register file layout |
| 09 | `09-build-pipeline.d2`   | Sources → tools → artifacts → board |
| 10 | `10-memory-map.d2`       | HPS address space + `pvz_top` register window |
| 11 | `11-drawer-pipeline.d2`  | `entity_drawer` 2-stage pipeline |
| 12 | `12-game-update.d2`      | `game_update()` per-tick phase order |
| 13 | `13-game-fsm.d2`         | Game loop state machine (PLAYING / WIN / LOSE) |
| 14 | `14-ioctl-path.d2`       | `ioctl(PVZ_WRITE_REG)` userland → Avalon |
| 15 | `15-place-plant.d2`      | Place-plant scenario (SPACE → grid → draw) |

## Conventions

`_style.d2` defines shared classes; every diagram spreads it
in with `...@_style.d2` on its first line. Roles:

| Class | Color (fill / stroke) | Used for |
|-------|-----------------------|----------|
| `ext`   | yellow / amber  | Off-chip / external (USB, monitor, SD) |
| `hps`   | light blue / navy | HPS userspace code |
| `krn`   | cream / amber   | Linux kernel / driver code |
| `hw`    | mint / green    | FPGA SystemVerilog modules |
| `bus`   | cyan hexagon    | Avalon-MM / interconnect |
| `reg`   | slate           | Register words, raw data |
| `mark`  | pink            | Highlight / caveat |
| `flow`  | violet oval     | FSM state / decision |
| `group` | dashed slate    | Containment |
| `note`  | italic gray text| Annotation |

## Rendering

```sh
make            # render every *.d2 to *.svg
make png        # also produce PNGs (for embedding in slides)
make clean      # remove SVGs and PNGs
```

Requires [`d2`](https://d2lang.com) v0.7.1+ on `PATH`. The
nested `direction:` keyword is honored only at the top level
in this version — for 2-row / 2-column wrap layouts the
diagrams use `grid-rows` / `grid-columns` on a container.
