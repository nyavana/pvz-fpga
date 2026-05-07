/* eslint-disable */

function Hardware() {
  return (
    <div>
      {/* HEADER */}
      <Section bg="var(--silicon-900)" color="var(--ink-50)" top={84} bottom={72} scanlines>
        <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 56, alignItems: "end" }}>
          <div>
            <Eyebrow color="var(--trace-cyan)">// hardware · systemverilog · cyclone v</Eyebrow>
            <PixelTitle size={44} color="#fff" style={{ marginBottom: 22 }}>
              The display pipeline.
            </PixelTitle>
            <Lede color="var(--silicon-200)" max="62ch">
              Five SystemVerilog modules turn a small table of shape descriptors into 640×480 pixels at 60 Hz.
              No frame buffer, no DDR — every pixel is computed at the pixel clock and held in a 640-byte line buffer.
              The scanline rasterizer is a five-state FSM walking 48 shapes per line.
            </Lede>
          </div>
          <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
            <SpecBadge variant="trace">PIXEL CLOCK · 25 MHz</SpecBadge>
            <SpecBadge variant="trace">HSYNC · 1600 cycles</SpecBadge>
            <SpecBadge variant="trace">VSYNC · 525 lines</SpecBadge>
            <SpecBadge variant="trace">PALETTE · 8 → 24 bit</SpecBadge>
            <SpecBadge variant="trace">SHAPES · 48 entries</SpecBadge>
          </div>
        </div>
      </Section>

      {/* PIPELINE DIAGRAM */}
      <Section bg="var(--silicon-900)" color="var(--ink-50)" top={0} bottom={96} scanlines border={false}>
        <div style={{ marginBottom: 28 }}>
          <Eyebrow color="var(--trace-mag)">// dataflow</Eyebrow>
          <PixelTitle size={26} color="#fff">From shape table to VGA pin.</PixelTitle>
        </div>
        <PipelineDiagram/>
      </Section>

      {/* PALETTE TABLE */}
      <Section bg="#fff" top={96} bottom={96}>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.4fr", gap: 64, alignItems: "start" }}>
          <div>
            <Eyebrow>// color_palette.sv</Eyebrow>
            <PixelTitle size={28} style={{ marginBottom: 18 }}>
              Thirteen colors.<br/>
              <span style={{color:"var(--lawn-600)"}}>Two hundred forty-three blanks.</span>
            </PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              Every pixel in the line buffer is an 8-bit index. At display time, the palette
              module turns it into 24-bit RGB with a single combinational <Code>case</Code>.
              Indices 13–255 are reserved for future plant types and zombie variants.
            </p>
            <p style={{ fontSize: 14, lineHeight: 1.7, color: "var(--ink-500)", fontFamily: "var(--font-mono)" }}>
              // hw/color_palette.sv : line 28<br/>
              // case (index) 8'd0 → 8'd12 ... default: black
            </p>
          </div>
          <PaletteGrid/>
        </div>
      </Section>

      {/* MODULES */}
      <Section bg="var(--ink-50)" top={96} bottom={96}>
        <div style={{ marginBottom: 36 }}>
          <Eyebrow>// hw/ — five files do all the work</Eyebrow>
          <PixelTitle size={28}>The modules.</PixelTitle>
        </div>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 16 }}>
          {[
            { name: "vga_counters.sv",   loc: "59 lines",  desc: "640×480@60 timing. 50 MHz in → 25 MHz pixel clock. hcount=0..1599, vcount=0..524, hsync, vsync, blank." },
            { name: "linebuffer.sv",     loc: "48 lines",  desc: "Two 640-byte BRAM banks. One displays the current scanline, the other is being drawn. Roles swap at hsync." },
            { name: "color_palette.sv",  loc: "55 lines",  desc: "Combinational 8→24 bit lookup. Hardcoded 13 entries from the MVP palette; rest default to black." },
            { name: "shape_table.sv",    loc: "117 lines", desc: "48 entries × {type, visible, x, y, w, h, color}. Shadow / active register file with vsync-edge latch." },
            { name: "shape_renderer.sv", loc: "262 lines", desc: "The big one. 5-state FSM rasterizes rect, circle, 7-seg digit, and 32×32 sprite per scanline. Painter's order." },
            { name: "pvz_top.sv",        loc: "238 lines", desc: "Avalon-MM agent. Wires the shape table, renderer, line buffer, palette, and VGA timing into one peripheral." },
          ].map(m => <ModuleCard key={m.name} {...m}/>)}
        </div>
      </Section>

      {/* SCANLINE ANATOMY */}
      <Section bg="#fff" top={96} bottom={96}>
        <div style={{ display: "grid", gridTemplateColumns: "1.1fr 1fr", gap: 56, alignItems: "center" }}>
          <div>
            <Eyebrow>// shape_renderer.sv — anatomy of one scanline</Eyebrow>
            <PixelTitle size={26} style={{ marginBottom: 18 }}>
              One scanline, ~1600 pixel clocks.
            </PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              The rasterizer is a 5-state FSM. It fills the line with the current cell's
              background color, then walks all 48 shapes — checking, drawing, advancing.
              Later shapes overwrite earlier ones (painter's algorithm). The whole thing
              must finish before the line buffer swap at the next hsync.
            </p>
            <ul style={{ listStyle: "none", padding: 0, margin: "20px 0 0", display: "flex", flexDirection: "column", gap: 10 }}>
              <FSMRow code="S_IDLE"        body="Wait for render_start pulse from VGA timing."/>
              <FSMRow code="S_BG_FILL"     body="640 cycles. Walk x = 0..639, write bg_grid color into the line buffer draw port."/>
              <FSMRow code="S_SHAPE_SETUP" body="Latch the 6 fields of shape[i] into local registers."/>
              <FSMRow code="S_SHAPE_CHECK" body="Skip if not visible or scanline outside [s_y, s_y+s_h]. Otherwise enter SHAPE_DRAW."/>
              <FSMRow code="S_SHAPE_DRAW"  body="For each x in [s_x, s_x+s_w): test pixel_hit by type (rect / circle / 7-seg / sprite). Write s_color if hit."/>
              <FSMRow code="S_DONE"        body="Hand off to line buffer swap. Wait for next render_start."/>
            </ul>
          </div>
          <ShapeTypesPanel/>
        </div>
      </Section>

      {/* AVALON REGISTERS */}
      <Section bg="var(--silicon-900)" color="var(--ink-50)" top={96} bottom={96} scanlines>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.4fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow color="var(--trace-cyan)">// avalon-mm · 0xff200000</Eyebrow>
            <PixelTitle size={28} color="#fff" style={{ marginBottom: 18 }}>
              Five 32-bit registers.
            </PixelTitle>
            <Lede color="var(--silicon-200)" style={{ marginBottom: 20 }}>
              Memory-mapped on the lightweight HPS-to-FPGA bridge. The kernel module exposes them as <Code dark>/dev/pvz</Code>.
              Three ioctls cover the entire interface: write a background cell, write a shape entry, commit.
            </Lede>
            <SpecBadge variant="trace">BASE · 0xff200000</SpecBadge>
          </div>
          <RegisterTable/>
        </div>
      </Section>

      {/* TIMING + TESTBENCHES */}
      <Section bg="var(--ink-50)" top={96} bottom={96} border={false}>
        <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow>// quartus build · timing</Eyebrow>
            <PixelTitle size={26} style={{ marginBottom: 18 }}>
              Synthesis &amp; testbenches.
            </PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)", marginBottom: 16 }}>
              Quartus Prime targets the Cyclone V SE on the DE1-SoC. Five self-checking ModelSim
              testbenches cover the pipeline end-to-end with a 200 ms watchdog on the integration test.
            </p>
            <CodeBlock lang="bash">{`# hw/Makefile targets
$ make qsys      # qsys-generate → soc_system/
$ make quartus   # full synthesis → soc_system.sof
$ make rbf       # .sof → SD-card .rbf
$ make dtb       # sopc2dts → device tree blob

# ModelSim — five self-checking benches
$ vsim -do "run -all; quit" work.tb_vga_counters
$ vsim -do "run -all; quit" work.tb_linebuffer
$ vsim -do "run -all; quit" work.tb_bg_grid
$ vsim -do "run -all; quit" work.tb_shape_renderer
$ vsim -do "run -all; quit" work.tb_pvz_top`}</CodeBlock>
          </div>
          <div style={{ display: "flex", flexDirection: "column", gap: 14 }}>
            <TBRow tb="tb_vga_counters"   note="hcount cycles 0–1599 · vcount 0–524 · sync pulses · blanking"/>
            <TBRow tb="tb_linebuffer"     note="dual-port swap at hsync · boundary pixels · 1-cycle read latency"/>
            <TBRow tb="tb_bg_grid"        note="cell color lookup · shadow → vsync latch · out-of-area = 0"/>
            <TBRow tb="tb_shape_renderer" note="rect pixels · z-order · invisible skipped · scanline write count"/>
            <TBRow tb="tb_pvz_top"        note="full pipeline · Avalon writes → VGA RGB · 200ms watchdog"/>
          </div>
        </div>
      </Section>
    </div>
  );
}

