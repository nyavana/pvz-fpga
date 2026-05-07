/* eslint-disable */

/* Live animated mini-lawn for the hero — same idea as the kit but bigger,
   with a few more entities and a softer animation cadence. */
function MiniLawn({ scale = 1 }) {
  const [tick, setTick] = useState(0);
  useEffect(() => {
    const i = setInterval(() => setTick(t => t + 1), 90);
    return () => clearInterval(i);
  }, []);
  const cols = 8, rows = 4, cell = Math.round(40 * scale);
  const W = cols * cell, H = rows * cell;

  // zombie 1 marches in row 1; zombie 2 a bit later in row 2
  const z1 = W - ((tick * 2) % (W + 120));
  const z2 = W + 60 - ((tick * 2 + 80) % (W + 200));
  // peas — periodic, two rows
  const p1 = ((tick * 7) % (W + 30)) - 16;
  const p2 = ((tick * 7 + 90) % (W + 30)) - 16;
  const p3 = ((tick * 7 + 40) % (W + 30)) - 16;

  return (
    <div style={{
      width: W, height: H,
      position: "relative",
      border: "3px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-3)",
      backgroundColor: "var(--lawn-500)",
      backgroundImage:
        "linear-gradient(45deg, var(--lawn-600) 25%, transparent 25%, transparent 75%, var(--lawn-600) 75%)," +
        "linear-gradient(45deg, var(--lawn-600) 25%, transparent 25%, transparent 75%, var(--lawn-600) 75%)",
      backgroundSize: `${cell*2}px ${cell*2}px`,
      backgroundPosition: `0 0, ${cell}px ${cell}px`,
      overflow: "hidden",
      imageRendering: "pixelated",
    }}>
      {/* peashooters: r1c0, r2c1, r3c0 */}
      <img src="assets/peashooter.png" width={cell-8} style={{ position: "absolute", left: 4, top: 1*cell+4, imageRendering: "pixelated" }}/>
      <img src="assets/peashooter.png" width={cell-8} style={{ position: "absolute", left: cell+4, top: 2*cell+4, imageRendering: "pixelated" }}/>
      <img src="assets/peashooter.png" width={cell-8} style={{ position: "absolute", left: 4, top: 3*cell+4, imageRendering: "pixelated" }}/>

      {/* peas */}
      <img src="assets/pea.png" width={Math.round(14*scale)} style={{ position: "absolute", left: p1, top: 1*cell + cell/2 - 7, imageRendering: "pixelated" }}/>
      <img src="assets/pea.png" width={Math.round(14*scale)} style={{ position: "absolute", left: p2 + cell, top: 2*cell + cell/2 - 7, imageRendering: "pixelated" }}/>
      <img src="assets/pea.png" width={Math.round(14*scale)} style={{ position: "absolute", left: p3, top: 3*cell + cell/2 - 7, imageRendering: "pixelated" }}/>

      {/* zombies */}
      <img src="assets/zombie.png" width={cell-12} style={{
        position: "absolute", left: z1, top: 1*cell - 2,
        imageRendering: "pixelated",
        transform: tick%4<2 ? "translateY(0)" : "translateY(-2px)",
      }}/>
      <img src="assets/zombie.png" width={cell-12} style={{
        position: "absolute", left: z2, top: 2*cell - 2,
        imageRendering: "pixelated",
        transform: tick%4<2 ? "translateY(-2px)" : "translateY(0)",
      }}/>

      {/* cursor */}
      <div style={{
        position: "absolute",
        left: ((tick/14|0)%cols)*cell + 2, top: 0*cell + 2,
        width: cell-4, height: cell-4,
        border: "3px solid var(--sun-500)",
        boxSizing: "border-box",
        opacity: tick%6<3 ? 1 : 0.4,
      }}/>

      {/* HUD: sun counter */}
      <div style={{
        position: "absolute", top: 6, left: 6,
        display: "flex", alignItems: "center", gap: 6,
        background: "rgba(0,0,0,0.7)", padding: "3px 8px",
        border: "2px solid var(--ink-900)",
      }}>
        <img src="assets/sun.png" width={Math.round(16*scale)} style={{imageRendering:"pixelated"}}/>
        <span style={{
          fontFamily: "var(--font-term)", color: "var(--sun-300)",
          fontSize: Math.round(20*scale), lineHeight: 1,
        }}>{String(125 + tick).padStart(4,"0")}</span>
      </div>
      {/* corner badge */}
      <div style={{
        position: "absolute", top: 6, right: 6,
        background: "var(--zombie-500)", color: "#fff",
        fontFamily: "var(--font-press)", fontSize: 9, letterSpacing: "0.08em",
        padding: "4px 7px", border: "2px solid var(--ink-900)",
      }}>WAVE 1</div>
    </div>
  );
}

