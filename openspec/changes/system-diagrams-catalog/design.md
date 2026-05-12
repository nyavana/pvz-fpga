## Context

`v5-cursor-controller/` is the final-review version of the PvZ-on-DE1-SoC project. The actual code (combinational `entity_drawer.sv`, flat 51-word Avalon register file, sprite-based renderer, two plant types selected via TAB) has diverged from the project's earlier Milestone-1 design (primitive-shape renderer with a shape table). The `README.md` references modules (`shape_renderer.sv`, `linebuffer.sv`) that no longer exist. The review is multi-hour with deep-dive questions possible on any line of code in `hw/` or `sw/`.

A prior brainstorming session produced a design spec (`docs/superpowers/specs/2026-05-12-system-diagrams-design.md`) and a detailed implementation plan (`docs/superpowers/plans/2026-05-12-system-diagrams.md`). Both are post-Codex-review and reflect the actual code. The decisions captured below come from that prior work and are restated here so this change is self-contained.

D2 v0.7.1 is already installed at `/home/linuxbrew/.linuxbrew/bin/d2`. GNU make and git are available.

## Goals / Non-Goals

**Goals:**

- Provide a navigable, fact-checked diagram set that lets the presenter pull up the right picture for any likely question in under 10 seconds.
- Cover the system at three zoom levels: system context, module-internal block-level, and signal-level (with bit-widths) for hot spots.
- Reflect the system **as actually built**, not as originally proposed.
- Make the diagrams cheap to maintain: one shared style file (`common.d2`), one render rule for everything (Makefile), uniform header comments so the source of each diagram is auditable.
- Commit rendered `.svg` + `.png` next to each source so the diagrams are viewable without a local `d2` install.

**Non-Goals:**

- Diagramming the legacy `main/` (Milestone 1) version.
- Diagramming planned-but-unbuilt features: USB gamepad, audio (Wolfson codec), additional plants, levels beyond level 1.
- Diagramming rejected / alternative designs.
- A diagram for `test_architecture` — `hw/tb/`, `sw/test/`, `.github/`, `deploy.sh` do not exist on this branch.
- Slide-deck integration, LaTeX embedding, or animating the diagrams.
- Fixing the stale `v5-cursor-controller/README.md` (separate cleanup; out of scope here).

## Decisions

### D1. D2 (not Mermaid, not Graphviz, not draw.io)

- **Choice:** D2 v0.7.1.
- **Rationale:** Text source diffs cleanly in git; supports containment, sequence diagrams, grid tables, and shared class imports — all of which are used here. Mermaid lacks shared class imports and is weaker at layered containment. Graphviz lacks sequence diagrams and is verbose. draw.io is a binary format and reviews badly.
- **Alternatives considered:** Mermaid (rejected for class-import gap), Graphviz/DOT (rejected for missing sequence support), PlantUML (extra Java dep), hand-drawn PDFs (no diffs).

### D2. Folder layout: six numbered section folders under `doc/diagrams/`

- **Choice:** `01-system/`, `02-hw/`, `03-sw/`, `04-interface/`, `05-cross-cutting/`, `06-flow/`, each with `NN_short-name.d2` files.
- **Rationale:** Numeric prefixes give deterministic ordering both for `ls` and for review-time presentation. Section split mirrors the boundaries that questions tend to land on (a question is almost always about HW, SW, or the interface).
- **Alternatives considered:** Flat folder (rejected — 35 files is too many to scan), per-module folder (rejected — many diagrams cross module boundaries).

### D3. Shared style classes in `common.d2`, imported via spread

- **Choice:** Every `.d2` file starts with `...@../common.d2` to pick up classes `hw_module`, `sw_module`, `kernel_module`, `register`, `signal`, `bus`, `fsm_state`, `external`, `note`, `layer`.
- **Rationale:** One file controls colors and shapes; visual consistency across 35 diagrams is free. **NOTE:** D2 0.7.1 rejects `...: @file` — must be the unprefixed `...@file` form. This is the kind of subtlety that has burned other projects.
- **Alternatives considered:** Inline styling per file (rejected — guaranteed drift).