/* ───────── pipeline diagram ───────── */
function PipelineDiagram() {
  return (
    <div style={{
      background: "#0e141c",
      border: "2px solid var(--silicon-600)",
      padding: 28,
      display: "grid", gridTemplateColumns: "1fr auto 1fr auto 1fr", gap: 0,
      alignItems: "stretch",
    }}>
      <PipeBox color="var(--trace-cyan)" name="HPS · Linux"
        lines={[
          "main.c — 60 Hz tick",
          "game.c — state",
          "render.c → ioctl",
          "pvz_driver.ko",
        ]}/>
      <Arrow label="ioctl" sub="0xff200000" color="var(--trace-cyan)"/>
      <PipeBox color="var(--sun-500)" name="shape_table.sv"
        lines={[
          "48 × {type, vis, x, y,",
          "      w, h, color}",
          "shadow → active",
          "latch on vsync edge",
        ]}/>
      <Arrow label="per scanline" sub="render_start" color="var(--sun-500)"/>
      <PipeBox color="var(--trace-mag)" name="shape_renderer.sv"
        lines={[
          "S_BG_FILL → 640 px",
          "× 48 shapes",
          "→ linebuffer write",
          "FSM @ 25 MHz",
        ]}/>
      <div style={{ gridColumn: "1 / -1", marginTop: 28, display: "grid", gridTemplateColumns: "1fr auto 1fr auto 1fr" }}>
        <PipeBox color="var(--zombie-500)" name="linebuffer.sv"
          lines={[
            "2 × 640-byte BRAM",
            "draw side / display",
            "swap at hsync",
            "8-bit palette index",
          ]}/>
        <Arrow label="index" sub="8 bits" color="var(--zombie-500)"/>
        <PipeBox color="var(--lawn-500)" name="color_palette.sv"
          lines={[
            "case (index)",
            "13 RGB entries",
            "default: black",
            "combinational",
          ]}/>
        <Arrow label="rgb" sub="24 bits" color="var(--lawn-500)"/>
        <PipeBox color="var(--ink-200)" name="VGA_R/G/B"
          lines={[
            "640×480 @ 60 Hz",
            "VGA_HS, VGA_VS",
            "VGA_BLANK_n",
            "→ DE1-SoC DAC",
          ]} dark/>
      </div>
    </div>
  );
}