/* ─────────────────────────────────────── Home page ─────── */

function Home({ setTab }) {
  return (
    <div>
      {/* HERO */}
      <Section bg="var(--ink-50)" top={84} bottom={84}>
        <div style={{
          display: "grid", gridTemplateColumns: "1.05fr 1fr", gap: 56, alignItems: "center",
        }}>
          <div>
            <Eyebrow>// CSEE 4840 · Spring 2026 · Milestone 1 shipped</Eyebrow>
            <PixelTitle size={52} style={{ marginBottom: 24 }}>
              Plants vs Zombies, <span style={{color:"var(--lawn-600)"}}>on a chip.</span>
            </PixelTitle>
            <Lede style={{ marginBottom: 28 }}>
              We built a Plants vs Zombies clone that runs on a Cyclone V FPGA — a 640×480 lawn,
              rasterized one scanline at a time, with five very slow zombies and one very calm Peashooter.
              The ARM CPU on the same board runs the game in C; the FPGA does the pixels.
            </Lede>
            <div style={{ display: "flex", gap: 12, flexWrap: "wrap", marginBottom: 28 }}>
              <PixelButton onClick={() => { setTab("demo"); window.scrollTo(0,0); }}>PLAY THE DEMO →</PixelButton>
              <PixelButton variant="ghost" onClick={() => { setTab("hardware"); window.scrollTo(0,0); }}>HOW IT WORKS</PixelButton>
              <PixelButton variant="night" as="a" href="https://github.com/nyavana/pvz-fpga" target="_blank">GITHUB ↗</PixelButton>
            </div>
            <div style={{ display: "flex", gap: 8, flexWrap: "wrap" }}>
              <SpecBadge variant="night">60 HZ</SpecBadge>
              <SpecBadge variant="night">640×480 VGA</SpecBadge>
              <SpecBadge variant="night">8-BIT INDEX</SpecBadge>
              <SpecBadge variant="sun">DE1-SoC</SpecBadge>
              <SpecBadge variant="fpga">CYCLONE V</SpecBadge>
            </div>
          </div>
          <div style={{ display: "flex", justifyContent: "center" }}>
            <div style={{ position: "relative" }}>
              <MiniLawn scale={1.2}/>
              <div style={{
                position: "absolute", left: -24, top: -24,
                fontFamily: "var(--font-mono)", fontSize: 11,
                color: "var(--ink-500)", letterSpacing: "0.06em",
              }}>↳ live preview · 60 fps in browser</div>
              <div style={{
                position: "absolute", right: -8, bottom: -32,
                fontFamily: "var(--font-mono)", fontSize: 11,
                color: "var(--ink-500)", letterSpacing: "0.06em",
              }}>// pal[7] = #008000 // pal[5] = #FF0000</div>
            </div>
          </div>
        </div>
      </Section>

      {/* SPECS STRIP */}
      <Section bg="#fff" top={48} bottom={48}>
        <div style={{
          display: "grid", gridTemplateColumns: "repeat(5,1fr)", gap: 16,
        }}>
          {[
            { num: "640×480", label: "VGA",        cap: "// 60 Hz, hardware timing" },
            { num: "13",      label: "Colors",     cap: "// hardware palette LUT" },
            { num: "48",      label: "Shapes",     cap: "// rect · circle · digit · sprite" },
            { num: "4×8",     label: "Lawn grid",  cap: "// 80×90 px cells" },
            { num: "1",       label: "Cyclone V",  cap: "// Terasic DE1-SoC" },
          ].map(it => (
            <PixelCard key={it.label} padding="18px 18px 16px" style={{ boxShadow: "var(--shadow-pixel-1)" }}>
              <div style={{ fontFamily: "var(--font-term)", fontSize: 40, lineHeight: 1, color: "var(--ink-900)" }}>{it.num}</div>
              <div style={{ fontFamily: "var(--font-press)", fontSize: 10, letterSpacing: "0.08em", marginTop: 12, color: "var(--lawn-700)" }}>{it.label.toUpperCase()}</div>
              <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--ink-500)", marginTop: 6 }}>{it.cap}</div>
            </PixelCard>
          ))}
        </div>
      </Section>

      {/* WHAT IS THIS — for casual viewers */}
      <Section bg="var(--ink-50)" top={96} bottom={64}>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.2fr", gap: 64, alignItems: "start" }}>
          <div>
            <Eyebrow>// the explainer</Eyebrow>
            <PixelTitle size={32} style={{ marginBottom: 18 }}>
              What's an FPGA, and why put a game on one?
            </PixelTitle>
            <Lede style={{ marginBottom: 18, color: "var(--ink-700)" }}>
              Most games run on a CPU and a graphics card. This one runs on a chip you can re-wire —
              a <Code>Cyclone V FPGA</Code>. Instead of installing a graphics card, we describe one
              in code, and the chip becomes that card.
            </Lede>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              The board has two halves on one die. The ARM side runs Linux and the C game loop —
              spawning zombies, moving the cursor, ticking the sun economy. The FPGA side reads a
              little table of "shapes" and turns them into pixels, one scanline at a time, sixty times
              a second. They talk to each other through five 32-bit registers at <Code>0xff200000</Code>.
            </p>
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 18 }}>
            <ConceptCard
              role="HPS"
              title="The brain"
              tint="var(--lawn-600)"
              body="ARM Cortex-A9 running Linux. Polls the keyboard, moves the cursor, decides when zombies spawn, ticks the sun economy. Writes the next frame to the FPGA, then sleeps until vsync."
              chip="0xff200000"
              icon={<BrainIcon/>}
            />
            <ConceptCard
              role="FPGA"
              title="The pixels"
              tint="var(--silicon-800)"
              body="A custom display pipeline drawn in SystemVerilog. Reads up to 48 shape descriptors per frame, rasterizes them into a dual line buffer, walks each VGA scanline at the pixel clock."
              chip="640×480 @ 60 Hz"
              icon={<ChipIcon/>}
            />
          </div>
        </div>
      </Section>

      {/* THE FRAME, BROKEN DOWN */}
      <Section bg="#fff" top={64} bottom={96}>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "end", marginBottom: 36, gap: 32, flexWrap: "wrap" }}>
          <div>
            <Eyebrow>// one frame, 16.67 ms</Eyebrow>
            <PixelTitle size={28}>How a single frame happens.</PixelTitle>
          </div>
          <Lede max="42ch" style={{ fontSize: 16 }}>
            Sixty times a second, the C game loop on the ARM CPU pushes a new state to the FPGA.
            The FPGA waits for vertical blank, latches it, and starts drawing.
          </Lede>
        </div>
        <FrameTimeline/>
      </Section>

      {/* CONTROLS / HOW TO PLAY */}
      <Section bg="var(--silicon-900)" color="var(--ink-50)" top={96} bottom={96} scanlines>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.2fr", gap: 64, alignItems: "center" }}>
          <div>
            <Eyebrow color="var(--trace-cyan)">// controls</Eyebrow>
            <PixelTitle size={30} color="#fff" style={{ marginBottom: 18 }}>
              Five keys. Five zombies.
            </PixelTitle>
            <Lede color="var(--silicon-200)" style={{ marginBottom: 24 }}>
              The driver reads the keyboard from <Code dark>/dev/input/event0</Code>. The C
              game loop polls it at 60 Hz. No mouse, no networking. Sit at the board, hands on the
              keys, like an arcade.
            </Lede>
            <div style={{ display: "flex", gap: 8, flexWrap: "wrap" }}>
              <SpecBadge variant="trace">FPS · 60</SpecBadge>
              <SpecBadge variant="trace">SUN · 25 / 8s</SpecBadge>
              <SpecBadge variant="trace">PEA · 1 dmg</SpecBadge>
              <SpecBadge variant="trace">ZOMBIE HP · 3</SpecBadge>
            </div>
          </div>
          <PixelCard style={{ background: "var(--silicon-800)", borderColor: "var(--silicon-600)", padding: 28, color: "var(--ink-50)" }}>
            <div style={{ display: "grid", gridTemplateColumns: "auto 1fr", rowGap: 14, columnGap: 16, alignItems: "center" }}>
              <div style={{ display: "flex", gap: 4 }}>
                <Kbd>↑</Kbd><Kbd>↓</Kbd><Kbd>←</Kbd><Kbd>→</Kbd>
              </div>
              <div style={{ fontFamily: "var(--font-mono)", fontSize: 14, color: "var(--silicon-100)" }}>
                Move the cursor across the 4×8 lawn grid.
              </div>
              <Kbd w={64}>Space</Kbd>
              <div style={{ fontFamily: "var(--font-mono)", fontSize: 14, color: "var(--silicon-100)" }}>
                Place a Peashooter — costs <span style={{color:"var(--sun-300)"}}>50 sun</span>.
              </div>
              <Kbd>D</Kbd>
              <div style={{ fontFamily: "var(--font-mono)", fontSize: 14, color: "var(--silicon-100)" }}>
                Remove a plant in the cursor cell.
              </div>
              <Kbd w={48}>Esc</Kbd>
              <div style={{ fontFamily: "var(--font-mono)", fontSize: 14, color: "var(--silicon-100)" }}>
                Quit the game.
              </div>
            </div>
            <div style={{
              marginTop: 24, paddingTop: 18,
              borderTop: "1px solid var(--silicon-600)",
              fontFamily: "var(--font-mono)", fontSize: 12,
              color: "var(--silicon-400)",
            }}>
              <span style={{color:"var(--trace-cyan)"}}>$</span> ./pvz<br/>
              <span style={{color:"var(--silicon-400)"}}>// arrow keys move the cursor.<br/>
              // win condition: kill all 5 zombies. lose: any zombie reaches x=0.</span>
            </div>
          </PixelCard>
        </div>
      </Section>

      {/* DESTINATIONS — pick a tab */}
      <Section bg="var(--ink-50)" top={96} bottom={96} border={false}>
        <div style={{ marginBottom: 36 }}>
          <Eyebrow>// where to next</Eyebrow>
          <PixelTitle size={28}>Pick a rabbit hole.</PixelTitle>
        </div>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 18 }}>
          <DestCard
            label="DEMO"
            title="Play it in the browser."
            body="A faithful port of the C game loop running in JS. Same grid, same costs, same five zombies. No FPGA required."
            badge="interactive"
            tint="var(--sun-500)"
            tintFg="var(--ink-900)"
            onClick={() => { setTab("demo"); window.scrollTo(0,0); }}
          />
          <DestCard
            label="HARDWARE"
            title="The display pipeline."
            body="VGA timing, line buffers, the 13-color palette LUT, the shape table, the scanline rasterizer. SystemVerilog inside."
            badge="FPGA"
            tint="var(--silicon-800)"
            tintFg="var(--trace-cyan)"
            onClick={() => { setTab("hardware"); window.scrollTo(0,0); }}
          />
          <DestCard
            label="SOFTWARE"
            title="The game loop."
            body="C running on ARM Linux. Game state, collision, sun economy. The kernel driver. The ioctl interface."
            badge="HPS · Linux"
            tint="var(--lawn-600)"
            tintFg="#fff"
            onClick={() => { setTab("software"); window.scrollTo(0,0); }}
          />
        </div>
      </Section>
    </div>
  );
}

