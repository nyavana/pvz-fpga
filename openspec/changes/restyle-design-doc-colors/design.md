## Context

`doc/design-document/design-document.tex` is the authoritative design artifact for the PvZ-on-FPGA project, currently ~1,987 lines. Its preamble loads `xcolor` with the `dvipsnames,svgnames,x11names` option set and uses generic named colors (`NavyBlue`, `RoyalBlue`, `ForestGreen`) plus tinted fills (`blue!8`, `blue!16`, `orange!12`, `orange!20`, `green!10`, `gray!10`) throughout seven TikZ figures and a handful of listings. The teammate leading this change has chosen a five-color palette (reference image: `doc/design-document/color/color-design.jpg`) with a specific visual intent: "spacious, relaxed, easy to read … feels like a designed document, not a series paper."

The palette carries a deliberate warm-to-cool gradient that lends itself to the HPS/FPGA split that is the document's central architectural story. This spec pins down exactly how the palette maps onto the document's existing structure, so the restyle can be executed without per-figure guesswork.

Stakeholders: the course instructor (reading the PDF for grading), the three teammates (reading and co-authoring the doc), and future-us (regenerating the PDF across branches).

## Goals / Non-Goals

**Goals:**
- Replace every generic color usage in the preamble and TikZ bodies with palette-native named colors.
- Give the document a distinctive visual identity (section headings, code listings, tables, title page) consistent with the palette.
- Improve readability via typography changes (`microtype`, line spacing, margins) without changing the body font.
- Preserve all technical content verbatim — captions, prose, numbers, algorithms, register tables, code excerpts.
- Preserve Figure 7's ForestGreen grass (the gameplay-area checkerboard) since the green there has semantic meaning (grass).
- Enforce a contrast rule so no white text lands on a light palette color.
- Keep the file regenerable with a standard TeX Live install; no exotic packages.

**Non-Goals:**
- Changing any narrative content, figure semantics, or tabular data.
- Swapping body font (Computer Modern stays) or math font.
- Adding new sections, moving sections, or editing prose.
- Adjusting the layered block-diagram structure set by `update-design-doc-v2dino` (the four-diagram split remains).
- Touching `hw/` or `sw/` sources or any build tooling.
- Styling anything outside `design-document.tex` (no restyle of the proposal or appendices stored elsewhere).

## Decisions

### Decision 1: Five-color palette with named LaTeX colors + two contrast helpers

Define once in the preamble, use everywhere:

```latex
\definecolor{pvzPlum}   {RGB}{119,  41,  93}   % #77295D — hero accent
\definecolor{pvzMagenta}{RGB}{195,  79, 162}   % #C34FA2 — HPS emphasis
\definecolor{pvzMist}   {RGB}{237, 241, 253}   % #EDF1FD — backgrounds / soft fills
\definecolor{pvzSky}    {RGB}{119, 183, 240}   % #77B7F0 — FPGA emphasis
\definecolor{pvzRoyal}  {RGB}{ 83, 100, 192}   % #5364C0 — structural / links
\definecolor{pvzInk}    {RGB}{ 26,  26,  26}   % #1A1A1A — body text on light
\definecolor{pvzOnDark} {RGB}{255, 255, 255}   % #FFFFFF — text on Plum/Magenta/Royal
```

**Why named colors, not hex per use-site**: the palette must be consistent. A name enforces intent ("this is the hero accent"); a hex string invites drift. Three aliases are added for block-diagram clarity:

```latex
\colorlet{pvzHPS}      {pvzMagenta}   % HPS emphasis (warm)
\colorlet{pvzFPGA}     {pvzSky}       % FPGA emphasis (cool)
\colorlet{pvzBridge}   {pvzPlum}      % The LW bridge between them
```

**Alternatives considered:**
- *Use xcolor's existing `\definecolorset`*: rejected — adds syntax noise with no benefit for seven fixed colors.
- *Use HTMLModel `\definecolor{pvzPlum}{HTML}{77295D}`*: equivalent to the RGB form, slightly less traditional in academic LaTeX. Chose `RGB` form to match the visible RGB values on the reference image.

### Decision 2: Warm/Cool role mapping for TikZ figures

HPS-side blocks get magenta family (warm); FPGA-side blocks get blue family (cool). External peripherals stay neutral in Mist. The LW Bridge is Plum — visually declaring itself as the single crossing point.