function PipeBox({ color, name, lines, dark }) {
  return (
    <div style={{
      background: dark ? "#fff" : "#0a1018",
      border: "2px solid " + color,
      padding: "16px 18px",
    }}>
      <div style={{
        fontFamily: "var(--font-press)", fontSize: 10, letterSpacing: "0.08em",
        color: dark ? "var(--ink-900)" : color, marginBottom: 12,
      }}>{name.toUpperCase()}</div>
      <ul style={{ listStyle: "none", margin: 0, padding: 0 }}>
        {lines.map((l,i) => (
          <li key={i} style={{
            fontFamily: "var(--font-mono)", fontSize: 12,
            color: dark ? "var(--ink-700)" : "var(--silicon-100)",
            padding: "3px 0",
            borderBottom: i < lines.length - 1 ? "1px solid rgba(255,255,255,0.06)" : "none",
          }}>{l}</li>
        ))}
      </ul>
    </div>
  );
}

function Arrow({ label, sub, color }) {
  return (
    <div style={{
      display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center",
      padding: "0 10px", minWidth: 70,
    }}>
      <div style={{ fontFamily: "var(--font-mono)", fontSize: 10, letterSpacing: "0.08em", color, marginBottom: 4 }}>{label}</div>
      <div style={{ color, fontSize: 22, lineHeight: 1 }}>→</div>
      <div style={{ fontFamily: "var(--font-mono)", fontSize: 10, color: "var(--silicon-400)", marginTop: 4 }}>{sub}</div>
    </div>
  );
}