/* ──────────────── helpers ──────────────── */

function ConceptCard({ role, title, body, chip, tint, icon }) {
  return (
    <div style={{
      background: "#fff",
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      padding: 22,
      display: "flex", flexDirection: "column", gap: 12,
    }}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
        <span style={{
          fontFamily: "var(--font-press)", fontSize: 10, letterSpacing: "0.08em",
          background: tint, color: "#fff",
          padding: "5px 8px", border: "2px solid var(--ink-900)",
        }}>{role}</span>
        <div style={{ width: 36, height: 36, color: tint }}>{icon}</div>
      </div>
      <div style={{ fontWeight: 700, fontSize: 22, color: "var(--ink-900)" }}>{title}</div>
      <p style={{ fontSize: 14, lineHeight: 1.6, color: "var(--ink-600)", margin: 0 }}>{body}</p>
      <div style={{
        marginTop: "auto",
        fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--silicon-700)",
        background: "var(--silicon-50)", padding: "6px 8px",
        border: "1px solid var(--silicon-100)",
      }}>{chip}</div>
    </div>
  );
}

function BrainIcon() {
  return (
    <svg viewBox="0 0 36 36" fill="none" xmlns="http://www.w3.org/2000/svg">
      <rect x="2" y="6" width="32" height="24" stroke="currentColor" strokeWidth="2"/>
      <rect x="6" y="10" width="24" height="16" fill="currentColor" opacity="0.15"/>
      <rect x="9" y="13" width="4" height="4" fill="currentColor"/>
      <rect x="16" y="13" width="4" height="4" fill="currentColor"/>
      <rect x="23" y="13" width="4" height="4" fill="currentColor"/>
      <rect x="9" y="20" width="4" height="4" fill="currentColor"/>
      <rect x="16" y="20" width="4" height="4" fill="currentColor"/>
      <rect x="23" y="20" width="4" height="4" fill="currentColor"/>
      <rect x="0" y="14" width="4" height="2" fill="currentColor"/>
      <rect x="0" y="20" width="4" height="2" fill="currentColor"/>
      <rect x="32" y="14" width="4" height="2" fill="currentColor"/>
      <rect x="32" y="20" width="4" height="2" fill="currentColor"/>
    </svg>
  );
}
function ChipIcon() {
  return (
    <svg viewBox="0 0 36 36" fill="none" xmlns="http://www.w3.org/2000/svg">
      <rect x="6" y="6" width="24" height="24" stroke="currentColor" strokeWidth="2"/>
      <rect x="11" y="11" width="14" height="14" fill="currentColor" opacity="0.18"/>
      <rect x="14" y="14" width="8" height="8" fill="currentColor"/>
      <rect x="2" y="11" width="4" height="2" fill="currentColor"/>
      <rect x="2" y="17" width="4" height="2" fill="currentColor"/>
      <rect x="2" y="23" width="4" height="2" fill="currentColor"/>
      <rect x="30" y="11" width="4" height="2" fill="currentColor"/>
      <rect x="30" y="17" width="4" height="2" fill="currentColor"/>
      <rect x="30" y="23" width="4" height="2" fill="currentColor"/>
      <rect x="11" y="2" width="2" height="4" fill="currentColor"/>
      <rect x="17" y="2" width="2" height="4" fill="currentColor"/>
      <rect x="23" y="2" width="2" height="4" fill="currentColor"/>
    </svg>
  );
}

