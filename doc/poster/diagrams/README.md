# Poster diagrams

D2 source for the PvZ-FPGA project poster. Self-contained: the local
`_style.d2` (larger fonts than the slide variant) and `Makefile` keep
this directory independent of `doc/slides/diagrams/` so poster artifacts
can evolve separately.

## Index

| # | File | Topic |
|--:|------|-------|
| 01 | `01-system-block.d2` | Poster centerpiece — external I/O, HPS userspace + kernel driver, Avalon-MM bridge, and every implemented FPGA module. |

The diagram intentionally omits the Wolfson WM8731 audio codec because
audio is listed in the proposal but is not yet implemented in the
SystemVerilog or driver code.

## Rendering

```sh
make            # render every *.d2 to *.svg
make png        # also produce PNGs (for embedding in the poster)
make clean      # remove SVGs and PNGs
```

Requires [`d2`](https://d2lang.com) v0.7.1+ on `PATH`. The nested
`direction:` keyword is honored only at the top level in this version —
for 2-row / 2-column wrap layouts the diagrams use `grid-rows` /
`grid-columns` on a container.

## Conventions

`_style.d2` mirrors the slide-deck palette (see
`doc/slides/diagrams/README.md`) so poster and slides remain visually
consistent: amber for external I/O, light blue for HPS userspace, cream
for the kernel driver, mint for FPGA modules, cyan hexagon for the
Avalon-MM bridge, dashed slate for containment groups.
