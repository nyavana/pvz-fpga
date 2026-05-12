# PvZ-on-DE1-SoC — D2 System Diagram Catalog Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Spec:** `docs/superpowers/specs/2026-05-12-system-diagrams-design.md`

**Goal:** Produce 35 D2 diagrams (`.d2` + `.svg` + `.png` each) plus shared scaffolding (`common.d2`, `Makefile`, `README.md`) documenting `v5-cursor-controller/` end-to-end for a multi-hour final design review.

**Architecture:** All files live in `doc/diagrams/`, sub-foldered by section. Every diagram imports a shared `common.d2` for consistent classes. A top-level `Makefile` renders every `.d2` to both SVG and PNG via the `d2` CLI. Work is phased so the four "hero" diagrams (system context, FPGA pipeline, register map, one-frame flow) land first and can be reviewed before the long tail.

**Tech Stack:** D2 v0.7.1 (already installed at `/home/linuxbrew/.linuxbrew/bin/d2`), GNU make, git. No language runtime; pure text + invoked binary.

---

## Working directory convention

Every shell snippet in this plan assumes **the working directory is the `v5-cursor-controller` worktree root** (`.../pvz-fpga/v5-cursor-controller`). The repo uses a bare-repo + per-branch worktree layout, so git commands MUST be run from inside the worktree, not the parent. To make commands portable, an implementer may set up once:

```bash
REPO=/home/nyavana/columbia/4840/pvz-fpga/v5-cursor-controller
cd "$REPO"
```

All `d2` and `git` invocations below use **repo-relative paths** (e.g. `doc/diagrams/01-system/01_context.d2`), never an absolute path or a `v5-cursor-controller/` prefix.

## Conventions used in every diagram task

Every diagram `.d2` file MUST begin with this header comment, with the
placeholders filled in:

```
# Title:       <short title>
# Section:     <01-system | 02-hw | 03-sw | 04-interface | 05-cross-cutting | 06-flow>
# Documents:   <source files; e.g., hw/entity_drawer.sv, sw/render.c>
# Defends:     <likely defense question this diagram answers>
# Detail:      <block | signal>
```

Every diagram MUST start with `...@../common.d2` (NOTE: no colon — `...: @file` is rejected by D2 0.7.1; the correct spread-import syntax is `...@file`) so it picks up the shared class definitions. From a section subfolder (one level below `doc/diagrams/`) the import path is `...@../common.d2`.

Every diagram task ends with the same three-step cadence (run from the `v5-cursor-controller` worktree root):

1. **Write the `.d2` file** with the spec'd content.
2. **Render and verify**:
   ```bash
   d2 --layout=elk doc/diagrams/<section>/<file>.d2 doc/diagrams/<section>/<file>.svg
   d2 --layout=elk doc/diagrams/<section>/<file>.d2 doc/diagrams/<section>/<file>.png
   ```
   Expected: both commands exit 0, both files appear next to the source. If `--layout=elk` warns "ELK not available" use the default `dagre` layout (still exits 0).
3. **Commit** the source plus the two rendered outputs:
   ```bash
   git add doc/diagrams/<section>/<file>.d2 doc/diagrams/<section>/<file>.svg doc/diagrams/<section>/<file>.png
   git -c commit.gpgsign=false commit -m "docs(diagrams): add <short-title>"
   ```

Where a diagram task lists a "Structured spec" instead of full D2 source, the implementer translates the listed `Nodes`, `Containers`, `Edges`, and `Style` into D2 mechanically. The hero diagrams in Phase 1 supply concrete D2 examples that demonstrate the translation; later phases follow the same idiom.

---

## Phase 0 — Scaffolding (4 tasks)

### Task 0.1: Verify d2 toolchain and create folder structure

**Files:**
- Create: `doc/diagrams/` (and all section subfolders)

- [ ] **Step 1: Verify d2 is on PATH**

```bash
d2 --version
```
Expected: `0.7.1` or newer.

- [ ] **Step 2: Create the folder tree**

```bash
mkdir -p doc/diagrams/{01-system,02-hw,03-sw,04-interface,05-cross-cutting,06-flow}
```

- [ ] **Step 3: Verify folders exist**

```bash
ls -1 doc/diagrams/
```
Expected: `01-system  02-hw  03-sw  04-interface  05-cross-cutting  06-flow`.