/* ───────── palette ───────── */
function PaletteGrid() {
  const palette = [
    [ 0, "#000000", "BLACK",        "background / unused"],
    [ 1, "#1B5E20", "DARK GREEN",   "grid cell — dark"],
    [ 2, "#2D8B2D", "LIGHT GREEN",  "grid cell — light"],
    [ 3, "#8B4513", "BROWN",        "soil / stem"],
    [ 4, "#FFD700", "YELLOW",       "cursor highlight"],
    [ 5, "#FF0000", "RED",          "zombie body"],
    [ 6, "#8B0000", "DARK RED",     "zombie head"],
    [ 7, "#008000", "GREEN",        "peashooter body"],
    [ 8, "#006400", "DARK GREEN 2", "peashooter stem"],
    [ 9, "#00FF00", "BRIGHT GREEN", "pea projectile"],
    [10, "#FFFFFF", "WHITE",        "HUD digits"],
    [11, "#808080", "GRAY",         "HUD background"],
    [12, "#FFA500", "ORANGE",       "sun indicator"],
  ];
  return (
    <div style={{
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      background: "#fff",
    }}>
      <div style={{
        background: "var(--silicon-900)", color: "var(--trace-cyan)",
        padding: "10px 14px", fontFamily: "var(--font-mono)", fontSize: 12,
        display: "flex", justifyContent: "space-between",
        borderBottom: "2px solid var(--ink-900)",
      }}>
        <span>// hw/color_palette.sv</span>
        <span style={{color:"var(--trace-mag)"}}>13 / 256 used</span>
      </div>
      <div style={{ display: "grid", gridTemplateColumns: "auto auto 1fr auto", gap: 0 }}>
        {palette.map(([idx, hex, name, use]) => (
          <React.Fragment key={idx}>
            <div style={{
              fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--zombie-700)",
              padding: "10px 14px", borderBottom: "1px solid var(--ink-200)",
              background: idx % 2 ? "var(--ink-50)" : "#fff",
            }}>{`pal[${String(idx).padStart(2,"0")}]`}</div>
            <div style={{
              padding: "8px 8px", borderBottom: "1px solid var(--ink-200)",
              background: idx % 2 ? "var(--ink-50)" : "#fff",
              display: "flex", alignItems: "center",
            }}>
              <div style={{ width: 28, height: 22, background: hex, border: "2px solid var(--ink-900)" }}/>
            </div>
            <div style={{
              fontFamily: "var(--font-press)", fontSize: 10, letterSpacing: "0.08em",
              padding: "10px 6px", borderBottom: "1px solid var(--ink-200)",
              background: idx % 2 ? "var(--ink-50)" : "#fff",
              color: "var(--ink-900)",
            }}>{name}</div>
            <div style={{
              fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--ink-500)",
              padding: "10px 14px", borderBottom: "1px solid var(--ink-200)",
              background: idx % 2 ? "var(--ink-50)" : "#fff",
            }}>// {use}</div>
          </React.Fragment>
        ))}
      </div>
    </div>
  );
}

