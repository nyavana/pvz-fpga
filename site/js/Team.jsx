/* eslint-disable */

function Team() {
  return (
    <div>
      {/* HEADER */}
      <Section bg="var(--ink-900)" color="var(--ink-50)" top={84} bottom={72} scanlines>
        <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 56, alignItems: "end" }}>
          <div>
            <Eyebrow color="var(--trace-cyan)">// team · csee 4840 · embedded systems</Eyebrow>
            <PixelTitle size={44} color="#fff" style={{ marginBottom: 22 }}>
              Four humans, one chip.
            </PixelTitle>
            <Lede color="var(--silicon-200)" max="62ch">
              Built over the spring 2026 semester at Columbia for CSEE 4840
              (Embedded System Design). Roles weren't strict — everybody touched everything — but
              we each had a center of gravity.
            </Lede>
          </div>
          <div style={{ display: "flex", flexDirection: "column", gap: 10, alignItems: "flex-start" }}>
            <SpecBadge variant="trace">SEMESTER · SPRING 2026</SpecBadge>
            <SpecBadge variant="trace">COURSE · CSEE 4840</SpecBadge>
            <SpecBadge variant="trace">TEAM SIZE · 4</SpecBadge>
            <SpecBadge variant="trace">SCHOOL · COLUMBIA SEAS</SpecBadge>
          </div>
        </div>
      </Section>

      {/* MEMBERS */}
      <Section bg="#fff" top={64} bottom={96}>
        <div style={{ marginBottom: 36 }}>
          <Eyebrow>// roster</Eyebrow>
          <PixelTitle size={28}>Team members.</PixelTitle>
        </div>
        <div style={{ display: "grid", gridTemplateColumns: "repeat(2, 1fr)", gap: 18 }}>
          <MemberCard
            initials="CY" tone="lawn"
            name="Chenhao Yang"
            uni="cy2822@columbia.edu"
            role="Hardware — VGA timing &amp; line buffer"
            owned={["vga_counters.sv", "linebuffer.sv", "tb_vga_counters", "tb_linebuffer"]}
            quote="If the pixel clock is right, the rest is just bookkeeping."/>
          <MemberCard
            initials="HC" tone="sun"
            name="Hao Cai"
            uni="hc3612@columbia.edu"
            role="Hardware — shape renderer &amp; palette"
            owned={["shape_renderer.sv", "color_palette.sv", "shape_table.sv", "tb_shape_renderer"]}
            quote="Painter's algorithm is fine — just don't run out of cycles before hsync."/>
          <MemberCard
            initials="YL" tone="zombie"
            name="Yabkun Li"
            uni="yl6022@columbia.edu"
            role="Software — game logic &amp; tests"
            owned={["game.c", "input.c", "test_game", "12 unit tests"]}
            quote="If srand(42) doesn't produce the same output twice, you have a bigger problem."/>
          <MemberCard
            initials="ZZ" tone="trace"
            name="Zhenghang Zhao"
            uni="zz3410@columbia.edu"
            role="Software/glue — driver &amp; bring-up"
            owned={["pvz_driver.c", "render.c", "Makefile", "device tree"]}
            quote="The kernel module is small. The dance to load it correctly is not."/>
        </div>
      </Section>

      {/* SCHEDULE */}
      <Section bg="var(--ink-50)" top={96} bottom={96}>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.2fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow>// milestones</Eyebrow>
            <PixelTitle size={26} style={{ marginBottom: 18 }}>The semester.</PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              We sequenced the project to get something on the screen as early as possible.
              VGA timing and a single hardcoded square came first; everything else was layered
              on top of working pixels.
            </p>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              The biggest moment was M2 — the day all four of us watched a peashooter shoot
              a real pea at a real zombie, on a real CRT, with code we wrote.
            </p>
          </div>
          <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
            <Milestone tag="W3"  title="VGA timing"        body="vga_counters.sv synthesizes; a hardcoded checkerboard appears."/>
            <Milestone tag="W5"  title="Line buffer + palette" body="Two-bank BRAM linebuffer + 8→24 bit color lookup. Flat color test patterns stable."/>
            <Milestone tag="W7"  title="Avalon agent"      body="pvz_top.sv exposed as memory-mapped peripheral; first ioctl from userspace draws a rect."/>
            <Milestone tag="W9"  title="Shape renderer"    body="rect + circle types; painter's algorithm; 48-entry shape table working."/>
            <Milestone tag="W10" title="Game loop"         body="game.c drives the cursor; place / remove plants; sun economy ticks."/>
            <Milestone tag="W12" title="MVP wave"          body="Five zombies spawn over 30s; peas collide; WIN/LOSE detected."/>
            <Milestone tag="W13" title="Sprite ROM"        body="32×32 ROM scaled 2× → 64×64; transparency via reserved palette index."/>
            <Milestone tag="W14" title="Final report"      body="28 pages; demo video; everything lints; tests are green."/>
          </div>
        </div>
      </Section>

      {/* THANKS */}
      <Section bg="var(--silicon-900)" color="var(--ink-50)" top={96} bottom={96} scanlines>
        <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 56 }}>
          <div>
            <Eyebrow color="var(--trace-cyan)">// acknowledgements</Eyebrow>
            <PixelTitle size={28} color="#fff" style={{ marginBottom: 18 }}>Thanks to.</PixelTitle>
            <ul style={{ listStyle: "none", padding: 0, margin: 0, fontFamily: "var(--font-mono)", fontSize: 14, color: "var(--silicon-100)", lineHeight: 1.9 }}>
              <li>— <span style={{color:"var(--trace-cyan)"}}>CSEE 4840 staff</span> · lectures, lab kit, and bring-up help</li>
              <li>— <span style={{color:"var(--trace-cyan)"}}>Terasic / Intel</span> · DE1-SoC reference design and Cyclone V SE</li>
              <li>— <span style={{color:"var(--trace-cyan)"}}>PopCap Games</span> · for inspiring everyone in 2009</li>
            </ul>
          </div>
          <div style={{
            background: "#0a1018", border: "2px solid var(--silicon-600)", padding: 24,
          }}>
            <div style={{ fontFamily: "var(--font-press)", fontSize: 11, color: "var(--trace-mag)", marginBottom: 14, letterSpacing: "0.08em" }}>// REPO</div>
            <CodeBlock dark>{`$ git clone https://github.com/csee4840/pvz-fpga
$ cd pvz-fpga
$ tree -L 1
.
├── hw/         # SystemVerilog + Quartus + Qsys
├── sw/         # C, kernel module, Makefile
├── docs/       # report, milestones, BOM
└── README.md`}</CodeBlock>
            <div style={{ marginTop: 16, fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--silicon-400)" }}>
              MIT license · plants vs. zombies belongs to its rightful owners; this is a coursework reimplementation.
            </div>
          </div>
        </div>
      </Section>
    </div>
  );
}