(No commit yet — empty folders aren't tracked by git. The first commit will land with Task 0.2.)

### Task 0.2: Write `common.d2` (shared styles)

**Files:**
- Create: `doc/diagrams/common.d2`

- [ ] **Step 1: Write the file**

```d2
# common.d2 — shared classes for the PvZ-FPGA diagram catalog.
# Imported by every other .d2 via `...@../common.d2` from a section
# subfolder.  (NOTE: D2 0.7.1 rejects `...: @file`; use `...@file`.)

classes: {
  hw_module: {
    shape: rectangle
    style: {
      fill: "#dbeafe"      # light blue
      stroke: "#1d4ed8"
      stroke-width: 2
      font-color: "#0b1d51"
      border-radius: 8
    }
  }
  sw_module: {
    shape: rectangle
    style: {
      fill: "#dcfce7"      # light green
      stroke: "#15803d"
      stroke-width: 2
      font-color: "#0a3d0a"
      border-radius: 8
    }
  }
  kernel_module: {
    shape: rectangle
    style: {
      fill: "#fde68a"      # amber
      stroke: "#b45309"
      stroke-width: 2
      font-color: "#3b1d04"
      border-radius: 8
    }
  }
  register: {
    shape: rectangle
    style: {
      fill: "#fee2e2"      # rose
      stroke: "#b91c1c"
      stroke-width: 1
      font-color: "#3b0a0a"
      font-size: 12
    }
  }
  signal: {
    shape: rectangle
    style: {
      fill: "#f5f5f4"
      stroke: "#525252"
      stroke-dash: 0
      stroke-width: 1
      font-size: 11
      italic: true
    }
  }
  bus: {
    shape: rectangle
    style: {
      fill: "#e0e7ff"
      stroke: "#3730a3"
      stroke-width: 2
      multiple: true
      font-color: "#1e1b4b"
    }
  }
  fsm_state: {
    shape: circle
    style: {
      fill: "#fef3c7"
      stroke: "#b45309"
      stroke-width: 2
      font-color: "#3b1d04"
    }
  }
  external: {
    shape: rectangle
    style: {
      fill: "#f5f5f4"
      stroke: "#404040"
      stroke-width: 2
      stroke-dash: 4
      font-color: "#1c1917"
    }
  }
  note: {
    shape: text
    style: {
      font-size: 11
      italic: true
      font-color: "#525252"
    }
  }
  layer: {
    # Used inside containers to group ordered pipeline stages.
    shape: rectangle
    style: {
      fill: "#f1f5f9"
      stroke: "#0f172a"
      stroke-width: 1
      border-radius: 4
    }
  }
}

# Standard edge label style for "this is a bus, width N".
# Apply at the diagram level with: `* -> *: foo {style.font-size: 11}`.
```

- [ ] **Step 2: Sanity-render**

```bash
# A "classes only" file has no nodes; create a trivial probe.
# Import points at our committed common.d2 by absolute path so the
# probe works regardless of cwd.
cat > /tmp/_probe.d2 <<EOF
...@$(pwd)/doc/diagrams/common.d2
probe.class: hw_module
EOF
d2 /tmp/_probe.d2 /tmp/_probe.svg && rm /tmp/_probe.d2 /tmp/_probe.svg
```
Expected: exit 0, no errors. If d2 reports an unknown class it means the import path is wrong — re-check.

- [ ] **Step 3: Commit**

```bash
git add doc/diagrams/common.d2
git -c commit.gpgsign=false commit -m "docs(diagrams): add shared common.d2 styles"
```

### Task 0.3: Write `Makefile`

**Files:**
- Create: `doc/diagrams/Makefile`

- [ ] **Step 1: Write the Makefile**

```make
# Render every .d2 source under this tree to a sibling .svg and .png.
# Usage:
#   make all           render everything
#   make clean         remove generated svg/png
#   make 01-system     render only one section (works for any section dir)
#   make watch         live-render with d2 watch (manual one-file mode)

D2          ?= d2
LAYOUT      ?= elk

# Find every .d2 except common.d2 (which has no nodes of its own).
SOURCES     := $(shell find . -name '*.d2' ! -name 'common.d2' | sort)
SVGS        := $(SOURCES:.d2=.svg)
PNGS        := $(SOURCES:.d2=.png)

.PHONY: all clean watch help check-d2 \
        01-system 02-hw 03-sw 04-interface 05-cross-cutting 06-flow

all: check-d2 $(SVGS) $(PNGS)

%.svg: %.d2 common.d2
	$(D2) --layout=$(LAYOUT) $< $@

%.png: %.d2 common.d2
	$(D2) --layout=$(LAYOUT) $< $@

01-system 02-hw 03-sw 04-interface 05-cross-cutting 06-flow: check-d2
	@for f in $@/*.d2; do \
	  echo "[d2] $$f"; \
	  $(D2) --layout=$(LAYOUT) $$f $${f%.d2}.svg || exit 1; \
	  $(D2) --layout=$(LAYOUT) $$f $${f%.d2}.png || exit 1; \
	done

watch:
	@echo "Usage: $(D2) --watch <file>.d2 <file>.svg" >&2; \
	echo "(make watch is informational; pick one file to live-edit.)" >&2

clean:
	@find . \( -name '*.svg' -o -name '*.png' \) -type f -print -delete

check-d2:
	@command -v $(D2) >/dev/null 2>&1 || { \
	  echo "ERROR: '$(D2)' not found in PATH. Install d2:" >&2; \
	  echo "  curl -fsSL https://d2lang.com/install.sh | sh -s --" >&2; \
	  exit 127; }

help:
	@echo "Targets: all clean watch help check-d2"
	@echo "Also: any of: 01-system 02-hw 03-sw 04-interface 05-cross-cutting 06-flow"
```

- [ ] **Step 2: Smoke-test `make check-d2`**

```bash
make -C doc/diagrams check-d2
```
Expected: silent success (exit 0).

- [ ] **Step 3: Smoke-test `make all` (no `.d2` files yet — should noop cleanly)**

```bash
make -C doc/diagrams all
```
Expected: `check-d2` passes, then make reports nothing to do (no `.d2` matches found since only `common.d2` exists — and it's filtered out). Exit 0.

- [ ] **Step 4: Commit**

```bash
git add doc/diagrams/Makefile
git -c commit.gpgsign=false commit -m "docs(diagrams): add Makefile to render all .d2 to SVG+PNG"
```

### Task 0.4: Write `README.md` (catalog index stub)

**Files:**
- Create: `doc/diagrams/README.md`

- [ ] **Step 1: Write the README**

```markdown
# PvZ-FPGA Diagram Catalog

D2 source diagrams documenting `v5-cursor-controller/` for the final
design review. Spec: `docs/superpowers/specs/2026-05-12-system-diagrams-design.md`.

## Prerequisites

- `d2` v0.7.1 or newer in `PATH`. Install: `curl -fsSL https://d2lang.com/install.sh | sh -s --`.
- GNU `make`.

## Render

```bash
make all              # render every .d2 to a sibling .svg and .png
make 02-hw            # render only one section
make clean            # remove generated svg/png (keeps .d2)
```

## Catalog

Each row links to the `.d2` source (the `.svg` and `.png` sit next to it).

### 01-system — System / context (4)

| # | File | What it shows |
|---|------|---------------|
| 1 | [01_context.d2](01-system/01_context.d2) | User, USB keyboard, DE1-SoC, VGA monitor, serial console host. |
| 2 | [02_hw_sw_boundary.d2](01-system/02_hw_sw_boundary.d2) | HPS-Linux ↔ FPGA fabric swim lanes with each named module placed. |
| 3 | [03_soc_block.d2](01-system/03_soc_block.d2) | Cyclone V SoC internals (cores, L2, DDR3, bridges, FPGA tile). |
| 4 | [04_memory_map.d2](01-system/04_memory_map.d2) | Memory regions; `pvz_top` slave window at `0xFF20_0000`. |

### 02-hw — Hardware tier (9)

| # | File | What it shows |
|---|------|---------------|
| 5 | [01_fpga_top.d2](02-hw/01_fpga_top.d2) | Containment tree: `soc_system_top` → `soc_system` → `pvz_top`. |
| 6 | [02_pvz_top_block.d2](02-hw/02_pvz_top_block.d2) | Inside `pvz_top`: register file + submodules. |
| 7 | [03_vga_counters.d2](02-hw/03_vga_counters.d2) | `hcount`/`vcount` counters, sync windows. |
| 8 | [04_bg_grid.d2](02-hw/04_bg_grid.d2) | `bg_grid` (px,py) → cell → checker color. |
| 9 | [05_sprite_rom.d2](02-hw/05_sprite_rom.d2) | `sprite_rom` + the three `*_idx.mem` files. |
| 10 | [06_entity_drawer_pipeline.d2](02-hw/06_entity_drawer_pipeline.d2) | 7-layer merge, 2-stage pipeline. |
| 11 | [07_entity_drawer_signals.d2](02-hw/07_entity_drawer_signals.d2) | Signal-level hot spot. |
| 12 | [08_color_palette.d2](02-hw/08_color_palette.d2) | 256-entry 8 → 24-bit RGB LUT. |
| 13 | [09_pvz_top_regfile_signals.d2](02-hw/09_pvz_top_regfile_signals.d2) | Avalon decoder, field-level updates, no vsync latching. |

### 03-sw — Software tier (7)

| # | File | What it shows |
|---|------|---------------|
| 14 | [01_process_architecture.d2](03-sw/01_process_architecture.d2) | `pvz` binary + driver + open FDs. |
| 15 | [02_kernel_driver.d2](03-sw/02_kernel_driver.d2) | Driver init + ioctl path. |
| 16 | [03_game_state_er.d2](03-sw/03_game_state_er.d2) | `game_state_t` data shape. |
| 17 | [04_game_loop_fsm.d2](03-sw/04_game_loop_fsm.d2) | INIT → PLAYING → WIN/LOSE. |
| 18 | [05_game_update_flow.d2](03-sw/05_game_update_flow.d2) | Order of phases inside `game_update()` (sw/game.c:288). |
| 19 | [06_input_pipeline.d2](03-sw/06_input_pipeline.d2) | `/dev/input/eventN` → action. |
| 20 | [07_render_mapping.d2](03-sw/07_render_mapping.d2) | Game-state field → FPGA register word. |

### 04-interface — HW/SW interface (4)

| # | File | What it shows |
|---|------|---------------|
| 21 | [01_avalon_bus.d2](04-interface/01_avalon_bus.d2) | HPS ↔ bridge ↔ `pvz_top` topology. |
| 22 | [02_ioctl_path.d2](04-interface/02_ioctl_path.d2) | Sequence of one register write end-to-end. |
| 23 | [03_register_map_cheatsheet.d2](04-interface/03_register_map_cheatsheet.d2) | All 51 words, bit layouts, writers/consumers. |
| 24 | [04_device_tree_binding.d2](04-interface/04_device_tree_binding.d2) | `_hw.tcl` → sopc2dts → dtb → driver `of_match`. |

### 05-cross-cutting — Timing / build / boot (5)

| # | File | What it shows |
|---|------|---------------|
| 25 | [01_frame_timing.d2](05-cross-cutting/01_frame_timing.d2) | 60 Hz tick budget; tearing window. |
| 26 | [02_vga_timing.d2](05-cross-cutting/02_vga_timing.d2) | 640×480@60 Hz waveforms. |
| 27 | [03_pixel_pipeline_timing.d2](05-cross-cutting/03_pixel_pipeline_timing.d2) | Entity drawer 2-cycle pipeline. |
| 28 | [04_build_pipeline.d2](05-cross-cutting/04_build_pipeline.d2) | HW (Quartus) + SW (Make) pipelines joining at SD card. |
| 29 | [05_boot_flow.d2](05-cross-cutting/05_boot_flow.d2) | Cold boot: preloader → U-Boot → `fpga load` → Linux → `insmod`. |

### 06-flow — Cross-layer flows (6)

| # | File | What it shows |
|---|------|---------------|
| 30 | [01_one_frame.d2](06-flow/01_one_frame.d2) | End-to-end frame: key → game → registers → pixel. |
| 31 | [02_place_plant.d2](06-flow/02_place_plant.d2) | TAB → SPACE → grid bit set → HUD update. |
| 32 | [03_pea_zombie_collision.d2](06-flow/03_pea_zombie_collision.d2) | Pea hits zombie → HP-- → dead. |
| 33 | [04_zombie_eats_plant.d2](06-flow/04_zombie_eats_plant.d2) | Zombie enters cell → eat → plant removed. |
| 34 | [05_sun_economy.d2](06-flow/05_sun_economy.d2) | 480-frame tick → sun += 25. |
| 35 | [06_insmod_probe.d2](06-flow/06_insmod_probe.d2) | `insmod` → DT walk → `probe` → `/dev/pvz`. |
```

- [ ] **Step 2: Commit**

```bash
git add doc/diagrams/README.md
git -c commit.gpgsign=false commit -m "docs(diagrams): add catalog README index"
```

---

## Phase 1 — Hero diagrams (4 tasks, review gate)

After this phase, render `make all` and stop for a review of the first
four diagrams before continuing.

### Task 1.1: `01-system/01_context.d2` — system context

**Files:**
- Create: `doc/diagrams/01-system/01_context.d2`
- Read for grounding: `sw/main.c` (controls printed at startup, lines 103-105), `doc/guide/01-introduction.md`

- [ ] **Step 1: Write the file**

```d2
# Title:       System context
# Section:     01-system
# Documents:   sw/main.c (controls), doc/guide/01-introduction.md
# Defends:     "What's connected to the board and what does the user touch?"
# Detail:      block

...@../common.d2

direction: down

user.class: external
user.label: "Player"
keyboard.class: external
keyboard.label: "USB keyboard\n(arrows, SPACE, TAB, D, ESC)"
monitor.class: external
monitor.label: "VGA monitor\n640x480 @ 60 Hz"

board: {
  label: "DE1-SoC board\n(Cyclone V SoC: HPS + FPGA)"
  class: hw_module
  hps: {
    label: "HPS\nARM Cortex-A9 dual\nLinux + ./pvz"
    class: sw_module
  }
  fpga: {
    label: "FPGA fabric\nVGA pipeline\n(pvz_top peripheral)"
    class: hw_module
  }
  hps -> fpga: "Avalon-MM\nregister writes"
}

dev_host.class: external
dev_host.label: "Workstation\n(serial console via ttyUSB0)"

user -> keyboard: "presses keys"
keyboard -> board.hps: "USB HID\n/dev/input/eventN"
board.fpga -> monitor: "VGA R/G/B,\nHS/VS, CLK"
dev_host -> board.hps: "screen /dev/ttyUSB0 115200\n(debug only)"
```

- [ ] **Step 2: Render and verify** (per the standard cadence, run from worktree root).

```bash
d2 --layout=elk doc/diagrams/01-system/01_context.d2 doc/diagrams/01-system/01_context.svg
d2 --layout=elk doc/diagrams/01-system/01_context.d2 doc/diagrams/01-system/01_context.png
```
Expected: both files written, exit 0.

- [ ] **Step 3: Commit**

```bash
git add doc/diagrams/01-system/01_context.d2 \
        doc/diagrams/01-system/01_context.svg \
        doc/diagrams/01-system/01_context.png
git -c commit.gpgsign=false commit -m "docs(diagrams): add system context (1/35)"
```

### Task 1.2: `02-hw/02_pvz_top_block.d2` — FPGA pipeline

**Files:**
- Create: `doc/diagrams/02-hw/02_pvz_top_block.d2`
- Read for grounding: `hw/pvz_top.sv:30-255`

- [ ] **Step 1: Write the file**

```d2
# Title:       pvz_top block diagram (FPGA pipeline)
# Section:     02-hw
# Documents:   hw/pvz_top.sv
# Defends:     "Walk us through the FPGA side at a high level."
# Detail:      block

...@../common.d2

direction: right

avalon: {
  label: "Avalon-MM slave\naddress[5:0]\nwritedata[31:0]\nwrite, chipselect"
  class: bus
}

pvz_top: {
  label: "pvz_top"
  class: hw_module

  regfile: {
    label: "Register file\n51 x 32-bit words\nplant_present, sunflower_present,\nzombie[8], pea[8], cursor, sun, selected"
    class: register
  }
  vga: {
    label: "vga_counters\nhcount[10:0], vcount[9:0]\nVGA sync"
    class: hw_module
  }
  bg: {
    label: "bg_grid\npx,py -> bg_color[7:0]"
    class: hw_module
  }
  rom_plant: {
    label: "sprite_rom\npeashooter_idx.mem\n64x64, 1-cycle latency"
    class: hw_module
  }
  rom_sun: {
    label: "sprite_rom\nsunflower_idx.mem\n(shares plant_addr)"
    class: hw_module
  }
  rom_z: {
    label: "sprite_rom\nzombie_idx.mem"
    class: hw_module
  }
  drawer: {
    label: "entity_drawer\n7-layer merge,\n2-stage pipeline"
    class: hw_module
  }
  pal: {
    label: "color_palette\nindex[7:0] -> R,G,B"
    class: hw_module
  }
  blank_mux: {
    label: "blank mux\nVGA_BLANK_n gate"
    class: layer
  }

  vga -> bg: "px,py"
  vga -> drawer: "px,py"
  bg -> drawer: "bg_color[7:0]"
  regfile -> drawer: "entity registers\n(packed)"
  drawer -> rom_plant: "plant_rd_addr[11:0]"
  rom_plant -> drawer: "plant_rd_pixel[7:0]"
  rom_sun -> drawer: "sunflower_rd_pixel[7:0]"
  drawer -> rom_z: "zombie_rd_addr[11:0]"
  rom_z -> drawer: "zombie_rd_pixel[7:0]"
  drawer -> pal: "color_out[7:0]"
  pal -> blank_mux: "R,G,B [7:0]"
  vga -> blank_mux: "VGA_BLANK_n,\nVGA_CLK, HS, VS"
}

avalon -> pvz_top.regfile

vga_pins: {
  label: "VGA pins\nR/G/B [7:0],\nHS, VS, CLK, BLANK_n, SYNC_n"
  class: external
}
pvz_top.blank_mux -> vga_pins
```

- [ ] **Step 2: Render and verify** — as the standard cadence (from worktree root, `doc/diagrams/02-hw/02_pvz_top_block.d2` → `.svg` and `.png`).

- [ ] **Step 3: Commit** — message: `docs(diagrams): add pvz_top block (2/35)`.

### Task 1.3: `04-interface/03_register_map_cheatsheet.d2` — register map

**Files:**
- Create: `doc/diagrams/04-interface/03_register_map_cheatsheet.d2`
- Read for grounding: `hw/pvz_top.sv:8-28, 94-148`, `sw/pvz.h:41-49`, `doc/guide/06-register-map.md:143-160` (alias gap).

- [ ] **Step 1: Write the file**

```d2
# Title:       Avalon register-map cheat sheet
# Section:     04-interface
# Documents:   hw/pvz_top.sv (register decoder), sw/pvz.h (PVZ_REG_* macros)
# Defends:     "Show me exactly where the cursor position lives in HW."
# Detail:      signal

...@../common.d2

grid-rows: 1
direction: right

regs: {
  grid-columns: 1
  label: "Register file (51 words, 4 B each, base 0xFF20_0000 + pvz_top offset)"

  w00: { label: "0  PLANTS\nbits[31:0] = peashooter at cell i (i = row*8+col)"; class: register }
  w01: { label: "1  SUNFLOWER\nbits[31:0] = sunflower at cell i"; class: register }
  w_alias: { label: "2..31  (alias trap)\nDecoder chain in pvz_top.sv:122 is\n  else if (address < 6'd40)\nso words 2..31 also write the zombie slot at\n  zombie[address[2:0]].\nSoftware contract reserves these — never write them."; class: note }
  w32: { label: "32..39  ZOMBIE[0..7]\nbit 31 = alive\nbits [11:10] = row (0..3)\nbits [9:0]   = x_pixel (0..639)"; class: register }
  w40: { label: "40..47  PEA[0..7]\nsame encoding as ZOMBIE"; class: register }
  w48: { label: "48  CURSOR\nbit 31 = visible\nbits [4:2] = col (0..7)\nbits [1:0] = row (0..3)"; class: register }
  w49: { label: "49  SUN\nbits [13:0] = sun count"; class: register }
  w50: { label: "50  SELECTED\nbits [1:0] = 0=peashooter, 1=sunflower"; class: register }
}

writers: {
  grid-columns: 1
  label: "Software writers (sw/render.c)"
  rp: { label: "render_plants() -> PLANTS, SUNFLOWER"; class: sw_module }
  rz: { label: "render_zombies() -> ZOMBIE[0..7]"; class: sw_module }
  ra: { label: "render_peas() -> PEA[0..7]"; class: sw_module }
  rc: { label: "render_cursor_sun_selected() -> CURSOR, SUN, SELECTED"; class: sw_module }
}

consumers: {
  grid-columns: 1
  label: "Hardware consumers (hw/entity_drawer.sv)"
  cp:  { label: "plant_present, sunflower_present -> plant/sunflower layer"; class: hw_module }
  cz:  { label: "zombie_alive, zombie_x_packed, zombie_row_packed -> zombie layer"; class: hw_module }
  ca:  { label: "pea_alive, pea_x_packed, pea_row_packed -> pea layer"; class: hw_module }
  cc:  { label: "cursor_visible, cursor_col, cursor_row -> cursor layer"; class: hw_module }
  cs:  { label: "sun_value -> sun HUD layer"; class: hw_module }
  cse: { label: "selected_plant -> selector border mux"; class: hw_module }
}

writers.rp -> regs.w00
writers.rp -> regs.w01
writers.rz -> regs.w32
writers.ra -> regs.w40
writers.rc -> regs.w48
writers.rc -> regs.w49
writers.rc -> regs.w50

regs.w00 -> consumers.cp
regs.w01 -> consumers.cp
regs.w32 -> consumers.cz
regs.w40 -> consumers.ca
regs.w48 -> consumers.cc
regs.w49 -> consumers.cs
regs.w50 -> consumers.cse

note1.class: note
note1.label: "Avalon `addressUnits = WORDS`. CPU writes byte offset N -> Avalon address = N >> 2.\nDriver uses `iowrite32(virt_base + word_index*4, value)`."

note2.class: note
note2.label: "No vsync latching: writes take effect on the next clk edge,\nso a write that races the scan can produce one frame of tearing."
```

- [ ] **Step 2: Render and verify** — standard cadence.

- [ ] **Step 3: Commit** — message: `docs(diagrams): add register-map cheat sheet (3/35)`.

### Task 1.4: `06-flow/01_one_frame.d2` — one-frame end-to-end

**Files:**
- Create: `doc/diagrams/06-flow/01_one_frame.d2`
- Read for grounding: `sw/main.c:107-150` (main loop body), `sw/main.c:34-68` (`process_input`), `sw/game.c:288-302` (`game_update` and its phase order), `sw/render.c`, `hw/entity_drawer.sv:280-360`

- [ ] **Step 1: Write the file**

```d2
# Title:       One frame end-to-end
# Section:     06-flow
# Documents:   sw/main.c (process_input, main loop), sw/game.c (game_update), sw/render.c, hw/entity_drawer.sv
# Defends:     "Walk me through what happens during one 16.67 ms frame."
# Detail:      block

...@../common.d2

shape: sequence_diagram

keyboard: { label: "USB keyboard\n(/dev/input/eventN)"; class: external }
main:     { label: "main.c\nwhile (gs.state >= 0)"; class: sw_module }
input_c:  { label: "input.c\ninput_poll() -> INPUT_*"; class: sw_module }
process:  { label: "main.c process_input(&gs)\n(cursor move, place/remove, TAB, ESC)"; class: sw_module }
game_c:   { label: "game.c game_update(&gs)\nsun, spawn, fire, projectiles,\nzombies, collisions, win"; class: sw_module }
render_c: { label: "render.c render_frame(&gs)"; class: sw_module }
driver:   { label: "pvz_driver.ko\nioctl PVZ_WRITE_REG"; class: kernel_module }
regfile:  { label: "pvz_top regfile"; class: register }
drawer:   { label: "entity_drawer + ROMs"; class: hw_module }
pal:      { label: "color_palette"; class: hw_module }
vga:      { label: "VGA pins -> monitor"; class: external }

main -> process: "1. process_input(&gs)"
process -> input_c: "input_poll()"
keyboard -> input_c: "input_event{EV_KEY, code, value}"
input_c -> process: "INPUT_UP/DOWN/.../TAB/SPACE/D/ESC"
main -> game_c: "2. game_update(&gs)"
game_c -> game_c: "update_sun -> update_spawning ->\nupdate_firing -> update_projectiles ->\nupdate_zombies -> check_collisions ->\ncheck_win"
main -> render_c: "3. render_frame(&gs)"
render_c -> driver: "ioctl PVZ_WRITE_REG\n(one per dirty word, up to 51)"
driver -> regfile: "iowrite32(virtbase + word*4)"
main -> main: "4. usleep(FRAME_USEC - elapsed)"

regfile -> drawer: "every clock,\nfor every (px,py)"
drawer -> pal:    "color_out[7:0]"
pal -> vga:       "R,G,B [7:0]"
```

- [ ] **Step 2: Render and verify** — standard cadence.

- [ ] **Step 3: Commit** — message: `docs(diagrams): add one-frame end-to-end flow (4/35)`.

### Task 1.5: REVIEW GATE — render Phase 1 and pause

- [ ] **Step 1: Render every diagram so far**

```bash
make -C doc/diagrams all
```
Expected: clean exit, four `.svg` and four `.png` files added.

- [ ] **Step 2: Stop and ask the user to inspect the four hero diagrams** before continuing.

Show the user the four file paths so they can open the SVGs in a browser or VS Code preview. Do not start Phase 2 until the user confirms.

---

## How to interpret tasks in Phases 2–7

Each task below provides:

- **Path** to create.
- **Read for grounding:** which source files the implementer should open while drafting.
- **Header:** verbatim header comment block to put at the top of the `.d2`.
- **Direction:** the `direction:` line to emit.
- **Structured spec:** the diagram's content as Nodes, Containers, Edges, Style, and Notes. Translate to D2 using the conventions already shown in Phase 1 and the classes from `common.d2`. Every named node and label is concrete — no placeholders.
- **Render and commit** follow the standard cadence (write, `d2 ... .svg && d2 ... .png`, `git add … && git commit -m "docs(diagrams): add <title> (N/36)"`).

---

## Phase 2 — System / context completion (3 tasks)

### Task 2.1: `01-system/02_hw_sw_boundary.d2`

**Read:** `hw/pvz_top.sv:30-45`, `sw/pvz_driver.c`, `sw/main.c`.

**Header:**
```
# Title:       Hardware / software boundary (swim lanes)
# Section:     01-system
# Documents:   sw/* (userspace + kernel) and hw/* (FPGA)
# Defends:     "Which parts run in software, which run in the FPGA, and how do they meet?"
# Detail:      block
```

**Direction:** `direction: right`

**Containers:**
- `userspace` (label "HPS Linux userspace", class `sw_module`, contains nodes `main`, `game`, `render`, `input`)
- `kernel`    (label "HPS Linux kernel",   class `kernel_module`, contains `driver` = "pvz_driver.ko\nmisc /dev/pvz")
- `bridge`    (label "HPS-to-FPGA bridge\n(0xFF20_0000)",  class `bus`)
- `fabric`    (label "FPGA fabric",         class `hw_module`, contains `pvz_top`, `vga_counters`, `bg_grid`, `sprite_roms`, `entity_drawer`, `color_palette`)
- `pins`      (label "Board pins: VGA, USB, UART, SD",   class `external`)

**Nodes:** as listed inside each container, all with appropriate classes (`sw_module` inside userspace; `hw_module` inside fabric; etc.).

**Edges:**
- `userspace.main -> kernel.driver: "ioctl(PVZ_WRITE_REG)"`
- `kernel.driver -> bridge: "iowrite32"`
- `bridge -> fabric.pvz_top: "Avalon-MM write"`
- `fabric.pvz_top -> fabric.entity_drawer: "registers"`
- `fabric.entity_drawer -> fabric.color_palette: "color_out[7:0]"`
- `fabric.color_palette -> pins: "VGA R/G/B"`
- `pins -> userspace.input: "/dev/input/eventN"`  (label "USB keyboard via HPS")

**Style:** containers themed by class.

**Notes:** add one `note` reading "Three-way naming tie: `_hw.tcl` compatible string == DT compatible == driver `of_match_table`. Must agree exactly (`csee4840,pvz_gpu-1.0`)."

### Task 2.2: `01-system/03_soc_block.d2`

**Read:** `doc/manual/DE1-SoC_User_manual.md` for board pinouts; `hw/soc_system_top.sv` for the actual instantiated peripherals.

**Header:**
```
# Title:       Cyclone V SoC block (HPS + FPGA)
# Section:     01-system
# Documents:   hw/soc_system_top.sv, board manual
# Defends:     "What chips are involved and how do the HPS and FPGA share resources?"
# Detail:      block
```

**Direction:** `direction: down`

**Containers / Nodes:**
- `chip` (label "Cyclone V SoC")
  - `hps` (class `sw_module`)
    - `a9` (label "ARM Cortex-A9\n(dual core)")
    - `l2` (label "L2 cache (512 KB)")
    - `sdram_ctrl` (label "SDRAM controller\n(DDR3)")
    - `usb` (label "USB OTG")
    - `uart` (label "UART -> /dev/ttyUSB0")
    - `mmc` (label "SD/MMC")
  - `bridges`
    - `axi`  (label "HPS-to-FPGA AXI bridge\n(unused for pvz_top)", class `bus`)
    - `lw`   (label "Lightweight HPS-to-FPGA bridge\nbase 0xFF20_0000",  class `bus`)
    - `f2h`  (label "FPGA-to-HPS bridge\n(unused)", class `bus`)
  - `fpga` (class `hw_module`)
    - `pvz_top` (class `hw_module`)
- `offchip`
  - `ddr3` (label "DDR3 SDRAM\n(1 GB)", class `external`)
  - `vga_dac` (label "VGA DAC + connector", class `external`)
  - `usb_keyboard` (class `external`)

**Edges:**
- `hps.sdram_ctrl -> offchip.ddr3`
- `hps.usb -> offchip.usb_keyboard`
- `hps -> bridges.lw` (label "control")
- `bridges.lw -> fpga.pvz_top`
- `fpga.pvz_top -> offchip.vga_dac` (label "VGA R/G/B + sync")

### Task 2.3: `01-system/04_memory_map.d2`

**Read:** `hw/pvz_top_hw.tcl` (address span), `sw/pvz.h` (register indices).

**Header:**
```
# Title:       HPS memory map and pvz_top window
# Section:     01-system
# Documents:   hw/pvz_top_hw.tcl, sw/pvz.h, sw/pvz_driver.c
# Defends:     "Where does pvz_top live in memory and how does the kernel reach it?"
# Detail:      block
```

**Direction:** `direction: down`

**Containers / Nodes** (table-style, class `register` per row):
- `ddr3_block` "DDR3 SDRAM\n0x0000_0000 - 0x3FFF_FFFF (1 GB)"
- `gap1` "...periph reserved..."
- `lw_bridge_block` "Lightweight HPS-to-FPGA bridge\n0xFF20_0000 - 0xFF3F_FFFF"
- `pvz_window` (nested inside `lw_bridge_block`, label "pvz_top peripheral window\n(qsys-assigned base)")
- Inside `pvz_window` show a small grid of 7 register groups (`PLANTS`, `SUNFLOWERS`, `ZOMBIE[0..7]`, `PEA[0..7]`, `CURSOR`, `SUN`, `SELECTED`) with their word indices and byte offsets.

**Notes:**
- "Driver `pvz_driver.c` calls `of_iomap()` to get the kernel virtual base for the peripheral window; per-write byte offset is `word_index * 4`."

---

## Phase 3 — Hardware tier completion (8 tasks)

### Task 3.1: `02-hw/01_fpga_top.d2`

**Read:** `hw/soc_system_top.sv`, `hw/soc_system.qsys` (top-level).

**Header:**
```
# Title:       FPGA top-level containment
# Section:     02-hw
# Documents:   hw/soc_system_top.sv, hw/soc_system.qsys
# Defends:     "Where does pvz_top sit inside the synthesised design?"
# Detail:      block
```

**Direction:** `direction: down`

**Containers:**
- `soc_system_top` (class `hw_module`)
  - `clocks` (label "PLL: 50 MHz -> clk")
  - `hps_io` (label "HPS pin assignments\n(DDR3, USB, UART, SD)", class `external`)
  - `soc_system` (class `hw_module`, qsys-generated)
    - `pvz_top` (class `hw_module`)
    - `bridge_glue` (label "HPS-to-FPGA lightweight slave master", class `bus`)
  - `vga_pins` (class `external`)
- `board_pins` (outside, listing the 50 MHz crystal, VGA connector, etc.)

**Edges:**
- `clocks -> soc_system: clk`
- `soc_system.bridge_glue -> soc_system.pvz_top: avalon`
- `soc_system.pvz_top -> vga_pins`
- `hps_io -> soc_system: HPS interface bundle`

### Task 3.2: `02-hw/03_vga_counters.d2`

**Read:** `hw/vga_counters.sv` (entire file).

**Header:**
```
# Title:       vga_counters timing generator
# Section:     02-hw
# Documents:   hw/vga_counters.sv
# Defends:     "How is hcount/vcount generated and what defines the 640x480@60 timing?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes:** `clk50_in` (class `signal`, label "clk50 (50 MHz)"), `pix_clk` (label "pixel clock 25 MHz\n= hcount[0]", class `signal`), `hcount` (label "hcount[10:0]\n0..1599\nincr every clk50", class `register`), `vcount` (label "vcount[9:0]\n0..524\nincr at hcount==1599", class `register`), `hsync_gen` (label "HSYNC\nactive low during back porch", class `hw_module`), `vsync_gen` (label "VSYNC\nactive low during V sync", class `hw_module`), `blank_gen` (label "VGA_BLANK_n\nlow outside 640x480 active", class `hw_module`).

**Edges:** `clk50_in -> hcount`, `hcount -> vcount`, `hcount -> pix_clk`, `hcount -> hsync_gen`, `vcount -> vsync_gen`, `hcount -> blank_gen`, `vcount -> blank_gen`, plus `hcount -> px (px = hcount[10:1])` and `vcount -> py (py = vcount)`.

**Notes:** "Timing constants come from VESA 640x480@60: H total 800, V total 525, 25.175 MHz pixel clock (here 25 MHz, close enough for monitors)."

### Task 3.3: `02-hw/04_bg_grid.d2`

**Read:** `hw/bg_grid.sv` (entire file). Constants: `GRID_X=64, GRID_Y=112, GRID_W=512, GRID_H=256`. Palette indices: `COL_DARK_GREEN=1, COL_LIGHT_GREEN=2, COL_BLUE=13`. The parity check uses `gx[6] ^ gy[6]`, not `(cell_col+cell_row)&1`.

**Header:**
```
# Title:       bg_grid (lawn checker background)
# Section:     02-hw
# Documents:   hw/bg_grid.sv
# Defends:     "How is the background drawn? What are GRID_X, GRID_Y, CELL?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes:**
- `inputs` (class `signal`, label "px[9:0]\npy[9:0]")
- `in_grid_check` (class `hw_module`, label "in_grid =\npx in [64..575] AND\npy in [112..367]")
- `offset` (class `hw_module`, label "gx = px - 64\ngy = py - 112")
- `parity` (class `hw_module`, label "light_cell = gx[6] ^ gy[6]\n(checker: alternates every 64 px)")
- `out_color` (class `signal`, label "color_out[7:0]\n  in_grid && light_cell  -> COL_LIGHT_GREEN (2)\n  in_grid && !light_cell -> COL_DARK_GREEN  (1)\n  !in_grid               -> COL_BLUE        (13)")

**Edges:** flow left to right; tie `inputs` to `in_grid_check` and `offset`; `offset -> parity`; `in_grid_check + parity -> out_color`.

**Notes:** "Constants must match `entity_drawer.sv`: GRID_X=64, GRID_Y=112, CELL=64, GRID_COLS=8, GRID_ROWS=4. Outside the lawn the background is sky BLUE (palette idx 13), NOT black."

### Task 3.4: `02-hw/05_sprite_rom.d2`

**Read:** `hw/sprite_rom.sv`, `hw/peashooter_idx.mem` (look at format, do not transcribe), `hw/pvz_top.sv:177-199`.

**Header:**
```
# Title:       sprite_rom and *_idx.mem layout
# Section:     02-hw
# Documents:   hw/sprite_rom.sv, hw/{peashooter,sunflower,zombie}_idx.mem
# Defends:     "How do sprite ROMs work and how are they addressed?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes:**
- `mem_files` (class `register`, label "peashooter_idx.mem\nsunflower_idx.mem\nzombie_idx.mem\n4096 bytes each\n($readmemh hex, palette index per pixel)")
- `rom_inst_plant`, `rom_inst_sun`, `rom_inst_zombie` (each class `hw_module`, label "sprite_rom\nclk + addr[11:0] -> pixel[7:0]\n1-cycle read latency")
- `addrs` (class `signal`, label "plant_rd_addr[11:0] = {in_cell_y[5:0], in_cell_x[5:0]}\n(plant + sunflower share this)\nzombie_rd_addr[11:0] = {zombie_in_y[5:0], zombie_in_x[5:0]}")
- `outputs` (class `signal`, label "plant_pixel, sunflower_pixel, zombie_pixel")

**Edges:** `mem_files -> rom_inst_*`, `addrs -> rom_inst_*`, `rom_inst_* -> outputs`.

**Notes:** "COL_TRANSPARENT = 0xFF means 'this sprite pixel is transparent at this cell location'."

### Task 3.5: `02-hw/06_entity_drawer_pipeline.d2`

**Read:** `hw/entity_drawer.sv:1-26` (layering comment, names the 7 logical layers), `:140-280` (stage 1 hit detection), `:282-325` (stage-1 hit signals registered via `always_ff`), `:327-358` (stage-2 final mux is `always_comb`, NOT registered).

**Header:**
```
# Title:       entity_drawer pipeline (7 logical layers, 2 stages)
# Section:     02-hw
# Documents:   hw/entity_drawer.sv
# Defends:     "How does the entity_drawer combine layers? Where is the 1-cycle latency?"
# Detail:      block
```

**Direction:** `direction: right`

**Containers:**
- `stage1` (label "Stage 1 — combinational hit detection (cycle N)", class `layer`)
  - `bg_hit` (class `hw_module`, label "bg lookup\n(bg_grid -> bg_color)")
  - `plant_hit` (label "plant_here = in_grid && plant_present[plant_idx]\nplant_rd_addr issued (shared with sunflower ROM)")
  - `sun_hit` (label "sunflower_here = in_grid && sunflower_present[plant_idx]")
  - `pea_hit` (label "pea bbox check\n(loop over 8 pea slots)")
  - `zombie_hit` (label "zombie bbox priority encoder\nzombie_rd_addr issued")
  - `cursor_hit` (label "cursor border check\n4 px border, CURSOR_BORDER")
  - `sun_hud_hit` (label "sun HUD blocks (up to 10)\nblock i lit when sun >= (i+1)*50")
  - `selector_hit` (label "selector fill + border\n(2 boxes, SEL_SZ=48)")
- `pipeline_regs` (label "Stage boundary flip-flops (always_ff @posedge clk)\n12 fields: bg_color_d[7:0], plant_here_d,\nsunflower_here_d, zombie_hit_d, pea_hit_d,\ncursor_hit_d, sun_hit_d,\nsel0_hit_d, sel0_border_d,\nsel1_hit_d, sel1_border_d,\nselected_plant_d[1:0]", class `register`)
- `stage2` (label "Stage 2 — final mux (cycle N+1, always_comb, NOT registered)\nLayer order in the mux (later overrides earlier):\nbg -> plant -> sunflower -> pea -> zombie ->\ncursor -> sun_hud -> sel0_fill -> sel1_fill ->\nsel0_border (if selected==0) -> sel1_border (if selected==1)", class `layer`)
  - `out` (class `signal`, label "color_out[7:0]\n(combinational mux output -> color_palette)")

**Edges:** every Stage 1 hit signal -> matching `pipeline_regs` flop -> Stage 2 mux; ROM pixels (`plant_rd_pixel`, `sunflower_rd_pixel`, `zombie_rd_pixel`) feed Stage 2 directly in cycle N+1 because their addresses were issued in cycle N. Stage 2 mux -> `out`.

**Notes:**
- "The mux itself is `always_comb` (entity_drawer.sv:330). The 1-cycle latency between `px,py` and `color_out` comes from registering the stage-1 hits AND from the sprite ROM's 1-cycle read latency — those two delays align so the mux can use both on cycle N+1."
- "Plant and sunflower share `plant_rd_addr` because they share the cell coordinates; the two ROMs read in parallel."

### Task 3.6: `02-hw/07_entity_drawer_signals.d2` ⚙ (signal-level hot spot)

**Read:** `hw/entity_drawer.sv` (full file). Especially: lines 33-79 ports (bit-widths), 122-194 unpack + zombie hit priority encoder, 282-325 pipeline registers (`always_ff`), 327-358 final mux (`always_comb`, combinational — do NOT claim the output is registered).

**Header:**
```
# Title:       entity_drawer signal-level datapath (hot spot)
# Section:     02-hw
# Documents:   hw/entity_drawer.sv (all)
# Defends:     "What is the bit-width of every signal entering / leaving entity_drawer?"
# Detail:      signal
```

**Direction:** `direction: right`

**Containers/Nodes** (class `signal` unless noted):
- `inputs`
  - `px[9:0]`, `py[9:0]` (from `vga_counters` via `pvz_top`)
  - `bg_color[7:0]` (from `bg_grid`)
  - `plant_present[31:0]`, `sunflower_present[31:0]` (from regfile word 0/1)
  - `selected_plant[1:0]` (regfile word 50)
  - `zombie_alive[7:0]`, `zombie_x_packed[79:0]`, `zombie_row_packed[15:0]` (regfile words 32..39, packed in pvz_top)
  - `pea_alive[7:0]`, `pea_x_packed[79:0]`, `pea_row_packed[15:0]` (regfile words 40..47, packed)
  - `cursor_visible[1]`, `cursor_col[2:0]`, `cursor_row[1:0]` (regfile word 48)
  - `sun_value[13:0]` (regfile word 49)
- `unpack` (class `hw_module`, label "generate-loop unpack\nzombie_x[i] = zombie_x_packed[i*10 +: 10]\nzombie_row[i] = zombie_row_packed[i*2 +: 2]\nsame for pea")
- `stage1_addr` (class `hw_module`, label "in_grid, gx[9:0], gy[9:0]\ncell_col[2:0]=gx[8:6], cell_row[1:0]=gy[7:6]\nin_cell_x[5:0]=gx[5:0], in_cell_y[5:0]=gy[5:0]\nplant_idx[4:0]={cell_row, cell_col}")
- `rom_io`
  - `plant_rd_addr[11:0]` (output to plant + sunflower ROMs)
  - `plant_rd_pixel[7:0]`, `sunflower_rd_pixel[7:0]` (input)
  - `zombie_rd_addr[11:0]` (output)
  - `zombie_rd_pixel[7:0]` (input)
- `pipeline_regs` (class `register`, label "12 flip-flops, all clk/reset:\nbg_color_d[7:0], plant_here_d,\nsunflower_here_d, zombie_hit_d,\npea_hit_d, cursor_hit_d, sun_hit_d,\nsel0_hit_d, sel0_border_d,\nsel1_hit_d, sel1_border_d,\nselected_plant_d[1:0]")
- `final_mux` (class `hw_module`, label "Layered mux (always_comb, combinational)\ncolor_out[7:0]\nbg_color_d -> plant -> sunflower ->\npea -> zombie -> cursor -> sun_hud ->\nsel0_fill -> sel1_fill ->\nsel0_border (if selected==0) ->\nsel1_border (if selected==1)")
- `color_out_node` (class `signal`, label "color_out[7:0] -> color_palette\n(combinational, NOT a separate flip-flop)")

**Edges:** match the source-of-truth: packed buses -> `unpack`; `unpack` + coordinates -> `stage1_addr`; `stage1_addr` -> `plant_rd_addr`, `zombie_rd_addr`; ROM pixel inputs -> `final_mux`; all stage-1 hit signals -> `pipeline_regs`; `pipeline_regs` + current-cycle ROM pixels -> `final_mux` -> `color_out_node`.

**Notes:**
- "Color indices used in the mux: GREEN=7, BRIGHT_GREEN=9, YELLOW=4, ORANGE=12, TRANSPARENT=0xFF (must match color_palette.sv)."
- "Only the stage-1 hit signals are registered (in `pipeline_regs`). The final mux at `entity_drawer.sv:330` is `always_comb` — `color_out` is combinational, NOT a separate flip-flop."

### Task 3.7: `02-hw/08_color_palette.d2`

**Read:** `hw/color_palette.sv` (entire file).

**Header:**
```
# Title:       color_palette LUT
# Section:     02-hw
# Documents:   hw/color_palette.sv
# Defends:     "How does an 8-bit index become 24-bit RGB?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes:**
- `idx_in` (class `signal`, label "index[7:0]")
- `lut` (class `hw_module`, label "256-entry case statement\n8-bit index -> {R[7:0], G[7:0], B[7:0]}\nKey entries:\n 0=black, 4=yellow, 7=green,\n 9=bright_green, 12=orange,\n 0xFF=transparent (drawer skips)")
- `out_r`, `out_g`, `out_b` (class `signal`, each `[7:0]`)

**Edges:** `idx_in -> lut`, `lut -> {out_r, out_g, out_b}`.

**Notes:** "The drawer never emits 0xFF (it short-circuits earlier), so palette[0xFF] only matters for the testbench."

### Task 3.8: `02-hw/09_pvz_top_regfile_signals.d2` ⚙ (signal-level hot spot)

**Read:** `hw/pvz_top.sv:30-148` (ports and Avalon write decode).

**Header:**
```
# Title:       pvz_top regfile decoder (signal-level hot spot)
# Section:     02-hw
# Documents:   hw/pvz_top.sv (Avalon-MM decoder)
# Defends:     "How does a 32-bit write to Avalon address N land in the right field?"
# Detail:      signal
```

**Direction:** `direction: right`

**Nodes:**
- `avalon_in` (class `bus`, label "address[5:0]\nwritedata[31:0]\nwrite\nchipselect")
- `decoder` (class `hw_module`, label "if (chipselect && write):\n  addr==0  -> plant_present <= writedata\n  addr==1  -> sunflower_present <= writedata\n  addr<40  -> zombie[addr[2:0]].{alive,x,row}\n              <= {writedata[31], [9:0], [11:10]}\n              ALIAS: addresses 2..31 also fall into this branch,\n              writing zombie slot (addr[2:0]). SW reserves 2..31\n              and must never write them.\n  addr<48  -> pea[addr[2:0]].{alive,x,row}\n              <= same\n  addr==48 -> cursor_visible, cursor_col[4:2], cursor_row[1:0]\n  addr==49 -> sun_value[13:0]\n  addr==50 -> selected_plant[1:0]")
- `regs` (class `register`, container with each field as a sub-node, all flip-flops:
  - `plant_present[31:0]`
  - `sunflower_present[31:0]`
  - `zombie_alive[7:0]`, `zombie_x[0..7][9:0]`, `zombie_row[0..7][1:0]`
  - `pea_alive[7:0]`, `pea_x[0..7][9:0]`, `pea_row[0..7][1:0]`
  - `cursor_visible`, `cursor_col[2:0]`, `cursor_row[1:0]`
  - `sun_value[13:0]`
  - `selected_plant[1:0]`)
- `packer` (class `hw_module`, label "generate-loop pack\nzombie_x_packed[i*10+:10] = zombie_x[i]\n(same for row, pea_x, pea_row)")
- `out_to_drawer` (class `bus`, label "to entity_drawer\nplant_present, sunflower_present,\nselected_plant, zombie_alive,\nzombie_x_packed[79:0],\nzombie_row_packed[15:0],\npea_alive, pea_x_packed[79:0],\npea_row_packed[15:0],\ncursor_*, sun_value")

**Edges:** `avalon_in -> decoder -> regs -> packer -> out_to_drawer`.

**Notes:**
- "Reset clears every register to 0."
- "No vsync latching: each write takes effect on the next clk edge. A write that races a scanline produces one frame of tearing — acceptable at 60 Hz."
- "Avalon `addressUnits = WORDS` in `pvz_top_hw.tcl`, so the 6-bit `address` is a word index. The driver does `iowrite32(base + word*4)` because the CPU side is byte-addressed."
- "Decoder uses `else if (address < 6'd40)` (pvz_top.sv:122), so words 2..31 alias into the zombie slot at `zombie[address[2:0]]`. SW must never write addresses 2..31. Documented in `doc/guide/06-register-map.md:143-160`."

---

## Phase 4 — Software tier (7 tasks)

### Task 4.1: `03-sw/01_process_architecture.d2`

**Read:** `sw/main.c` (whole file, especially the loop at 110-150), `sw/game.h`, `sw/render.h`, `sw/input.h`.

**Header:**
```
# Title:       Userspace process architecture
# Section:     03-sw
# Documents:   sw/main.c, sw/{game,render,input}.{c,h}, sw/pvz_driver.c
# Defends:     "What's in the userspace binary, what's in the kernel, and how do they connect?"
# Detail:      block
```

**Direction:** `direction: down`

**Containers / Nodes:**
- `pvz_binary` (class `sw_module`, label "./pvz binary (one process, one thread)")
  - `main`   ("main.c — 60 Hz loop:\nprocess_input(&gs) ->\ngame_update(&gs) ->\nrender_frame(&gs) ->\nusleep(FRAME_USEC - elapsed)")
  - `game`   ("game.c / game.h — state + rules")
  - `render` ("render.c — game state -> ioctl writes")
  - `input`  ("input.c — evdev decode (input_poll())")
- `kernel` (class `kernel_module`, label "Linux kernel")
  - `pvz_drv` ("pvz_driver.ko — misc dev /dev/pvz")
  - `evdev`   ("evdev — /dev/input/eventN")
- `device_files` (class `external`, label "/dev/pvz, /dev/input/eventN")

**Edges:** main calls into game/render/input; render -> /dev/pvz -> pvz_drv -> Avalon writes; input -> /dev/input/eventN -> evdev -> USB stack.

### Task 4.2: `03-sw/02_kernel_driver.d2`

**Read:** `sw/pvz_driver.c` (full file — pay attention to `pvz_init` at line 127 calling `platform_driver_probe` NOT `platform_driver_register`; and `pvz_probe` at line 61 calling `misc_register` BEFORE `of_address_to_resource`/`request_mem_region`/`of_iomap`), `sw/pvz.h`.

**Header:**
```
# Title:       pvz_driver internals (init, probe, ioctl, cleanup)
# Section:     03-sw
# Documents:   sw/pvz_driver.c, sw/pvz_driver.h, sw/pvz.h
# Defends:     "Walk us through the kernel module."
# Detail:      block
```

**Direction:** `direction: down`

**Containers:**
- `init` (label "Module load — pvz_init() at sw/pvz_driver.c:127")
  - Nodes (in order; note `pvz_init` calls `platform_driver_probe`, the legacy entry that performs registration AND immediate probing if a device exists):
    - `pvz_init` ("pvz_init() — module_init hook")
    - `pdrv_probe` ("platform_driver_probe(&pvz_driver, pvz_probe)\n(registers driver AND calls pvz_probe if the DT node already matches)")
- `probe` (label "pvz_probe() at sw/pvz_driver.c:61")
  - Nodes in source order:
    - `miscreg` ("misc_register(&pvz_misc_device)\n-> /dev/pvz appears now (BEFORE iomap)")
    - `of_addr` ("of_address_to_resource(of_node, 0, &dev.res)\n-> physical addr range")
    - `req_mem` ("request_mem_region(start, size, DRIVER_NAME)\n-> claim the I/O window")
    - `of_iomap` ("of_iomap(of_node, 0)\n-> dev.virtbase (kernel virtual base)")
    - `pr_info` ("pr_info(DRIVER_NAME ': initialized at 0x%08lx', dev.res.start)")
  - Note: failure at any of `of_addr`/`req_mem`/`of_iomap` jumps to `out_deregister` (or `out_release_mem_region`), which undoes the partial init in reverse.
- `runtime` (label "ioctl write path — pvz_ioctl() at sw/pvz_driver.c:30")
  - `open` ("open(/dev/pvz)")
  - `pvz_ioctl` ("pvz_ioctl(cmd, arg)")
  - `copy` ("copy_from_user(&w, (pvz_write_arg_t*)arg, sizeof(w))\n-> -EACCES on fault")
  - `bounds_check` ("if (w.word_index >= PVZ_NUM_REGS) return -EINVAL")
  - `iowrite32_call` ("iowrite32(w.value, dev.virtbase + w.word_index*4)")
- `exit` (label "Module unload — pvz_remove() at sw/pvz_driver.c:101")
  - `pvz_remove` ("iounmap(dev.virtbase) ->\nrelease_mem_region ->\nmisc_deregister(&pvz_misc_device)")
  - `pvz_exit` ("pvz_exit() -> platform_driver_unregister(&pvz_driver)")

**Edges:** init flow -> probe sequence (vertical chain in source order); runtime flow vertically; arrow from `open` to `pvz_ioctl` labelled "ioctl(PVZ_WRITE_REG)".

**Notes:**
- "Compatible string must match across `_hw.tcl`, generated DT, and `of_match_table` — `csee4840,pvz_gpu-1.0`."
- "`pvz_init` uses `platform_driver_probe`, which is the legacy API and requires the DT node to be present at registration time. If the device is added later (hotplug), it won't be probed — fine here because the DT entry is fixed at boot."
- "`misc_register` is called BEFORE the I/O window is mapped (pvz_driver.c:65, then iomap at :83). Userspace can theoretically `open(/dev/pvz)` between those steps; ioctls would still work because the bounds check happens before the `iowrite32` reaches the uninitialised pointer — but in practice `pvz_init` blocks until probe returns, so this race is academic."

### Task 4.3: `03-sw/03_game_state_er.d2`

**Read:** `sw/game.h` (struct definitions).

**Header:**
```
# Title:       game_state_t data shape
# Section:     03-sw
# Documents:   sw/game.h
# Defends:     "What does the game state struct look like?"
# Detail:      block
```

**Direction:** `direction: right`

**Containers / Nodes** (use `shape: sql_table` if useful, else class `register`):
- `game_state_t` table-style
  - `grid[GRID_ROWS][GRID_COLS]: plant_t`
  - `zombies[MAX_ZOMBIES]: zombie_t`
  - `projectiles[MAX_PROJECTILES]: projectile_t`
  - `cursor_row, cursor_col, cursor_visible`
  - `sun (int)`
  - `frame_count (int)`
  - `state (STATE_PLAYING/WIN/LOSE)`
  - `selected_plant (PLANT_PEASHOOTER/SUNFLOWER)`
  - `zombies_spawned`
- `plant_t` (`type`, `fire_cooldown`, `hp`)
- `zombie_t` (`active`, `row`, `x_pixel`, `hp`, `move_counter`, `eating`, `eat_timer`)
- `projectile_t` (`active`, `row`, `x_pixel`)

**Edges:** arrows from outer struct fields to inner struct definitions.

**Notes:** "Constants from game.h: GRID_ROWS=4, GRID_COLS=8, MAX_ZOMBIES=8, MAX_PROJECTILES=16."

### Task 4.4: `03-sw/04_game_loop_fsm.d2`

**Read:** `sw/game.c` (look for `g->state =` writes), `sw/game.h` (STATE_*).

**Header:**
```
# Title:       Game state FSM
# Section:     03-sw
# Documents:   sw/game.c, sw/game.h
# Defends:     "When does the game declare win or lose?"
# Detail:      block
```

**Direction:** `direction: right`

**States (class `fsm_state`):** `INIT`, `PLAYING`, `WIN`, `LOSE`.

**Transitions:**
- `INIT -> PLAYING`: "game_init(): sun=100, grid empty, cursor at (0,0)"
- `PLAYING -> LOSE`: "any zombie's x_pixel <= GAME_AREA_X (=64); set inside update_zombies at game.c:142-144"
- `PLAYING -> WIN`: "all TOTAL_ZOMBIES (=5) spawned and no zombies active; set in check_win at game.c:280-285"
- `WIN -> exit` and `LOSE -> exit` (terminal states; main loop in sw/main.c:133-144 renders one final frame, sleep 5s, then break)

### Task 4.5: `03-sw/05_game_update_flow.d2`

**Read:** `sw/game.c:288-302` (`game_update` body — this is the canonical phase order), and the bodies of `update_sun`, `update_spawning`, `update_firing`, `update_projectiles`, `update_zombies`, `check_collisions`, `check_win` defined earlier in the same file. `sw/game.h` for constants.

**Header:**
```
# Title:       game_update() per-frame phase order
# Section:     03-sw
# Documents:   sw/game.c (game_update at line 288; input handled separately in sw/main.c::process_input)
# Defends:     "What is the exact order of phases each frame, and where does input fit?"
# Detail:      block
```

**Direction:** `direction: down`

**Important:** input handling is NOT inside `game_update`. The main loop calls `process_input(&gs)` first (sw/main.c:114), then `game_update(&gs)` (sw/main.c:119), then `render_frame(&gs)` (sw/main.c:122). This diagram covers the `game_update` phases only.

**Nodes (sequential, class `sw_module`; match `game.c:295-301` exactly):**
1. `early_return` "if (gs->state != STATE_PLAYING) return;"
2. `frame_inc` "gs->frame_count++"
3. `update_sun` "every SUN_INTERVAL (480) frames: gs->sun += SUN_INCREMENT (25)"
4. `update_spawning` "spawn next zombie if frame_count is in window; uses srand-seeded RNG"
5. `update_firing` "for each peashooter in the grid: if cooldown==0 AND any zombie in row to the right -> spawn a pea, reset cooldown to PLANT_FIRE_COOLDOWN (120)"
6. `update_projectiles` "for each active pea: x_pixel += PEA_SPEED (2); deactivate at right edge"
7. `update_zombies` "for each active zombie: if eating -> tick eat timer, on bite damage plant; else -> move 1 px every ZOMBIE_SPEED_FRAMES (3)"
8. `check_collisions` "for each active pea vs each active zombie in same row: bbox overlap -> zombie.hp -= PEA_DAMAGE (1); pea cleared; on zombie.hp==0 zombies_killed++"
9. `check_win` "if any active zombie -> still playing; else if zombies_spawned == TOTAL_ZOMBIES (5) -> STATE_WIN"

**Edges:** straight vertical chain; `early_return` short-circuits the rest.

**Notes:** "Lose condition is set inside `update_zombies`, NOT in `check_win` — see `sw/game.c:142-145`: when a zombie's `x_pixel <= GAME_AREA_X` (=64, the lawn left edge), it sets `gs->state = STATE_LOSE` and returns. `check_win` only handles the WIN case."

### Task 4.6: `03-sw/06_input_pipeline.d2`

**Read:** `sw/input.c` (full file), `sw/input.h`.

**Header:**
```
# Title:       Input pipeline (keyboard / gamepad -> action)
# Section:     03-sw
# Documents:   sw/input.c, sw/input.h
# Defends:     "How does a key press get into the game?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes:**
- `usb_keyboard` (class `external`, "USB keyboard / Xbox 360 pad")
- `kernel_evdev` (class `kernel_module`, "/dev/input/eventN")
- `open_call` (class `sw_module`, "input_init(): open(path, O_RDONLY|O_NONBLOCK)")
- `read_loop` (class `sw_module`, "input_poll(): read(fd, &ev, sizeof ev)\nrepeat until EAGAIN")
- `decode` (class `sw_module`, "if ev.type==EV_KEY && ev.value!=0: map ev.code -> action_t")
- `actions` (class `signal`, "INPUT_NONE=0, INPUT_UP=1, INPUT_DOWN=2,\nINPUT_LEFT=3, INPUT_RIGHT=4,\nINPUT_SPACE=5, INPUT_D=6, INPUT_ESC=7,\nINPUT_TAB=8  (from sw/input.h)")
- `main_loop` (class `sw_module`, "main.c::process_input(&gs)\nwhile ((key = input_poll()) != INPUT_NONE) ... ")

**Edges:** `usb_keyboard -> kernel_evdev -> open_call -> read_loop -> decode -> actions -> main_loop`.

**Notes:** "Recent commit `fc14c06` added Xbox 360 gamepad button mapping in the same `input_poll`. Verify the action names against sw/input.h — they are the canonical source."

### Task 4.7: `03-sw/07_render_mapping.d2`

**Read:** `sw/render.c` (full file), `sw/pvz.h` (PVZ_REG_*, pvz_pack_entity, pvz_pack_cursor).

**Header:**
```
# Title:       render_frame() field -> register mapping
# Section:     03-sw
# Documents:   sw/render.c, sw/pvz.h
# Defends:     "Show me exactly which game field becomes which register write."
# Detail:      block
```

**Direction:** `direction: right`

**Two columns (containers):**
- `game_state` (left, class `sw_module`) sub-nodes: `grid[r][c].type==PEASHOOTER`, `grid[r][c].type==SUNFLOWER`, `zombies[i]`, `projectiles[i]`, `cursor_*`, `sun`, `selected_plant`
- `regs` (right, class `register`) sub-nodes mirroring all 51 register words.

**Edges:**
- `grid... PEASHOOTER -> regs.PLANTS` (with bit index = row*8+col)
- `grid... SUNFLOWER  -> regs.SUNFLOWER`
- `zombies[i] -> regs.ZOMBIE[i]` (via pvz_pack_entity)
- `projectiles[i] -> regs.PEA[i]`
- `cursor_* -> regs.CURSOR` (via pvz_pack_cursor)
- `sun -> regs.SUN` (low 14 bits)
- `selected_plant -> regs.SELECTED`

**Notes:** "render_frame() walks the state in this order, issuing PVZ_WRITE_REG ioctls one per word. With no commit handshake, hardware sees writes immediately."

---

## Phase 5 — HW/SW interface completion (3 tasks)

### Task 5.1: `04-interface/01_avalon_bus.d2`

**Read:** `hw/pvz_top_hw.tcl`, `sw/pvz_driver.c`, `doc/reference-design/lab3/description/description.md` (Avalon address-units gotcha).

**Header:**
```
# Title:       Avalon-MM bus topology
# Section:     04-interface
# Documents:   hw/pvz_top_hw.tcl, hw/pvz_top.sv (ports), sw/pvz_driver.c
# Defends:     "How does a CPU store reach pvz_top?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes:**
- `cpu` (class `sw_module`, "ARM Cortex-A9 (HPS)")
- `axi` (class `bus`, "Cortex AXI interconnect")
- `lw_bridge` (class `bus`, "HPS-to-FPGA lightweight bridge\n0xFF20_0000")
- `qsys_fabric` (class `bus`, "qsys-generated Avalon interconnect")
- `pvz_slave` (class `hw_module`, "pvz_top Avalon slave\naddress[5:0] (WORDS), writedata[31:0], write, chipselect")

**Edges:** straight chain `cpu -> axi -> lw_bridge -> qsys_fabric -> pvz_slave`.

**Notes:**
- "Avalon `addressUnits = WORDS` in `pvz_top_hw.tcl`. The CPU still issues byte addresses; qsys narrows them: word_index = byte_offset >> 2."
- "Driver always uses `iowrite32(base + word*4, value)` — byte offset, NOT word offset."

### Task 5.2: `04-interface/02_ioctl_path.d2`

**Read:** `sw/pvz_driver.c` (full file), `sw/render.c` (`write_reg` helper), `hw/pvz_top.sv:94-148`.

**Header:**
```
# Title:       ioctl write path (one register, end-to-end)
# Section:     04-interface
# Documents:   sw/render.c (write_reg), sw/pvz_driver.c (pvz_ioctl), hw/pvz_top.sv (decoder)
# Defends:     "Trace one ioctl all the way to the flip-flop."
# Detail:      block
```

**Direction:** `direction: right` (use `shape: sequence_diagram`).

**Participants:**
- `render_c` (class `sw_module`, "render.c write_reg(w, v)")
- `libc`    (class `sw_module`, "libc ioctl(fd, PVZ_WRITE_REG, &arg)")
- `vfs`     (class `kernel_module`, "VFS / chrdev dispatch")
- `pvz_ioctl` (class `kernel_module`, "pvz_ioctl()")
- `copy`    (class `kernel_module`, "copy_from_user(&arg, ...)")
- `io_w`    (class `kernel_module`, "iowrite32(arg.value, base + arg.word_index*4)")
- `avalon`  (class `bus`, "Avalon write transaction")
- `regfile` (class `register`, "pvz_top regfile flip-flop")

**Messages (in order):**
1. `render_c -> libc`: "write_reg(word, value)"
2. `libc -> vfs`: "ioctl(fd, _IOW('p',1,...), &arg)"
3. `vfs -> pvz_ioctl`: "dispatch"
4. `pvz_ioctl -> copy`: "copy_from_user"
5. `pvz_ioctl -> pvz_ioctl`: "bounds check word_index < 51"
6. `pvz_ioctl -> io_w`: ""
7. `io_w -> avalon`: "address = word, writedata = value"
8. `avalon -> regfile`: "next clk edge: field updated"

### Task 5.3: `04-interface/04_device_tree_binding.d2`

**Read:** `hw/pvz_top_hw.tcl`, `sw/pvz_driver.c` (look for `of_match_table`), `doc/reference-design/lab3/description/description.md`.

**Header:**
```
# Title:       Device tree binding (three-way compatible-string tie)
# Section:     04-interface
# Documents:   hw/pvz_top_hw.tcl, sw/pvz_driver.c
# Defends:     "Why does the driver bind, and what breaks if a string is off by one?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes (sequential):**
- `hw_tcl` (class `hw_module`, "pvz_top_hw.tcl\nset_module_assignment\nembeddedsw.dts.compatible = csee4840,pvz_gpu-1.0")
- `qsys` (class `hw_module`, "qsys-generate -> soc_system.sopcinfo")
- `sopc2dts` (class `hw_module`, "sopc2dts -> soc_system.dts (text)")
- `dtc` (class `hw_module`, "dtc -I dts -O dtb -> soc_system.dtb")
- `boot` (class `external`, "U-Boot loads dtb\nkernel walks DT")
- `of_match` (class `kernel_module`, "pvz_driver.c\nof_device_id { .compatible = csee4840,pvz_gpu-1.0 }")
- `probe` (class `kernel_module`, "match found -> pvz_probe(pdev)")

**Edges:** straight chain.

**Notes:** "If any of the three strings disagree (`_hw.tcl`, generated DT, `of_match_table`) the driver silently fails to bind. Verify with `ls /proc/device-tree/sopc@0/` and `dmesg | tail`."

---

## Phase 6 — Cross-cutting (5 tasks)

*Note: an earlier draft included a `06_test_architecture.d2` diagram (ModelSim TBs, on-board test programs, host `test_game`). None of those exist on this branch — `hw/tb/`, `sw/test/`, `.github/`, and `deploy.sh` are all absent (`doc/guide/README.md:61` explicitly says so). Diagram dropped.*

### Task 6.1: `05-cross-cutting/01_frame_timing.d2`

**Read:** `sw/main.c` (FRAME_USEC), `sw/render.c`.

**Header:**
```
# Title:       60 Hz frame timing budget
# Section:     05-cross-cutting
# Documents:   sw/main.c (FRAME_USEC = 16667), sw/render.c, hw/pvz_top.sv
# Defends:     "How is the 16.67 ms frame spent and when can tearing occur?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes (linear, class `sw_module` except the last):**
- `t0` "process_input(&gs) (~us, non-blocking input_poll loop)"
- `t1` "game_update(&gs) (~us, all in C)"
- `t2` "render_frame(&gs): up to 51 ioctl writes (~us)"
- `t3` "usleep(FRAME_USEC - elapsed)  // FRAME_USEC = 16667"
- `vga_tick` (class `hw_module`) "VGA scan: 525 lines × 800 px = ~16.67 ms, completely independent of the SW loop"

**Notes:** "Writes can land mid-scan; there's no vsync latching. Tearing window = duration of writes (~us) ≪ 16.67 ms, so visible artifacts are rare and brief."

### Task 6.2: `05-cross-cutting/02_vga_timing.d2`

**Read:** `hw/vga_counters.sv`.

**Header:**
```
# Title:       VGA 640x480@60 Hz timing
# Section:     05-cross-cutting
# Documents:   hw/vga_counters.sv
# Defends:     "What are the porch / sync timings and pixel clock?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes (linear; class `signal` for waveform-style blocks):**
- `h_lane` (container labelled "Horizontal (per line) — 800 px total at 25 MHz pixel clock"):
  - `h_active` "Active video: 640 px (hcount 0..639)"
  - `h_fp` "Front porch: 16 px"
  - `h_sync` "HSYNC pulse (low): 96 px"
  - `h_bp` "Back porch: 48 px"
- `v_lane` (container labelled "Vertical (per frame) — 525 lines at 60 Hz"):
  - `v_active` "Active: 480 lines (vcount 0..479)"
  - `v_fp` "Front porch: 10"
  - `v_sync` "VSYNC pulse (low): 2"
  - `v_bp` "Back porch: 33"

**Notes:** "`VGA_BLANK_n` is asserted only during the 640×480 active region. `VGA_CLK = hcount[0]` = 25 MHz."

### Task 6.3: `05-cross-cutting/03_pixel_pipeline_timing.d2`

**Read:** `hw/entity_drawer.sv:280-358`.

**Header:**
```
# Title:       Entity-drawer 2-cycle pixel pipeline
# Section:     05-cross-cutting
# Documents:   hw/entity_drawer.sv
# Defends:     "Why is there a 1-cycle delay between px,py and the pixel color?"
# Detail:      block
```

**Direction:** `direction: right`

**Nodes (3 columns representing cycle N-1, N, N+1):**
- `cyc_n_minus_1` "px,py = X-1\nstage 1 issues ROM address for X-1"
- `cyc_n` "px,py = X\nROM returns pixel for X-1\nstage 2 mux uses registered hits + ROM(X-1)"
- `cyc_n_plus_1` "color_out (registered) for X-1 arrives at color_palette"

**Notes:** "Net effect: color_out lags px,py by one pixel clock. VGA timing tolerates this — we just align by feeding px,py directly to bg_grid (also delayed implicitly via bg_color_d)."

### Task 6.4: `05-cross-cutting/04_build_pipeline.d2`

**Read:** `hw/Makefile` (whole file), `sw/Makefile` (whole file). Do NOT include CI / GitHub Actions content — `.github/` is absent on this branch (verified, and stated in `doc/guide/README.md:61`).

**Header:**
```
# Title:       Build pipeline (HW + SW)
# Section:     05-cross-cutting
# Documents:   hw/Makefile, sw/Makefile
# Defends:     "How is everything built and where do artifacts come from?"
# Detail:      block
```

**Direction:** `direction: down`

**Containers / Nodes:**
- `hw_lane` (label "HW (workstation, Quartus)")
  - `make_qsys` "make qsys -> qsys-generate --synthesis=VERILOG"
  - `make_quartus` "make quartus -> quartus_sh --flow compile -> .sof"
  - `make_rbf` "make rbf -> quartus_cpf -> .rbf"
  - `make_dtb` "make dtb -> sopc2dts | dtc -> .dtb (requires embedded_command_shell.sh)"
- `sw_lane` (label "SW (on-board native build by default; cross-compile via CC/ARCH/CROSS_COMPILE/KERNEL_SOURCE vars)")
  - `make_module` "make module -> kbuild against /usr/src/linux-headers-$(uname -r) -> pvz_driver.ko"
  - `make_pvz` "make pvz -> gcc -Wall -O2 -o pvz main.c game.c render.c input.c -lpthread"
- `sdcard` (class `external`, "SD card: .rbf + .dtb on the FAT partition; Linux rootfs on the ext partition")
- `board_install` (class `external`, "Board side: copy pvz_driver.ko + pvz onto board; insmod + run")

**Edges:** hw_lane outputs go to sdcard; sw_lane outputs go to board_install. sdcard -> board_install (the board reads it at boot).

**Notes:** "CI/CD, `deploy.sh`, and test programs mentioned in the stale top-level `README.md` and `CLAUDE.md` are not present on this branch. `doc/guide/README.md:61` confirms."

### Task 6.5: `05-cross-cutting/05_boot_flow.d2`

**Read:** `doc/guide/08-build-and-run.md` (boot/deploy section as documented on this branch). `deploy.sh` is absent — do NOT diagram it.

**Header:**
```
# Title:       Cold boot (preloader -> U-Boot -> Linux -> insmod)
# Section:     05-cross-cutting
# Documents:   doc/guide/08-build-and-run.md (boot/install steps), sw/pvz_driver.c (probe)
# Defends:     "How does the board come up from power-on to a running game?"
# Detail:      block
```

**Direction:** `direction: down` using `shape: sequence_diagram`.

**Participants (single lane — there is no deploy.sh on this branch):**
- `power`, `preloader`, `uboot`, `kernel`, `rootfs`, `user`, `pvz_drv`, `pvz`

**Messages (in order):**
1. `power -> preloader`: "power on; BootROM hands off"
2. `preloader -> uboot`: "load U-Boot from SD"
3. `uboot -> uboot`: "fatload mmc 0:1 ${fpgadata} soc_system.rbf"
4. `uboot -> uboot`: "fpga load 0 ${fpgadata} ${filesize}  // program FPGA fabric"
5. `uboot -> uboot`: "run bridge_enable_handoff  // open HPS-to-FPGA bridge"
6. `uboot -> kernel`: "boot Linux with soc_system.dtb"
7. `kernel -> rootfs`: "mount, exec /sbin/init"
8. `user -> kernel`: "insmod pvz_driver.ko  // manual on serial console"
9. `kernel -> pvz_drv`: "pvz_init -> platform_driver_probe -> pvz_probe\nmisc_register('/dev/pvz')"
10. `user -> pvz`: "./pvz"
11. `pvz -> pvz_drv`: "open('/dev/pvz'); ioctl(PVZ_WRITE_REG) loop at 60 Hz"

**Notes:** "`deploy.sh`, GitHub Releases, and an automated `download/install/run/test` workflow are mentioned in the stale top-level `README.md` but DO NOT exist on this branch. Boot and install are manual via the serial console."

---

## Phase 7 — Flow tier completion (5 tasks)

### Task 7.1: `06-flow/02_place_plant.d2`

**Read:** `sw/game.c` (place_plant logic, sun deduction), `sw/render.c`, `hw/entity_drawer.sv:108-264` (selector + plant layers).

**Header:**
```
# Title:       Place a plant (TAB + SPACE flow)
# Section:     06-flow
# Documents:   sw/input.c, sw/game.c, sw/render.c, hw/entity_drawer.sv
# Defends:     "What happens between pressing SPACE and seeing the plant?"
# Detail:      block
```

**Direction:** `direction: down`

**Sequence (linear; class `sw_module` for SW nodes, `hw_module` for HW nodes):**
1. `user_tab` (class `external`) "user presses TAB"
2. `input_tab` "input.c -> action INPUT_TAB"
3. `state_sel` "game.c: selected_plant = (selected_plant == 0) ? 1 : 0"
4. `render_sel` "render.c writes word 50 (SELECTED)"
5. `hw_selector_border` "entity_drawer: selector border moves to other box next frame"
6. `user_space` (class `external`) "user presses SPACE"
7. `input_space` "input.c -> action INPUT_PLACE"
8. `cost_check` "game.c: if sun >= PLANT_COST and grid cell empty"
9. `state_place` "grid[r][c].type = (selected_plant==0)? PEASHOOTER : SUNFLOWER; sun -= 50"
10. `render_grid` "render.c writes word 0 or 1 (PLANTS/SUNFLOWER) and word 49 (SUN)"
11. `hw_layer` "entity_drawer: plant_present/sunflower_present bit -> plant layer draws sprite"

**Edges:** straight vertical chain.

### Task 7.2: `06-flow/03_pea_zombie_collision.d2`

**Read:** `sw/game.c` (collision logic), `sw/game.h` (PEA_DAMAGE, ZOMBIE_HP).

**Header:**
```
# Title:       Pea / zombie collision
# Section:     06-flow
# Documents:   sw/game.c
# Defends:     "How is collision detected and what happens to each entity?"
# Detail:      block
```

**Direction:** `direction: down`

**Steps:**
1. `pea_advance` "each frame: pea.x_pixel += PEA_SPEED (=2)"
2. `same_row_check` "for each zombie in pea.row that is active"
3. `bbox_overlap` "if zombie.x_pixel <= pea.x_pixel <= zombie.x_pixel + ZOMBIE_WIDTH"
4. `apply_damage` "zombie.hp -= PEA_DAMAGE (=1); pea.active = 0"
5. `kill_check` "if zombie.hp <= 0: zombie.active = 0; zombies_killed++"
6. `render_clear` "render.c writes word 40+i with alive=0; word 32+i with alive=0"
7. `hw_layer_off` "entity_drawer: pea_alive[i] and/or zombie_alive[i] = 0 -> layers stop drawing"

### Task 7.3: `06-flow/04_zombie_eats_plant.d2`

**Read:** `sw/game.c` (eat logic), `sw/game.h` (PLANT_HP, ZOMBIE_EAT_COOLDOWN).

**Header:**
```
# Title:       Zombie eats plant
# Section:     06-flow
# Documents:   sw/game.c
# Defends:     "How does the eat loop work and when do zombies resume walking?"
# Detail:      block
```

**Direction:** `direction: down`

**Steps:**
1. `enter_cell` "zombie x_pixel aligned with plant cell (col*64 + GAME_AREA_X) and grid[row][col].type != PLANT_NONE"
2. `start_eating` "zombie.eating = 1; zombie.eat_timer = ZOMBIE_EAT_COOLDOWN (=60); zombie does not move"
3. `bite_tick` "every frame eat_timer--; on 0 -> grid[row][col].hp -= 1; reset timer"
4. `plant_dead` "if grid[r][c].hp <= 0: grid[r][c].type = PLANT_NONE"
5. `resume_walk` "zombie.eating = 0; zombie continues moving leftward next tick"
6. `render_update` "render.c writes word 0 / word 1 with bit cleared; zombie word updated with new x"
7. `hw_layer_change` "entity_drawer: plant_present bit clears; zombie position advances"

### Task 7.4: `06-flow/05_sun_economy.d2`

**Read:** `sw/game.c` (sun accumulator), `sw/game.h` (SUN_INTERVAL, SUN_INCREMENT).

**Header:**
```
# Title:       Sun economy (8s tick)
# Section:     06-flow
# Documents:   sw/game.c, hw/entity_drawer.sv (sun HUD)
# Defends:     "Where does sun come from and how is it shown?"
# Detail:      block
```

**Direction:** `direction: down`

**Steps:**
1. `frame_count` "g->frame_count++ each tick"
2. `interval_check` "if (frame_count % SUN_INTERVAL == 0) // SUN_INTERVAL = 480 frames = 8s"
3. `accum` "sun = min(sun + SUN_INCREMENT, max)"
4. `render_sun` "render.c writes word 49 (SUN)"
5. `hw_hud` "entity_drawer sun HUD: 10 blocks; block i lit when sun_value >= (i+1)*50"

### Task 7.5: `06-flow/06_insmod_probe.d2`

**Read:** `sw/pvz_driver.c:61-99` (`pvz_probe`), `:127-131` (`pvz_init` uses `platform_driver_probe`, NOT `platform_driver_register`), `hw/pvz_top_hw.tcl` (compatible string `csee4840,pvz_gpu-1.0`).

**Header:**
```
# Title:       insmod -> probe -> /dev/pvz
# Section:     06-flow
# Documents:   sw/pvz_driver.c (pvz_init, pvz_probe), hw/pvz_top_hw.tcl
# Defends:     "Trace driver attach in detail."
# Detail:      block
```

**Direction:** `direction: right` (use `shape: sequence_diagram`).

**Participants:** `user`, `insmod`, `kernel`, `pdrv_probe` (`platform_driver_probe`), `dt_walker`, `probe_fn` (`pvz_probe`), `misc`, `of_addr`, `req_mem`, `iomap`, `devfs`.

**Messages (match the source order in `pvz_probe`):**
1. `user -> insmod`: "insmod pvz_driver.ko"
2. `insmod -> kernel`: "load ELF, run init_module() == pvz_init()"
3. `kernel -> pdrv_probe`: "platform_driver_probe(&pvz_driver, pvz_probe)\n(registers the driver and probes immediately)"
4. `pdrv_probe -> dt_walker`: "walk DT, look for of_match_table entries"
5. `dt_walker -> probe_fn`: "found csee4840,pvz_gpu-1.0 at sopc@0/...\n-> call pvz_probe(pdev)"
6. `probe_fn -> misc`: "misc_register(&pvz_misc_device)\n-> /dev/pvz appears  (BEFORE iomap)"
7. `misc -> devfs`: "create /dev/pvz"
8. `probe_fn -> of_addr`: "of_address_to_resource(of_node, 0, &dev.res)"
9. `probe_fn -> req_mem`: "request_mem_region(dev.res.start, ...)"
10. `probe_fn -> iomap`: "of_iomap(of_node, 0) -> dev.virtbase"
11. `probe_fn -> kernel`: "pr_info('initialized at 0x%08lx', dev.res.start) -> return 0"

**Notes:**
- "`pvz_init` uses the legacy `platform_driver_probe()`, which combines registration with an immediate probe if a matching DT node is already present. Fine here because the DT entry is fixed at boot."
- "Source order matters: `misc_register` is called BEFORE the I/O window is mapped (`pvz_driver.c:65,83`). The diagram preserves this order."
- "Verify success on the board with `dmesg | tail` (look for `pvz_gpu: initialized at 0x...`) and `ls /dev/pvz`."

---

## Phase 8 — Finalize (3 tasks)

### Task 8.1: Verify the catalog renders end-to-end

- [ ] **Step 1: Clean and re-render everything**

```bash
make -C doc/diagrams clean
make -C doc/diagrams all
```
Expected: exit 0. Count outputs:
```bash
find doc/diagrams -name '*.d2' ! -name 'common.d2' | wc -l   # 35
find doc/diagrams -name '*.svg' | wc -l                       # 35
find doc/diagrams -name '*.png' | wc -l                       # 35
```

If the counts disagree, find the offender:
```bash
for f in $(find doc/diagrams -name '*.d2' ! -name 'common.d2'); do
  test -f "${f%.d2}.svg" || echo "missing svg: $f"
  test -f "${f%.d2}.png" || echo "missing png: $f"
done
```

- [ ] **Step 2: Commit any newly-rendered files**

If `git status` shows changed `.svg`/`.png` (because rendering is sensitive to D2 version), commit them:
```bash
git add doc/diagrams
git -c commit.gpgsign=false commit -m "docs(diagrams): re-render full catalog (35/35)"
```
Skip if the working tree is already clean.

### Task 8.2: Refresh `README.md` cross-references

- [ ] **Step 1: Confirm every link in `doc/diagrams/README.md` resolves**

```bash
grep -oE '\(([0-9]{2}-[^)]+\.d2)\)' doc/diagrams/README.md | tr -d '()' | while read p; do
  test -f "doc/diagrams/$p" || echo "broken link: $p"
done
```
Expected: no output (all 35 links resolve).

- [ ] **Step 2: If any link is broken, fix the README and commit**

```bash
git add doc/diagrams/README.md
git -c commit.gpgsign=false commit -m "docs(diagrams): fix README cross-references"
```

### Task 8.3: Spot-check accuracy against source code

The diagrams must agree with the real source. Pick the four hot-spot / hero diagrams and do a deliberate cross-check:

- [ ] **Step 1: Verify `02-hw/07_entity_drawer_signals.d2` against `hw/entity_drawer.sv`**

For each port mentioned in the diagram, grep:
```bash
grep -nE 'input  logic|output logic' hw/entity_drawer.sv
```
Bit-widths in the diagram must match `[N:0]` declarations. Also confirm the diagram says "color_out is combinational (always_comb at line 330), NOT a separately registered output." Fix the diagram if any mismatch.

- [ ] **Step 2: Verify `02-hw/09_pvz_top_regfile_signals.d2` against `hw/pvz_top.sv:94-148`**

The address comparisons in the diagram (`addr<40`, `addr<48`, `addr==48`, `addr==49`, `addr==50`) must match the source's `else if` chain exactly. Fix if drifted.

- [ ] **Step 3: Verify `04-interface/03_register_map_cheatsheet.d2` against `hw/pvz_top.sv:8-28` (header comment) and `sw/pvz.h:41-49` (PVZ_REG_*)**

Word indices, field names, and bit ranges must match all three sources.

- [ ] **Step 4: Verify `06-flow/01_one_frame.d2` against `sw/main.c:107-150` loop body and `sw/game.c:288-302` `game_update`**

The phases in the diagram must match the source exactly:
1. main: `process_input(&gs)` (sw/main.c:114)
2. main: `game_update(&gs)` (sw/main.c:119)
3. main: `render_frame(&gs)` (sw/main.c:122)
4. main: `usleep(FRAME_USEC - elapsed)` (sw/main.c:149)

And the phases inside `game_update` (sw/game.c:295-301) in this order:
`update_sun -> update_spawning -> update_firing -> update_projectiles -> update_zombies -> check_collisions -> check_win`.

Do NOT name anything `game_tick` — that function does not exist.

- [ ] **Step 5: Commit any fixes from steps 1–4**

```bash
git add doc/diagrams
git -c commit.gpgsign=false commit -m "docs(diagrams): cross-check fixups against source-of-truth"
```
Skip if no diagrams needed correction.

---

## Self-Review Checklist (for the plan author)

1. **Spec coverage.** Each spec section has tasks:
   - §3 Folder layout → Task 0.1
   - §3 `common.d2` → Task 0.2
   - §3 D2 conventions → Task 0.2 (in `common.d2`) + Phase 1 examples (use `...@../common.d2` — no colon)
   - §3 Per-file header comment → embedded in every diagram task
   - §4 Catalog of 35 diagrams → Tasks 1.1–1.4 + 2.1–7.5 (one task per diagram; `06_test_architecture.d2` dropped because the test surfaces don't exist on this branch)
   - §5 Rendering/Makefile → Task 0.3
   - §6 Acceptance criteria 1 (all .d2 exist) → Phase 8.1 count check (35 each)
   - §6 Acceptance criteria 2 (common.d2 imported) → Phase 1 examples + per-task header convention
   - §6 Acceptance criteria 3 (renders clean) → Phase 8.1
   - §6 Acceptance criteria 4 (factually grounded) → Phase 8.3 cross-check
   - §6 Acceptance criteria 5 (README index) → Task 0.4 + Phase 8.2 check
   - §7 Out of scope → no tasks added (correctly excluded)

2. **Placeholder scan.** No "TBD", "TODO", "fill in", or "similar to Task N" appears. Where a task lists a structured spec instead of inline D2, the spec enumerates concrete node IDs, labels, and edges — no placeholder content.

3. **Type / name consistency.**
   - Class names (`hw_module`, `sw_module`, `kernel_module`, `register`, `signal`, `bus`, `fsm_state`, `external`, `note`, `layer`) introduced in Task 0.2 are used consistently in every later task.
   - All shell snippets run from the `v5-cursor-controller` worktree root with repo-relative paths (no `v5-cursor-controller/` prefix inside the worktree).
   - Register-map fields use the exact names in the source: `PLANTS`, `SUNFLOWER`, `ZOMBIE[i]`, `PEA[i]`, `CURSOR`, `SUN`, `SELECTED` (matching `pvz.h:42-48` and `pvz_top.sv:9-20`).
   - SW function names match the source: `process_input` (sw/main.c:34), `game_update` (sw/game.c:288), `render_frame` (sw/render.c). The name `game_tick` does NOT appear anywhere in the plan.
   - Driver flow matches the source order: `platform_driver_probe` (NOT `platform_driver_register`) in `pvz_init` at sw/pvz_driver.c:130; inside `pvz_probe`, `misc_register` (line 65) runs BEFORE `of_address_to_resource` / `request_mem_region` / `of_iomap` (lines 71/77/83).
   - HW facts: `bg_grid` background = `COL_BLUE` (idx 13) outside the lawn (hw/bg_grid.sv:48); `entity_drawer.color_out` is combinational (`always_comb` at hw/entity_drawer.sv:330), not a separately registered output; the decoder uses `else if (address < 6'd40)` so words 2..31 alias into `zombie[address[2:0]]` (hw/pvz_top.sv:122).