/* ───────── module cards ───────── */
function ModuleCard({ name, loc, desc }) {
  return (
    <div style={{
      background: "#fff",
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-1)",
      padding: 0,
    }}>
      <div style={{
        background: "var(--silicon-900)", color: "var(--trace-cyan)",
        padding: "10px 14px", fontFamily: "var(--font-mono)", fontSize: 13,
        borderBottom: "2px solid var(--ink-900)",
        display: "flex", justifyContent: "space-between",
      }}>
        <span>{name}</span>
        <span style={{color:"var(--silicon-400)", fontSize: 11}}>{loc}</span>
      </div>
      <div style={{ padding: "16px 18px", fontSize: 14, color: "var(--ink-700)", lineHeight: 1.6 }}>
        {desc}
      </div>
    </div>
  );
}

/* ───────── FSM rows ───────── */
function FSMRow({ code, body }) {
  return (
    <li style={{
      display: "grid", gridTemplateColumns: "auto 1fr", gap: 14, alignItems: "start",
      padding: "10px 14px",
      background: "#fff",
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-1)",
    }}>
      <span style={{
        fontFamily: "var(--font-mono)", fontSize: 12,
        color: "var(--zombie-700)",
        background: "var(--silicon-50)",
        padding: "4px 8px", border: "1px solid var(--silicon-100)",
        whiteSpace: "nowrap",
      }}>{code}</span>
      <span style={{ fontSize: 13, color: "var(--ink-700)", lineHeight: 1.55 }}>{body}</span>
    </li>
  );
}

/* ───────── shape types panel ───────── */
function ShapeTypesPanel() {
  return (
    <div style={{
      background: "var(--silicon-900)",
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      padding: 24,
    }}>
      <div style={{
        fontFamily: "var(--font-press)", fontSize: 11, letterSpacing: "0.08em",
        color: "var(--trace-cyan)", marginBottom: 18,
      }}>// SHAPE TYPES — type[1:0]</div>
      <div style={{ display: "grid", gridTemplateColumns: "auto 1fr auto", rowGap: 16, columnGap: 14, alignItems: "center" }}>
        <ShapePreview kind="rect"/>
        <div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--silicon-100)", fontWeight: 600 }}>0 · RECT</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--silicon-400)" }}>filled rectangle, w × h, color</div>
        </div>
        <SpecBadge variant="trace">always hit</SpecBadge>

        <ShapePreview kind="circle"/>
        <div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--silicon-100)", fontWeight: 600 }}>1 · CIRCLE</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--silicon-400)" }}>distance² ≤ (w/2)²</div>
        </div>
        <SpecBadge variant="trace">peas</SpecBadge>

        <ShapePreview kind="digit"/>
        <div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--silicon-100)", fontWeight: 600 }}>2 · 7-SEG DIGIT</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--silicon-400)" }}>20 × 30 px, val in w[3:0]</div>
        </div>
        <SpecBadge variant="trace">HUD sun</SpecBadge>

        <ShapePreview kind="sprite"/>
        <div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--silicon-100)", fontWeight: 600 }}>3 · SPRITE</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--silicon-400)" }}>32×32 ROM @ 2× → 64×64; pal[0xFF] = transparent</div>
        </div>
        <SpecBadge variant="trace">M2</SpecBadge>
      </div>
    </div>
  );
}