| Element | Fill | Border | Text |
|---|---|---|---|
| HPS software (soft blocks: `gameloop`, `render`, `input`) | `pvzMist` | `pvzMagenta` | `pvzInk` |
| HPS kernel driver (emphasized) | `pvzMagenta` | `pvzMagenta` | `pvzOnDark` |
| FPGA peripheral (soft blocks: `shape_renderer`, `linebuffer`, `palette`, `bg_grid`, `vga_counters`, etc.) | `pvzMist` | `pvzSky` | `pvzInk` |
| FPGA `pvz_top` (emphasized) | `pvzSky` | `pvzRoyal` | `pvzInk` |
| External peripheral (`VGA Monitor`, `USB Gamepad`, `Speaker`) | `pvzMist` | lightgray (`gray!40`) | `pvzInk` |
| LW Bridge rectangle | `pvzPlum` | `pvzPlum` | `pvzOnDark` (if labeled) |
| Data bus arrows (`bus`, `dbus`) | — | `pvzRoyal` | `pvzInk` (for annotations) |

**This must propagate to all seven TikZ figures** (`fig:system-block`, `fig:peripheral-internal`, `fig:scanline-timing`, `fig:scanline-dataflow`, `fig:shape-bits`, `fig:sw-stack`, `fig:screen-layout`), with the explicit exception noted in Decision 8.

**Why warm/cool and not, e.g., HPS=blue / FPGA=magenta**: the warm/cool axis carries the strongest visual "these are different kinds of thing" signal. Magenta-as-software also gives the kernel driver (the most conceptually active HPS component) a more arresting visual weight than "another blue box." The previous document's orange-for-FPGA intuition is preserved in principle (warm = hardware); the new palette just replaces orange with plum/magenta on the HPS side and blue with sky/royal on the FPGA side.

**Alternatives considered:**
- *HPS=cool, FPGA=warm (reverse)*: rejected — the current doc uses blue on the HPS side out of convention; preserving that intuition avoids re-reading effort for readers already familiar with the V2 doc.
- *Monochrome blues for both, plum+magenta as accent only*: rejected — loses the HPS/FPGA-at-a-glance distinction. Good elegance trade, poor pedagogical trade.

### Decision 3: Contrast rule (white-on-white prevention)

White text is allowed only on the three dark palette colors. This must be encoded at the preamble level, not left to per-figure vigilance.

| Background | Text color | Notes |
|---|---|---|
| `pvzPlum` | `pvzOnDark` | Contrast ratio ≈ 9.4 (AAA) |
| `pvzMagenta` | `pvzOnDark` | Contrast ratio ≈ 3.9 (AA large / small-text borderline) — use bold for small labels |
| `pvzRoyal` | `pvzOnDark` | Contrast ratio ≈ 6.3 (AA) |
| `pvzSky` | `pvzInk` | White would fail — Sky is too light |
| `pvzMist` | `pvzInk` | Always dark text |
| white / page | `pvzInk` | Default body |

Every TikZ block style definition must name the text color explicitly; no block style may omit `text=...`.

### Decision 4: Section heading restyle via `titlesec`

Section headings adopt a left-bar + eyebrow + title layout, rendered with `titlesec`. Approximate intended output for a section:

```
┃                          (plum→magenta vertical gradient bar, 3pt wide, height of title)
┃  SECTION 02              (tracked uppercase eyebrow, pvzMagenta, 9pt)
┃  System Block Diagram    (bold sans-serif, pvzInk, 18pt)
   ─────────────────────    (thin pvzMist rule, 0.8pt)
```

```latex
\usepackage{titlesec}
\usepackage{tikz}
\newcommand{\pvzSectionBar}{%
  \tikz[baseline=(a.base)]{
    \node[rectangle, fill=pvzPlum, minimum width=3pt,
          minimum height=1.9em, anchor=base, inner sep=0] (a) {};
  }%
}
\titleformat{\section}[block]
  {\normalfont\sffamily}
  {}
  {0pt}
  {\pvzSectionBar\hspace{0.8em}%
   \begin{minipage}[t]{0.9\textwidth}
     \raggedright
     {\color{pvzMagenta}\scshape\bfseries\footnotesize\mdseries SECTION~\thesection}\\[-2pt]
     {\color{pvzInk}\LARGE\bfseries\sffamily #1}%
   \end{minipage}}
  [\vspace{2pt}\color{pvzMist}\titlerule[0.8pt]]
```

Subsections (`\subsection`) get a smaller treatment: a 2pt Sky bar on the left and the title in `pvzRoyal\bfseries\sffamily`. No eyebrow.

