# PvZ FPGA Onboarding Guide

This is the entry point for new members of the Plants vs Zombies
FPGA project on the Terasic DE1-SoC. It documents the code on the
current branch (`v5-cursor-controller`) as of May 2026.

If you're brand new to the project, read the chapters in order. If
you already know your way around embedded systems, skip ahead to
chapter 03 (architecture) and dip into the deep-dive chapters by
file name.

## Chapters

1. [**Introduction**](01-introduction.md) — What this project is,
   what the game looks like, what's on this branch.
2. [**Background**](02-background.md) — DE1-SoC, FPGA vs HPS, Linux
   on ARM, Avalon-MM, VGA, devicetree, the toolchain. For readers
   without embedded experience.
3. [**Architecture**](03-architecture.md) — HPS / FPGA split, the
   single shared register file, end-to-end frame flow.
4. [**Hardware deep dive**](04-hardware.md) — every file under
   `hw/`, with line citations.
5. [**Software deep dive**](05-software.md) — every file under
   `sw/`, with line citations.
6. [**Register map reference**](06-register-map.md) — exhaustive
   bit-level reference for the 51-word Avalon-MM register file.
7. [**Frame walkthrough**](07-frame-walkthrough.md) — end-to-end
   data flow for a single button press, from USB to pixel.
8. [**Build and run**](08-build-and-run.md) — how to compile
   hardware, compile software, deploy to the board, debug.
9. [**References and glossary**](09-references-and-glossary.md) —
   external references, flat file index, definitions.

## Diagrams

All diagrams are under [`diagrams/`](diagrams/). Both D2 source and
rendered SVG are committed. To regenerate (requires
[d2](https://d2lang.com/)):

```bash
cd doc/guide/diagrams
for f in *.d2; do d2 "$f" "${f%.d2}.svg"; done
```

## Suggested reading paths

- **You've never touched FPGAs or embedded Linux.** Read everything
  in order. Plan a couple of sittings; chapters 04 and 05 are dense.
- **You've done CSEE4840 lab 3 but nothing else with this repo.**
  Skim 01, skip 02, read 03 carefully, then 04 and 05. Reference
  06 as needed.
- **You're back after a break.** Re-read 03 and 07 to refresh the
  data flow, then look up your file of interest in the index in
  chapter 09.
- **You're about to make a hardware change.** Re-read 03 + 04 +
  06, and look at the "gotchas" section in the bubble-bobble report
  (linked from chapter 09).
- **You're about to make a software change.** 03 + 05 + 06 are
  enough.

## What this guide does NOT cover

The repo's `CLAUDE.md` and `README.md` mention features that are
not yet built on this branch: CI/CD pipeline, `deploy.sh`,
`worktree.sh`, on-board test programs (`test_shapes`, `test_input`,
`test_game`), audio output, libusb gamepad polling, double-buffered
frame buffer, and the older "shape-table" hardware engine described
in `doc/milestone1.md`. None of those exist in the current code, so
none are documented here.