function ShapePreview({ kind }) {
  const box = { width: 48, height: 36, border: "2px solid var(--silicon-600)", background: "#000", position: "relative" };
  if (kind === "rect") return <div style={{...box, background: "var(--lawn-500)"}}/>;
  if (kind === "circle") return (
    <div style={box}>
      <div style={{ position: "absolute", inset: 6, borderRadius: "50%", background: "var(--lawn-500)" }}/>
    </div>
  );
  if (kind === "digit") return (
    <div style={{...box, display:"flex",alignItems:"center",justifyContent:"center"}}>
      <span style={{ fontFamily: "var(--font-term)", fontSize: 30, color: "var(--sun-500)", lineHeight: 1 }}>5</span>
    </div>
  );
  return (
    <div style={box}>
      <img src="assets/peashooter.png" width={36} style={{
        position: "absolute", left: 6, top: 0, imageRendering: "pixelated",
      }}/>
    </div>
  );
}

/* ───────── register table ───────── */
function RegisterTable() {
  const rows = [
    ["0x00", "BG_CELL",      "background grid cell color (row, col, color)"],
    ["0x04", "SHAPE_ADDR",   "shape table entry select (0–47)"],
    ["0x08", "SHAPE_DATA0",  "type[1:0] · visible · x[9:0] · y[8:0]"],
    ["0x0C", "SHAPE_DATA1",  "w[8:0] · h[8:0] · color[7:0]"],
    ["0x10", "SHAPE_COMMIT", "latch shadow → active at next vsync"],
  ];
  return (
    <div style={{ border: "2px solid var(--ink-900)", boxShadow: "var(--shadow-pixel-2)", background: "#fff" }}>
      <div style={{
        background: "var(--silicon-900)", color: "var(--trace-cyan)",
        padding: "10px 14px", fontFamily: "var(--font-mono)", fontSize: 12,
        letterSpacing: "0.08em", borderBottom: "2px solid var(--ink-900)",
        display: "flex", justifyContent: "space-between",
      }}>
        <span>// pvz.h — Avalon register byte offsets</span>
        <span style={{ color: "var(--trace-mag)" }}>0xff200000</span>
      </div>
      <table style={{ width: "100%", borderCollapse: "collapse", fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--ink-900)" }}>
        <thead>
          <tr style={{ background: "var(--ink-100)" }}>
            <th style={{ textAlign: "left", padding: "10px 14px", borderBottom: "1px solid var(--ink-300)" }}>OFFSET</th>
            <th style={{ textAlign: "left", padding: "10px 14px", borderBottom: "1px solid var(--ink-300)" }}>NAME</th>
            <th style={{ textAlign: "left", padding: "10px 14px", borderBottom: "1px solid var(--ink-300)" }}>DESCRIPTION</th>
          </tr>
        </thead>
        <tbody>
          {rows.map((r,i) => (
            <tr key={r[0]} style={{ background: i % 2 ? "var(--ink-50)" : "#fff" }}>
              <td style={{ padding: "10px 14px", color: "var(--zombie-700)", borderBottom: "1px solid var(--ink-200)" }}>{r[0]}</td>
              <td style={{ padding: "10px 14px", borderBottom: "1px solid var(--ink-200)" }}>{r[1]}</td>
              <td style={{ padding: "10px 14px", color: "var(--ink-600)", borderBottom: "1px solid var(--ink-200)" }}>{r[2]}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

function TBRow({ tb, note }) {
  return (
    <div style={{
      background: "#fff", border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-1)", padding: "12px 16px",
    }}>
      <div style={{ fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--zombie-700)", fontWeight: 600 }}>{tb}</div>
      <div style={{ fontSize: 12, color: "var(--ink-600)", marginTop: 4, lineHeight: 1.5 }}>{note}</div>
    </div>
  );
}

window.Hardware = Hardware;