```latex
\titleformat{\subsection}[block]
  {\normalfont\sffamily\bfseries}
  {}
  {0pt}
  {\hspace{-0.25em}\textcolor{pvzSky}{\rule[-0.1em]{2pt}{1.05em}}\hspace{0.6em}%
   {\color{pvzRoyal}\normalsize\thesubsection\quad#1}}
  []
```

Subsubsections (rare in this doc) inherit titlesec's default but with color set to `pvzRoyal`.

**Why sans-serif for headings over serif body**: contrast. Sans headings against a serif body is a very common "designed-document" pattern (think product spec sheets) and makes the restyle land without touching body typography. `\sffamily` uses the same default as the rest of the document (Computer Modern Sans), so no new font package.

**Alternatives considered:**
- *Plain pill/badge (Option B from brainstorming)*: rejected — less editorial feel; the user picked sidebar during brainstorming.
- *Drop-caps on each section's first paragraph*: rejected — too ornate for a technical doc.
- *Colored section numbers only (minimal)*: rejected — user asked for bold restyle.

### Decision 5: Code listing theme (listings)

Replace the current two-color listings configuration with a palette-native theme that works for C, Verilog, TCL, and unstyled `language={}` blocks. The header band shows language and filename where given via `title={}`.

```latex
\lstdefinestyle{pvzListing}{
  basicstyle        = \ttfamily\small\color{pvzInk},
  keywordstyle      = \bfseries\color{pvzPlum},
  ndkeywordstyle    = \bfseries\color{pvzRoyal},     % types (C) / system tasks (Verilog)
  identifierstyle   = \color{pvzInk},
  stringstyle       = \color{pvzMagenta},
  commentstyle      = \itshape\color{black!48},       % slate gray, not ForestGreen
  numberstyle       = \color{pvzPlum},
  directivestyle    = \color{pvzPlum!80!black},       % #include, #define
  frame             = single,
  rulecolor         = \color{pvzMist!60!pvzSky},
  backgroundcolor   = \color{pvzMist!35},             % very soft Mist tint
  framesep          = 6pt,
  framerule         = 0.7pt,
  xleftmargin       = 2.6em,
  framexleftmargin  = 2.1em,
  numbers           = left,
  numbersep         = 10pt,
  breaklines        = true,
  captionpos        = t,
  abovecaptionskip  = 4pt,
  belowcaptionskip  = 6pt,
  showstringspaces  = false,
}
\lstset{style=pvzListing}
```

`listings` doesn't natively support `ndkeywordstyle`; for two-tier keyword highlighting, declare a custom language per target that partitions builtins into two `morekeywords={}` lists with explicit styles:

```latex
\lstdefinelanguage{pvzC}[]{C}{
  morekeywords=[2]{__u8,__u16,__u32,size_t,ssize_t,ioctl},
  keywordstyle=[2]\bfseries\color{pvzRoyal},
  morecomment=[l]{//},
}
\lstdefinelanguage{pvzVerilog}[]{Verilog}{
  morekeywords=[2]{logic,bit,integer,real,time,wire,reg},
  keywordstyle=[2]\bfseries\color{pvzRoyal},
}
```

All existing `\begin{lstlisting}[language=C]` and `[language=Verilog]` blocks switch to `[language=pvzC]` / `[language=pvzVerilog]`. Plain blocks (`language={}`) and TCL blocks keep their single keyword tier.

**Why a soft Mist tint for the background instead of pure white**: sets the code block apart without heavy borders; aligns with the palette; pairs naturally with the Royal header/rule.

**Alternatives considered:**
- *Pygments via `minted`*: rejected — requires `-shell-escape` and Python. Breaks "any standard TeX Live" promise.
- *GitHub-ish light theme (Option B in brainstorming)*: rejected at brainstorm step. Not palette-integrated.

### Decision 6: Tables

Booktabs rules in `pvzPlum`, thinner midrules, small-caps `pvzPlum` column headers. Soft Mist zebra striping optional on tables wider than ~4 columns (the register-map table in §Hardware/Software Interface and the shape-table allocation table).

```latex
\usepackage{colortbl}
\arrayrulecolor{pvzPlum}
\setlength{\heavyrulewidth}{1.4pt}
\setlength{\lightrulewidth}{0.6pt}

% Optional zebra striping, applied per-table:
\newcommand{\pvzZebra}{\rowcolors{2}{pvzMist!45}{white}}

% Column-header macro:
\newcommand{\pvzTH}[1]{{\color{pvzPlum}\scshape\bfseries #1}}
```

Usage pattern in the body for a striped table:

```latex
{\pvzZebra
\begin{tabular}{...}
\toprule
\pvzTH{module} & \pvzTH{role} & \pvzTH{notes} \\
\midrule
...
\bottomrule
\end{tabular}}
```

The `\arrayrulecolor` is global; `\rowcolors` is scoped to the local group introduced by the surrounding braces.

### Decision 7: Figure captions

```latex
\usepackage[labelfont={small,bf,sc,color=pvzPlum},
            textfont={small},
            labelsep=quad]{caption}
\captionsetup[figure]{skip=6pt}
\captionsetup[table]{skip=4pt}
```

The `sc` (small caps) + `color=pvzPlum` on `labelfont` turns "Figure 7:" into a plum small-caps label. Body of the caption stays in the default small serif.

### Decision 8: Figure 7 grass preservation

The screen-layout TikZ at `design-document.tex:1524-1588` uses `ForestGreen!25` and `ForestGreen!40` for the 4x8 gameplay grid checkerboard. Those fill commands are preserved verbatim — green there means grass, not a structural category.

**All other greens are structural, not grass**, and must swap to the new palette:

- `design-document.tex:142` — `extblock` style `fill=green!10` in `fig:system-block`: change to `fill=pvzMist` with border `draw=gray!40`.
- `design-document.tex:603` — `ext` style `fill=green!10` in `fig:scanline-dataflow`: same swap.
- `design-document.tex:654` — prose reference "green path" describing the linebuffer data flow: the linebuffer read-side in Decision 2's mapping uses `pvzSky`, so update the prose from "green path" to "sky-blue path" to match the restyled figure.
- `design-document.tex:1258` — `fill=green!10` on the `y[8:0]` field node in `fig:shape-bits`: change to `fill=pvzMist` with text `pvzInk` (and optionally a thin `pvzSky` border for emphasis).

### Decision 9: Hyperlinks

```latex
\hypersetup{
  colorlinks=true,
  linkcolor=pvzRoyal,
  urlcolor=pvzMagenta,
  citecolor=pvzPlum,
  filecolor=pvzRoyal,
}
```

Replaces the current `linkcolor=NavyBlue,urlcolor=RoyalBlue` setup.

### Decision 10: Title page

Current title: centered `\LARGE\bfseries` over a four-author `tabular`. Restyled:

```latex
\begin{center}
  {\color{pvzInk}\sffamily\bfseries\Huge Plants vs~Zombies on FPGA}\par
  \vspace{0.4em}
  \noindent\begin{tikzpicture}
    \fill[left color=pvzPlum, right color=pvzRoyal, middle color=pvzMist]
      (0,0) rectangle (\linewidth, 2.5pt);
  \end{tikzpicture}\par
  \vspace{0.6em}
  {\color{pvzRoyal}\sffamily\large CSEE 4840 Embedded System Design \textperiodcentered{} Spring 2026 \textperiodcentered{} Design Document}\par
  \vspace{1.6em}
  {\sffamily\small\color{pvzInk}
   \begin{tabular}{llll}
     \color{pvzMagenta}Hao Cai & \texttt{hc3612} &
     \color{pvzMagenta}Chenhao Yang & \texttt{cy2822} \\
     \color{pvzMagenta}Zhenghang Zhao & \texttt{zz3410} &
     \color{pvzMagenta}Yabkun Li & \texttt{yl6022} \\
   \end{tabular}}
\end{center}
```

Gradient rule uses the palette's natural Plum→Mist→Royal arc. The center Mist stop keeps the rule from feeling purely warm.

### Decision 11: Table of contents

```latex
\usepackage{tocloft}
\renewcommand{\cftsecfont}{\sffamily\bfseries\color{pvzInk}}
\renewcommand{\cftsecpagefont}{\sffamily\color{pvzRoyal}}
\renewcommand{\cftsecnumfont}{\sffamily\bfseries\color{pvzPlum}}
\renewcommand{\cftsubsecfont}{\sffamily\color{pvzInk}}
\renewcommand{\cftsubsecpagefont}{\sffamily\color{pvzRoyal}}
\renewcommand{\cftsubsecnumfont}{\sffamily\color{pvzMagenta}}
\renewcommand{\cftdotsep}{2.0}
\renewcommand{\cftsecleader}{\cftdotfill{\cftdotsep}}
```

Numbers in plum/magenta, titles in ink, page numbers in royal, soft dot leaders.

### Decision 12: Typography & spacing ("spacious and relaxed")