### D4. Mandatory header comment in every diagram

- **Choice:** Each `.d2` opens with `# Title:`, `# Section:`, `# Documents:`, `# Defends:`, `# Detail:` lines.
- **Rationale:** `Documents:` lists the source files the diagram is grounded in, so anyone re-checking accuracy knows where to look. `Defends:` documents the likely question this diagram answers, which is the whole point of the catalog.

### D5. Detail level: block by default, signal-level only for hot spots

- **Choice:** Two diagrams escalate to signal-level with bit-widths — `02-hw/07_entity_drawer_signals.d2` (entity drawer datapath) and `02-hw/09_pvz_top_regfile_signals.d2` (Avalon register-file decoder).
- **Rationale:** Signal-level diagrams are expensive to maintain and information-dense. Two is the right number: enough to defend against deep questions on the two hottest spots, few enough not to bury readers.
- **Alternatives considered:** Signal-level for every HW module (rejected — too dense, would not be read), block-only everywhere (rejected — fails the deep-question test).

### D6. Render `.svg` and `.png` for every `.d2`, commit all three

- **Choice:** `make all` renders both formats; both are committed alongside the `.d2` source.
- **Rationale:** `.svg` is the canonical reviewable artifact (scales, searchable text); `.png` is a fallback for environments where SVG renders poorly. Committing the renders means the catalog is usable at review time without a local D2 install — a real risk if the demo machine differs from the dev workstation.
- **Alternatives considered:** Generate on demand only (rejected — review-machine dependency risk), SVG only (rejected — PNG fallback is cheap).

### D7. Phasing: hero diagrams first, then long tail

- **Choice:** Implementation plan lands the four "hero" diagrams (system context, `pvz_top` block, register map cheat sheet, one-frame end-to-end flow) before the remaining 31. Each diagram is its own commit.
- **Rationale:** Hero diagrams are the most likely to be reviewed and revised; landing them first lets the user steer style and accuracy before the long tail is generated. Per-diagram commits keep the review small and rollback cheap.

### D8. Accuracy is grounded in `v5-cursor-controller/` source, not the stale README

- **Choice:** Where the README and code disagree, the diagrams follow the code.
- **Rationale:** The stale README still names removed modules (`shape_renderer.sv`, `linebuffer.sv`). The whole point of this catalog is to be a reliable reference; mirroring the README's errors would defeat that.

### D9. Subagent dispatch for parallel diagram authoring

- **Choice:** Diagrams are produced by dispatched subagents (`Agent` tool, `subagent_type: general-purpose`), batched per coherent section group — not one agent per diagram. The breakdown is: one subagent for the 4 hero diagrams (sequential, run first to anchor style), then six section subagents that fan out in parallel (Section 1 remaining, Section 2 block-level, Section 2 signal-level hot spots, Section 3, Section 4 remaining, Section 5, Section 6 remaining). Eight dispatches total.
- **Rationale:**
  - Keeps the main session's context window clean — diagram authoring re-reads the same source files in each batch, which is wasteful if done in-conversation.
  - Per-section batching means each subagent has a coherent set of source files in front of it and can produce stylistically consistent diagrams within its section.
  - Signal-level diagrams are split into their own dispatch because the work is qualitatively different (deep source reading + bit-width verification), and bundling them with block-level work risks the subagent rushing the precision-sensitive part.
  - Heroes go first and serial: the user reviews the four style-anchor diagrams before parallel dispatches start, so style drift across parallel batches is bounded.
- **Alternatives considered:** One agent per diagram (rejected — 35 agents is wasteful coordination overhead and no shared context within a section); single agent does all 35 (rejected — context bloat, slow, no parallelism); fully in main session (rejected — pollutes main context with source code that the user does not need to see).

