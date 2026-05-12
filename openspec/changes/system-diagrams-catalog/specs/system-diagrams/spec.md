## ADDED Requirements

### Requirement: Catalog location and folder layout

The diagram catalog SHALL live at `doc/diagrams/` under the `v5-cursor-controller/` repository root. It MUST contain exactly six section subfolders named `01-system/`, `02-hw/`, `03-sw/`, `04-interface/`, `05-cross-cutting/`, and `06-flow/`, plus the shared files `common.d2`, `Makefile`, and `README.md` at the catalog root.

#### Scenario: Folder tree present after scaffolding

- **WHEN** Phase 0 scaffolding tasks are complete
- **THEN** `ls -1 doc/diagrams/` lists `01-system`, `02-hw`, `03-sw`, `04-interface`, `05-cross-cutting`, `06-flow`, `common.d2`, `Makefile`, `README.md` and nothing else.

#### Scenario: Diagram files placed under correct section

- **WHEN** any new diagram is added
- **THEN** it MUST live under exactly one of the six section subfolders and follow the `NN_short-name.d2` filename pattern, where `NN` is a two-digit decimal prefix that determines presentation order within the section.

### Requirement: Required diagram set

The catalog SHALL contain at least 35 diagrams distributed across the six sections as follows: 4 in `01-system/`, 9 in `02-hw/`, 7 in `03-sw/`, 4 in `04-interface/`, 5 in `05-cross-cutting/`, 6 in `06-flow/`. Each diagram MUST be uniquely named within its section.

#### Scenario: All 35 diagrams present

- **WHEN** the catalog is considered complete
- **THEN** `find doc/diagrams -name '*.d2' -not -name 'common.d2' | wc -l` returns at least 35, and per-section counts match (4, 9, 7, 4, 5, 6).

#### Scenario: Each diagram targets a documented file

- **WHEN** a diagram is added to the catalog
- **THEN** every module, register, signal, function, or constant named in the diagram MUST exist in the corresponding source file under `hw/` or `sw/` of `v5-cursor-controller/`.

### Requirement: Shared style classes via `common.d2`

`doc/diagrams/common.d2` SHALL define a single `classes:` block containing at minimum the classes `hw_module`, `sw_module`, `kernel_module`, `register`, `signal`, `bus`, `fsm_state`, `external`, `note`, and `layer`. Every other `.d2` file in the catalog MUST import it via the spread syntax `...@../common.d2` as the first non-comment statement.

#### Scenario: Shared classes defined

- **WHEN** `common.d2` is opened
- **THEN** it declares a top-level `classes:` block with at least the ten class names listed above and uses a consistent palette (blue family for hardware, green for userspace software, amber for kernel module).

#### Scenario: Every diagram imports common.d2

- **WHEN** any `.d2` file under `doc/diagrams/{01-system,02-hw,03-sw,04-interface,05-cross-cutting,06-flow}/` is opened
- **THEN** its first non-comment line is `...@../common.d2` (using the unprefixed spread form that D2 0.7.1 accepts; the `...: @file` form MUST NOT be used).

### Requirement: Mandatory diagram header comment

Every diagram `.d2` file (excluding `common.d2`) SHALL open with a five-line header comment block containing the fields `# Title:`, `# Section:`, `# Documents:`, `# Defends:`, and `# Detail:`.

#### Scenario: Header present and complete

- **WHEN** any catalog diagram is opened
- **THEN** the first five non-blank lines are comment lines beginning with `# Title:`, `# Section:`, `# Documents:`, `# Defends:`, and `# Detail:` in that order, with no placeholder text (`<short title>`, `<…>`, etc.) remaining.

#### Scenario: Documents field cites real source files

- **WHEN** a diagram's `# Documents:` line is checked
- **THEN** every file listed exists under `hw/` or `sw/` of `v5-cursor-controller/`.

### Requirement: Hot-spot signal-level diagrams

The catalog SHALL include at least two signal-level diagrams with explicit bit-widths: one for the `entity_drawer` datapath and one for the `pvz_top` register-file decoder. Both MUST set `# Detail: signal` in their header.

#### Scenario: Entity drawer signal-level diagram exists

- **WHEN** the catalog is built
- **THEN** `doc/diagrams/02-hw/07_entity_drawer_signals.d2` exists, sets `# Detail: signal`, and labels all inter-stage signals with their bit-widths consistent with `hw/entity_drawer.sv`.

#### Scenario: Register file signal-level diagram exists

- **WHEN** the catalog is built
- **THEN** `doc/diagrams/02-hw/09_pvz_top_regfile_signals.d2` exists, sets `# Detail: signal`, shows Avalon `address[5:0]`, `writedata[31:0]`, `write`, and `chipselect`, and explicitly annotates the absence of vsync latching (one-frame tearing window) plus the `else if (address < 6'd40)` aliasing gotcha at `hw/pvz_top.sv:122`.

### Requirement: Code-grounded accuracy corrections

The diagrams SHALL reflect the system as actually built. Specifically, where the stale `v5-cursor-controller/README.md` references removed modules, the diagrams MUST follow the code. The following facts MUST be represented correctly:

- `entity_drawer.color_out` is combinational (`always_comb` at `hw/entity_drawer.sv:330`), not separately registered.
- Kernel module uses `platform_driver_probe` (not `platform_driver_register`) in `pvz_init` (`sw/pvz_driver.c:130`).
- `pvz_probe` call order is `misc_register` → `of_address_to_resource` → `request_mem_region` → `of_iomap` (`sw/pvz_driver.c:61`).
- Main game loop calls `process_input(&gs)` → `game_update(&gs)` → `render_frame(&gs)` (`sw/main.c:114,119,122`); no function called `game_tick(action)` exists.
- `game_update()` phase order is sun → spawning → firing → projectiles → zombies → collisions → win (`sw/game.c:295-301`).
- Register-file decoder uses `else if (address < 6'd40)` (`hw/pvz_top.sv:122`), causing words 2..31 to alias into the zombie slot.