function FrameTimeline() {
  const steps = [
    { t: "t = 0 ms",     a: "HPS",  c: "var(--lawn-600)",      title: "Poll keyboard",       body: "input.c reads /dev/input/event0. Cursor moves, plant placed if user hit Space." },
    { t: "t ≈ 0.1 ms",   a: "HPS",  c: "var(--lawn-600)",      title: "Update game state",   body: "game.c ticks: zombie movement, peas fired, collision, sun economy, win/lose check." },
    { t: "t ≈ 0.5 ms",   a: "HPS",  c: "var(--lawn-600)",      title: "Build shape list",    body: "render.c walks the world and writes up to 48 shape descriptors via ioctl." },
    { t: "t ≈ 0.6 ms",   a: "BUS",  c: "var(--trace-cyan)",    title: "Commit",               body: "ioctl(PVZ_COMMIT_SHAPES) latches the shadow table. The FPGA now sees the new frame." },
    { t: "t ≈ 0.7 ms",   a: "FPGA", c: "var(--zombie-500)",    title: "Wait for vsync",       body: "Linebuffer swap roles at hsync. New shapes activate at the next vertical blank." },
    { t: "t ≈ 1 ms",     a: "FPGA", c: "var(--zombie-500)",    title: "Rasterize 480 lines",  body: "shape_renderer.sv walks 525 vcounts × 800 hcounts. Each pixel: BG cell color overwritten by any visible shape on that scanline." },
    { t: "t = 16.67 ms", a: "VGA",  c: "var(--sun-500)",       title: "Frame out the wire",   body: "640×480 pixels reach the monitor. The HPS is already half-done with the next frame." },
  ];
  return (
    <div style={{ position: "relative" }}>
      {/* timeline line */}
      <div style={{
        position: "absolute", left: 0, right: 0, top: 64,
        height: 2, background: "var(--ink-900)",
      }}/>
      <div style={{ display: "grid", gridTemplateColumns: "repeat(7, 1fr)", gap: 12 }}>
        {steps.map((s, i) => (
          <div key={i}>
            <div style={{
              fontFamily: "var(--font-press)", fontSize: 9, letterSpacing: "0.08em",
              padding: "5px 7px", background: s.c, color: s.a === "BUS" ? "var(--ink-900)" : "#fff",
              border: "2px solid var(--ink-900)",
              display: "inline-block", marginBottom: 12,
            }}>{s.a}</div>
            <div style={{ position: "relative", marginBottom: 12 }}>
              <div style={{
                position: "absolute", left: -2, top: -6,
                width: 14, height: 14, background: s.c,
                border: "2px solid var(--ink-900)",
              }}/>
            </div>
            <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--ink-500)", marginTop: 22 }}>{s.t}</div>
            <div style={{ fontWeight: 700, fontSize: 14, color: "var(--ink-900)", margin: "4px 0 6px" }}>{s.title}</div>
            <div style={{ fontSize: 12, lineHeight: 1.55, color: "var(--ink-600)" }}>{s.body}</div>
          </div>
        ))}
      </div>
    </div>
  );
}

