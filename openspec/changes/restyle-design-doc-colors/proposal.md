## Why

The design document (`doc/design-document/design-document.tex`) currently renders with LaTeX's default article typography and xcolor's generic named colors (`NavyBlue`, `ForestGreen`, `RoyalBlue`, plus raw `blue!8` / `orange!12` / `green!10` fills). It reads as a functional academic paper: correct but characterless. For an embedded-systems design doc — which is closer to a product/game spec than a journal submission — that default presentation undersells the work.

A custom palette has been selected (`doc/design-document/color/color-design.jpg`) and extracted: five colors running warm-to-cool from deep plum through magenta, pale lavender, sky blue, and royal blue. The palette deserves a restyle that uses it across block diagrams, section headings, tables, code listings, links, and the title page, so the document feels like a cohesive designed artifact.

## What Changes

- Replace all default/generic colors with a five-color palette defined once in the preamble as named LaTeX colors (`pvzPlum`, `pvzMagenta`, `pvzMist`, `pvzSky`, `pvzRoyal`) plus two contrast helpers (`pvzInk`, `pvzOnDark`).
- Apply a **warm/cool role mapping** across all TikZ figures: HPS-side blocks use the magenta family, FPGA-side blocks use the blue family, external peripherals use Mist, and the LW HPS-to-FPGA bridge uses Plum as a "hero" accent.
- Restyle **section headings** using `titlesec`: vertical plum-to-magenta gradient side-bar, a tracked-caps magenta "SECTION NN" eyebrow above the title, and a thin Mist rule below.
- Rebuild the **code listing theme** in `listings` with proper syntax highlighting for C, Verilog, TCL, and plain text: plum keywords, royal types, magenta strings, slate comments, mist code background with a royal header strip showing language and filename.
- Retune **table styling** (booktabs rules in plum, small-caps tracked uppercase headers in plum, optional soft Mist zebra striping on wide tables).
- Restyle **figure captions** with a plum small-caps label ("Figure 7:").
- Retune **links** (`hyperref`): `linkcolor=pvzRoyal`, `urlcolor=pvzMagenta`, `citecolor=pvzPlum`.
- Restyle the **title page** and **table of contents** to anchor the document's visual identity (gradient rule, tinted section numbers).
- Reflow the document for a **spacious, relaxed read**: `microtype`, `\linespread{1.15}`, slightly wider top/bottom margins (1.2in), generous figure/caption spacing.
- Enforce a **contrast rule**: white text appears only on Plum, Magenta, or Royal fills; Sky and Mist always carry dark text (Ink or Royal).
- Preserve **Figure 7's ForestGreen grass** (the screen-layout checkerboard at `design-document.tex:1538–1557`). Other non-grass greens elsewhere in the document (external-peripheral block fills in Figs 1 and 4; the `y[8:0]` field in Fig 5) swap to the new palette per the role mapping — those are structural greens, not grass.

**Non-goals**: rewriting prose, changing tables' data, altering figure semantics, swapping body/math fonts, adding new content, touching `hw/` or `sw/` source. This is a pure visual/typographic restyle of one file (`design-document.tex`).

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `design-document`: the structure/content requirements established under `update-design-doc-v2dino` are preserved; only the visual presentation changes.

## Impact

- **Files touched**:
  - `doc/design-document/design-document.tex` (preamble rewrite + in-body style-class replacements).
  - `doc/design-document/design-document.pdf` (regenerated).
  - `doc/design-document/color/color-design.jpg` (already present; reference only).
- **New LaTeX packages** (all in standard TeX Live Basic): `titlesec` (section restyle), `caption` (caption restyle), `tocloft` (TOC restyle), `colortbl` (table row coloring), `microtype` (typographic polish). Already loaded: `xcolor` with `dvipsnames,svgnames,x11names`; `tikz` with libraries; `hyperref`; `parskip`. No exotic or fragile packages.
- **Code touched**: none.
- **Reviewers**: teammates and instructor — the restyle must not obscure technical content; legibility of block diagrams and tables is the success criterion beyond aesthetic preference.
