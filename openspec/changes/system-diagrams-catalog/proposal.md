## Why

The PvZ-on-DE1-SoC final design review is a multi-hour session where the professor may probe any line of code in `v5-cursor-controller/`. The current `README.md` is stale (still references `shape_renderer.sv` and `linebuffer.sv` that no longer exist) and there are no diagrams documenting the system as actually built — combinational racing-the-beam `entity_drawer`, flat 51-word Avalon register file, sprite-based renderer with two plant types. Without a navigable diagram set, the presenter cannot pull up the right picture on demand when a deep question lands on a specific module, signal, or boundary.

## What Changes

- Add a comprehensive D2 diagram catalog at `doc/diagrams/` covering the system end-to-end as built in `v5-cursor-controller/`.
- Produce **35 diagrams** organized into 6 sections (system context, FPGA hardware, HPS software, HW/SW interface, cross-cutting concerns, cross-layer flows).
- Provide shared scaffolding: `common.d2` (consistent classes for hw_module / sw_module / register / signal / bus / fsm_state / external / note), a top-level `Makefile` that renders every `.d2` to sibling `.svg` and `.png`, and a `README.md` index listing every diagram with a one-line description and source-file pointers.
- Two diagrams escalate to **signal-level detail** (with bit-widths) for hot-spot modules: `02-hw/07_entity_drawer_signals.d2` and `02-hw/09_pvz_top_regfile_signals.d2`.
- Ship rendered `.svg` and `.png` next to each `.d2` source, all committed to git, so the presenter does not depend on local `d2` availability at review time.
- **Out of scope** (explicit): legacy `main/` (Milestone 1 primitive-shape) diagrams, planned-but-unbuilt features (USB gamepad, audio, additional plant types), rejected/alternative designs, test/CI/deploy lanes (`hw/tb/`, `sw/test/`, `.github/`, `deploy.sh` do not exist on this branch), LaTeX/slide-deck integration.

## Capabilities

### New Capabilities

- `system-diagrams`: D2 source catalog plus rendered artifacts that document `v5-cursor-controller/` for the final design review. Covers folder layout, header-comment convention, shared style classes, per-section catalog contents, signal-level escalation rules, rendering toolchain, and acceptance criteria.

### Modified Capabilities

<!-- None. This change introduces documentation; no existing requirement specs are modified. -->

## Impact

- **New files only.** No source code under `hw/` or `sw/` is modified. Anything the diagrams document is read-only reference.
- **New directory:** `doc/diagrams/` with subfolders `01-system/`, `02-hw/`, `03-sw/`, `04-interface/`, `05-cross-cutting/`, `06-flow/`, plus `common.d2`, `Makefile`, `README.md`.
- **New tool dependency at build time:** `d2` CLI (already installed at `/home/linuxbrew/.linuxbrew/bin/d2`, v0.7.1). Not a runtime dependency — rendered outputs are committed.
- **Repository size:** ~70 generated files (35 × `.svg` + 35 × `.png`) committed alongside sources. Each is small (<200 KB typical).
- **No impact** on FPGA bitstream, kernel module, game binary, CI, or board deployment.