function DestCard({ label, title, body, badge, tint, tintFg, onClick }) {
  const [hover, setHover] = useState(false);
  return (
    <div
      onClick={onClick}
      onMouseEnter={() => setHover(true)}
      onMouseLeave={() => setHover(false)}
      style={{
        background: "#fff",
        border: "2px solid var(--ink-900)",
        boxShadow: hover ? "var(--shadow-pixel-3)" : "var(--shadow-pixel-2)",
        transform: hover ? "translate(-2px,-2px)" : "none",
        cursor: "pointer",
        padding: 0,
        display: "flex", flexDirection: "column",
      }}>
      <div style={{
        background: tint, color: tintFg,
        padding: "14px 18px",
        borderBottom: "2px solid var(--ink-900)",
        display: "flex", justifyContent: "space-between", alignItems: "center",
      }}>
        <span style={{ fontFamily: "var(--font-press)", fontSize: 11, letterSpacing: "0.08em" }}>{label}</span>
        <span style={{ fontFamily: "var(--font-mono)", fontSize: 11 }}>// {badge}</span>
      </div>
      <div style={{ padding: 22, display: "flex", flexDirection: "column", gap: 10, flex: 1 }}>
        <div style={{ fontWeight: 700, fontSize: 22, color: "var(--ink-900)" }}>{title}</div>
        <p style={{ fontSize: 14, lineHeight: 1.6, color: "var(--ink-600)", margin: 0 }}>{body}</p>
        <div style={{
          marginTop: "auto",
          fontFamily: "var(--font-press)", fontSize: 10, letterSpacing: "0.08em",
          color: "var(--lawn-700)",
        }}>OPEN →</div>
      </div>
    </div>
  );
}

window.Home = Home;