#### Scenario: No removed modules appear in any diagram

- **WHEN** any diagram is inspected
- **THEN** the strings `shape_renderer.sv` and `linebuffer.sv` MUST NOT appear in any catalog `.d2` file, because those modules do not exist in `v5-cursor-controller/hw/`.

#### Scenario: Combinational color_out depicted correctly

- **WHEN** diagrams `02-hw/06_entity_drawer_pipeline.d2` or `02-hw/07_entity_drawer_signals.d2` are inspected
- **THEN** they depict `color_out` as combinational output (no register symbol on the final stage); any pipeline note refers to the 1-cycle latency in the stage-1 hit-signal flops + sprite ROM read, not in a registered output.

#### Scenario: Probe sequence ordering shown correctly

- **WHEN** diagrams `03-sw/02_kernel_driver.d2` or `06-flow/06_insmod_probe.d2` are inspected
- **THEN** they show `pvz_init` calling `platform_driver_probe(&pvz_driver, pvz_probe)`, and `pvz_probe` performing `misc_register` → `of_address_to_resource` → `request_mem_region` → `of_iomap` in that exact order.

#### Scenario: Register aliasing gotcha annotated

- **WHEN** diagrams `04-interface/03_register_map_cheatsheet.d2` and `02-hw/09_pvz_top_regfile_signals.d2` are inspected
- **THEN** both contain an explicit note that the `address < 6'd40` decoder makes words 2..31 alias into the zombie register slot, citing `hw/pvz_top.sv:122`.

### Requirement: Rendered artifacts committed alongside sources

For every `.d2` source file under the section folders, the catalog SHALL commit a sibling `.svg` and `.png` rendering produced by `d2`. The renderings MUST be regenerable from source with no syntax errors.

#### Scenario: SVG and PNG present for every diagram

- **WHEN** the catalog is considered complete
- **THEN** for every `doc/diagrams/<section>/<NN>_<name>.d2` there exist `doc/diagrams/<section>/<NN>_<name>.svg` and `doc/diagrams/<section>/<NN>_<name>.png` files committed to git.

#### Scenario: Renders are reproducible

- **WHEN** `make clean && make all` runs from `doc/diagrams/`
- **THEN** every `.d2` renders to its sibling `.svg` and `.png` with exit code 0 from `d2`.

### Requirement: Build automation via Makefile

`doc/diagrams/Makefile` SHALL provide at minimum these phony targets: `all`, `clean`, and one per-section target (`01-system`, `02-hw`, `03-sw`, `04-interface`, `05-cross-cutting`, `06-flow`). `make all` MUST render every `.d2` under the section folders to a sibling `.svg` and `.png` using the `d2` CLI. `make clean` MUST remove generated `.svg` and `.png` files while preserving sources.

#### Scenario: make all renders everything

- **WHEN** `make all` runs from `doc/diagrams/` with `d2` on PATH
- **THEN** every diagram source produces sibling `.svg` and `.png` files, the command exits 0, and `common.d2` is not rendered (it is import-only).

#### Scenario: make clean preserves sources

- **WHEN** `make clean` runs from `doc/diagrams/`
- **THEN** all `.svg` and `.png` files under section folders are removed, but every `.d2` source file (including `common.d2`) remains untouched.

#### Scenario: Missing d2 binary gives a helpful error

- **WHEN** `make all` runs without `d2` on PATH
- **THEN** the Makefile prints a clear message naming `d2` as the missing tool and exits non-zero, rather than producing an opaque shell error.

### Requirement: Catalog README index

`doc/diagrams/README.md` SHALL list every diagram in the catalog with at minimum its file path, a one-line description, and a pointer to the source file(s) it documents. The index MUST be organized by section in the same order as the section folders.

#### Scenario: Every diagram listed in README

- **WHEN** the catalog is considered complete
- **THEN** `README.md` contains an entry for every `.d2` file under the section folders, and the entry count matches the file count.

#### Scenario: README explains prerequisites

- **WHEN** `README.md` is opened
- **THEN** it documents that `d2` (v0.7.1 or newer) is required to re-render diagrams and that committed `.svg`/`.png` files allow read-only viewing without it.

### Requirement: Out-of-scope content excluded

The catalog MUST NOT contain diagrams for the legacy `main/` (Milestone 1) primitive-shape version, planned-but-unbuilt features (USB gamepad, audio/Wolfson codec, additional plant types, levels beyond level 1), rejected/alternative designs, test/CI/deploy lanes (`hw/tb/`, `sw/test/`, `.github/`, `deploy.sh` — none of which exist on this branch), or LaTeX/slide-deck integration assets.

#### Scenario: No legacy or planned-feature diagrams

- **WHEN** the catalog is reviewed
- **THEN** no `.d2` file references `main/`, `linebuffer.sv`, `shape_renderer.sv`, `usb_gamepad`, `wolfson`, `i2s`, `audio_codec`, or other planned-but-unbuilt elements as if they existed.

#### Scenario: No test/CI/deploy diagrams

- **WHEN** the catalog is reviewed
- **THEN** no diagram named `test_architecture` or containing a CI lane or `deploy.sh` lane is present, consistent with `doc/guide/README.md:61` ("not yet built on this branch").