### D10. Commit-message convention

- **Choice:** One diagram per commit. Message form `docs(diagrams): add <short-title>` (conventional-commit, lowercase scope). No `Co-Authored-By` line. No mention of Claude, AI assistants, or generation tooling. No emojis. Scaffolding (Task 1.4) and README index (Task 4.1) get their own single commits.
- **Rationale:** Granular per-diagram commits make `git bisect` and review trivial — each diagram lands or rolls back independently. Attribution-free messages match the repo's existing convention (existing `git log` on this branch shows no `Co-Authored-By` lines) and the user's stated preference.
- **Alternatives considered:** One commit per section (rejected — bisect is coarser, single bad diagram contaminates a section), one giant commit (rejected — reviewer can't focus, rollback is all-or-nothing).

### D11. Specific code-level corrections that the diagrams must reflect

These came out of a Codex review of the design spec and must be preserved:

- `entity_drawer.color_out` is **combinational** (`always_comb` at `hw/entity_drawer.sv:330`), not a separately registered output.
- Kernel module uses `platform_driver_probe` (not `platform_driver_register`) in `pvz_init` (`sw/pvz_driver.c:130`).
- Inside `pvz_probe` (`sw/pvz_driver.c:61`) the call order is `misc_register` → `of_address_to_resource` → `request_mem_region` → `of_iomap`.
- Main game loop: `process_input(&gs)` (`sw/main.c:114`) → `game_update(&gs)` (`:119`) → `render_frame(&gs)` (`:122`). No function called `game_tick(action)` exists.
- `game_update()` phase order: sun → spawning → firing → projectiles → zombies → collisions → win (`sw/game.c:295-301`).
- Register-file decoder uses `else if (address < 6'd40)` (`hw/pvz_top.sv:122`), so words 2..31 alias into the zombie slot. This gotcha must be called out in both the cheat sheet and the regfile signal-level diagram.

## Risks / Trade-offs

- **Risk:** D2 not installed on the review machine → `.svg`/`.png` are committed so this does not block reading the catalog. Building new diagrams still needs `d2`, but reviewing does not.
- **Risk:** Source code drifts after diagrams are committed → mitigation is per-diagram `# Documents:` headers naming the source files, so a future reader can audit each diagram against current code. No automated cross-checker.
- **Risk:** 35 diagrams is verbose → if implementation reveals this is too much, the design spec names the two safest cuts: `05-cross-cutting/03_pixel_pipeline_timing.d2` (overlaps with diagram 11) and `03-sw/04_game_loop_fsm.d2` (trivially small FSM). Both kept by default.
- **Trade-off:** Committing 70 generated files inflates repo size. Each render is small (<200 KB); total <15 MB. Worth it for offline viewability.
- **Trade-off:** Signal-level diagrams will need maintenance if bit-widths change. The two hot spots are stable parts of the design; this is judged acceptable.
- **Risk:** D2 `...@file` import syntax is subtle (the `...: @file` form silently fails on 0.7.1). Mitigation: `common.d2` includes a comment documenting the gotcha and the hero diagrams in Phase 1 are concrete worked examples for later phases to copy.

## Migration Plan

Not applicable — this change adds a new `doc/diagrams/` tree alongside existing code. There is nothing to migrate from. Rollback is `rm -rf doc/diagrams/`; nothing else depends on it.

## Open Questions

- **Q1.** Should the `Makefile` use ELK or Dagre as the default D2 layout engine? The plan tries `--layout=elk` first and falls back to Dagre on the warning. Either is acceptable; pick during scaffolding (Task 0.2 / 0.4 in tasks.md) based on which produces more readable hero diagrams. Not a blocker.
- **Q2.** Will the presenter want a single "tour" markdown that walks through the diagrams in a recommended order? Out of scope for this change; `README.md` will list every diagram with a one-line description, which is enough to navigate.