/* ───────── member card ───────── */
function MemberCard({ initials, tone, name, uni, role, owned, quote }) {
  const tones = {
    lawn:   "var(--lawn-600)",
    sun:    "var(--sun-500)",
    zombie: "var(--zombie-500)",
    trace:  "var(--trace-cyan)",
  };
  const fg = tone === "sun" || tone === "trace" ? "var(--ink-900)" : "#fff";
  return (
    <div style={{
      background: "#fff", border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      display: "grid", gridTemplateColumns: "120px 1fr",
    }}>
      <div style={{
        background: tones[tone], color: fg,
        display: "flex", alignItems: "center", justifyContent: "center",
        fontFamily: "var(--font-press)", fontSize: 32,
        borderRight: "2px solid var(--ink-900)",
        letterSpacing: "0.05em",
      }}>{initials}</div>
      <div style={{ padding: "18px 20px" }}>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline", marginBottom: 4 }}>
          <div style={{ fontWeight: 700, fontSize: 18, color: "var(--ink-900)" }}>{name}</div>
          <code style={{ fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--zombie-700)" }}>{uni}</code>
        </div>
        <div style={{ fontSize: 13, color: "var(--ink-600)", marginBottom: 12 }} dangerouslySetInnerHTML={{__html: role}}/>
        <div style={{ display: "flex", gap: 6, flexWrap: "wrap", marginBottom: 12 }}>
          {owned.map(o => (
            <span key={o} style={{
              fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--ink-700)",
              background: "var(--ink-50)", border: "1px solid var(--ink-300)",
              padding: "2px 6px",
            }}>{o}</span>
          ))}
        </div>
        <div style={{
          fontSize: 13, fontStyle: "italic", color: "var(--ink-700)",
          borderLeft: "3px solid " + tones[tone], paddingLeft: 12,
        }}>"{quote}"</div>
      </div>
    </div>
  );
}

/* ───────── milestone row ───────── */
function Milestone({ tag, title, body }) {
  return (
    <div style={{
      display: "grid", gridTemplateColumns: "60px 1fr",
      background: "#fff", border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-1)",
    }}>
      <div style={{
        background: "var(--silicon-900)", color: "var(--trace-cyan)",
        display: "flex", alignItems: "center", justifyContent: "center",
        fontFamily: "var(--font-press)", fontSize: 11,
        borderRight: "2px solid var(--ink-900)",
      }}>{tag}</div>
      <div style={{ padding: "12px 16px" }}>
        <div style={{ fontWeight: 700, fontSize: 14, color: "var(--ink-900)" }}>{title}</div>
        <div style={{ fontSize: 13, color: "var(--ink-600)", marginTop: 2, lineHeight: 1.5 }}>{body}</div>
      </div>
    </div>
  );
}

window.Team = Team;