```latex
\usepackage{microtype}
\linespread{1.15}
\geometry{margin=1in, top=1.2in, bottom=1.2in}
% `parskip` package is already loaded; it sets parskip to a small skip
% and zeroes parindent. We override parskip slightly upward for a more
% "relaxed" paragraph rhythm:
\setlength{\parskip}{0.7em plus 0.15em minus 0.1em}
\setlength{\abovecaptionskip}{6pt}
\setlength{\belowcaptionskip}{2pt}
\setlength{\intextsep}{14pt}
```

Body font stays Computer Modern. Math stays Computer Modern Math. Sans (`\sffamily`) remains the default CMSS — consistent with what the existing listings and TikZ nodes already assume.

**Why not a modern font (e.g., Libertinus, EB Garamond)**: font swaps in LaTeX can break mathematical rendering, fail silently on different TeX Live installs, and expand build complexity. The restyle gets 90% of the "designed" feeling from color + heading treatment + microtype + line-spread without the risk.

## Integration notes

1. **Preamble ordering matters**: `xcolor` (already loaded) → color definitions → `tikz` library loads → `titlesec` → `caption` → `hyperref` (must be near-last among packages that define colors used in links).
2. **TikZ style redefinitions**: each figure's `\tikzset` block currently defines `block/hwblock/extblock`, etc. These stay local but should pull their colors from the new `pvz*` names rather than inline `blue!8`/`orange!12`/`green!10`.
3. **Per-figure audit checklist**: for each of the seven figures, walk all `fill=`, `draw=`, `color=`, `text=` attributes and ensure (a) they reference `pvz*` colors, (b) no white text sits on a light fill, (c) the role mapping (Decision 2) is honored. Figure 7 is the single exception.
4. **Prose edits**: the single "green path" reference at `design-document.tex:654` must be updated to match whatever color the linebuffer read path takes in the restyled figure (expected: "sky path" or "blue path").
5. **Build sanity**: the PDF must continue to build with `pdflatex` on a stock TeX Live (the tool used on Columbia workstations). Every new package listed (`titlesec`, `caption`, `tocloft`, `colortbl`, `microtype`) ships with TeX Live Basic.
6. **Compile-breaks-are-rollbackable**: the restyle is gated at the preamble level — if any package interaction breaks compilation, the preamble rewrite can be reverted without touching the body.

## Risks / Open questions

- **Magenta-on-dark contrast for small text** is borderline AA. Any small white labels on Magenta fills (e.g., the "Kernel Driver" block in `fig:system-block`) should be `\bfseries\small` at minimum. Flagged as a review-time check.
- **`titlesec` section-number rendering**: `titlesec` occasionally interferes with `hyperref` / `tocloft` on the `\section` number. If the TOC page-number alignment breaks, fall back to `titlesec`-free section restyle using raw `\Large\bfseries\color{pvzInk}` and a `\hrule` below. Flagged as fallback.
- **Caption package loaded after `hyperref`** can cause link-color drift on caption labels. The preamble ordering above (`caption` before `hyperref`) should prevent this, but worth compiling once and checking caption link colors.
- **Long tables** (register map, shape-index allocation) may need tuning once the new column-header macro is in — the tracked-uppercase small-caps in `pvzPlum` widens `\pvzTH{...}` header cells slightly.

## Acceptance criteria

The restyle is done when:

1. The preamble defines all `pvz*` colors and loads `microtype`, `titlesec`, `caption`, `tocloft`.
2. Every `fill=...`, `draw=...`, `color=...`, `text=...` in the seven TikZ figures references a `pvz*` color (with Figure 7's `ForestGreen` as the single allowed exception).
3. Every `\section` renders with the vertical plum bar, magenta eyebrow, ink title, and Mist rule.
4. Every `\subsection` renders with the Sky bar and royal number/title.
5. Every `\begin{lstlisting}` block uses the `pvzListing` style with language-appropriate highlighting (plum keywords, royal types, magenta strings, slate comments, Mist background).
6. All tables render with plum booktabs rules and `\pvzTH` headers; the two widest tables have Mist zebra striping.
7. All figure captions render with `Figure N:` in plum small caps.
8. `linkcolor=pvzRoyal`, `urlcolor=pvzMagenta`, `citecolor=pvzPlum`.
9. The title page, TOC, and "spacious/relaxed" spacing (microtype, `\linespread{1.15}`, 1.2in top/bottom margins) are applied.
10. No white text lands on `pvzSky`, `pvzMist`, or white fills.
11. `pdflatex design-document` completes cleanly on a stock TeX Live with no missing-package errors and no overfull-hbox warnings above 4pt.
12. Figure 7's gameplay-area checkerboard remains ForestGreen.
