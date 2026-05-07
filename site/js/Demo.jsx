/* eslint-disable */

function Demo() {
  return (
    <div>
      {/* HEADER */}
      <Section bg="var(--sun-300)" top={84} bottom={72}>
        <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 56, alignItems: "end" }}>
          <div>
            <Eyebrow>// the game · de1-soc · 640×480 · 60 fps</Eyebrow>
            <PixelTitle size={44} style={{ marginBottom: 22 }}>
              The game itself.
            </PixelTitle>
            <Lede max="62ch">
              A single-lane MVP of Plants vs. Zombies. Place peashooters on a 4×8 lawn,
              shoot peas, kill zombies before they reach the house. Sun ticks up at +25 every
              eight seconds. Five zombies in the wave; survive them all to win.
            </Lede>
          </div>
          <div style={{ display: "flex", flexDirection: "column", gap: 10, alignItems: "flex-start" }}>
            <SpecBadge>WAVE · 5 ZOMBIES</SpecBadge>
            <SpecBadge>START · 100 SUN</SpecBadge>
            <SpecBadge>WIN · ALL DEAD</SpecBadge>
            <SpecBadge>LOSE · ZOMBIE @ X=0</SpecBadge>
          </div>
        </div>
      </Section>

      {/* MOCK SCREEN */}
      <Section bg="var(--ink-50)" top={0} bottom={96}>
        <div style={{ marginTop: -32, display: "grid", gridTemplateColumns: "1fr auto", gap: 36, alignItems: "start" }}>
          <DemoScreen/>
          <ControlsCard/>
        </div>
      </Section>

      {/* HUD anatomy */}
      <Section bg="#fff" top={0} bottom={96}>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow>// 640×480 layout</Eyebrow>
            <PixelTitle size={26} style={{ marginBottom: 18 }}>HUD on top, lawn below.</PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              The top 60 px hold the sun counter and game-state overlays (READY / WIN / LOSE).
              The remaining 420 px host the 4-row, 8-column lawn — each cell exactly 80 × 90 px.
              The cursor is shape index 45; the HUD digits are shapes 46–47.
            </p>
            <ul style={{ listStyle: "none", padding: 0, margin: "18px 0 0", fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--ink-700)", lineHeight: 1.9 }}>
              <li><span style={{color:"var(--zombie-700)"}}>HUD</span> · y ∈ [0, 60) — sun digits, status</li>
              <li><span style={{color:"var(--lawn-600)"}}>LAWN</span> · y ∈ [60, 420) — 4 rows × 8 cols</li>
              <li><span style={{color:"var(--silicon-700)"}}>CELL</span> · 80 px wide × 90 px tall</li>
              <li><span style={{color:"var(--zombie-500)"}}>SPAWN</span> · zombies enter at x=640</li>
              <li><span style={{color:"var(--ink-900)"}}>HOUSE</span> · zombie reaching x=0 ends the game</li>
            </ul>
          </div>
          <FieldGrid/>
        </div>
      </Section>

      {/* GAME STATES */}
      <Section bg="var(--ink-50)" top={0} bottom={96}>
        <div style={{ marginBottom: 28 }}>
          <Eyebrow>// game.h — state machine</Eyebrow>
          <PixelTitle size={26}>Three states, two transitions.</PixelTitle>
        </div>
        <div style={{ display: "grid", gridTemplateColumns: "repeat(3,1fr)", gap: 16 }}>
          <StateCard label="STATE_PLAYING" tone="lawn"
            blurb="Default. Wave is live, zombies spawn over 30 seconds. Cursor moves, peas fly."
            edges={["→ WIN  if all zombies dead", "→ LOSE if any zombie x ≤ 0"]}/>
          <StateCard label="STATE_WIN" tone="sun"
            blurb="Loop continues to render but ignores input. Background flashes lawn-green."
            edges={["restart with R"]}/>
          <StateCard label="STATE_LOSE" tone="zombie"
            blurb="Background flashes red. Zombies freeze in place. Sun counter dimmed."
            edges={["restart with R"]}/>
        </div>
      </Section>

      {/* WAVE TIMELINE */}
      <Section bg="#fff" top={0} bottom={96}>
        <div style={{ marginBottom: 24 }}>
          <Eyebrow>// MVP wave · 5 zombies · 30 seconds</Eyebrow>
          <PixelTitle size={26}>The wave timeline.</PixelTitle>
        </div>
        <WaveTimeline/>
      </Section>
    </div>
  );
}

/* ───────── mock screen ───────── */
function DemoScreen() {
  return (
    <div style={{
      background: "var(--silicon-900)",
      border: "3px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-3)",
      padding: 18,
    }}>
      <div style={{
        display: "flex", justifyContent: "space-between", alignItems: "center",
        fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--trace-cyan)",
        marginBottom: 10, letterSpacing: "0.08em",
      }}>
        <span>VGA_OUT · 640×480 · 60Hz</span>
        <span style={{color:"var(--trace-mag)"}}>● REC</span>
      </div>
      <div style={{
        width: 640, height: 480,
        background: "#000",
        position: "relative",
        imageRendering: "pixelated",
        border: "2px solid var(--silicon-600)",
      }}>
        {/* HUD bar */}
        <div style={{
          position: "absolute", left: 0, top: 0, width: "100%", height: 60,
          background: "#404040", borderBottom: "2px solid #000",
          display: "flex", alignItems: "center", padding: "0 16px", gap: 14,
        }}>
          <div style={{ width: 28, height: 28, background: "#FFA500", border: "2px solid #000" }}/>
          <div style={{ fontFamily: "var(--font-term)", fontSize: 30, color: "#fff" }}>0125</div>
          <div style={{ flex: 1 }}/>
          <div style={{ fontFamily: "var(--font-press)", fontSize: 9, color: "#9aa", letterSpacing: "0.1em" }}>WAVE 1 · 3 / 5 LEFT</div>
        </div>
        {/* Lawn rows */}
        {[0,1,2,3].map(r => (
          <div key={r} style={{
            position: "absolute", left: 0, top: 60 + r * 90, width: "100%", height: 90,
            display: "grid", gridTemplateColumns: "repeat(8, 1fr)",
          }}>
            {[0,1,2,3,4,5,6,7].map(c => (
              <div key={c} style={{
                background: (r + c) % 2 ? "#2D8B2D" : "#1B5E20",
                borderRight: c < 7 ? "1px solid rgba(0,0,0,0.15)" : "none",
              }}/>
            ))}
          </div>
        ))}
        {/* Plants — peashooters */}
        <Plant col={0} row={0}/>
        <Plant col={1} row={1}/>
        <Plant col={2} row={2}/>
        <Plant col={0} row={3}/>
        {/* Peas */}
        <Pea col={2} row={0}/>
        <Pea col={3} row={1}/>
        <Pea col={4} row={1}/>
        <Pea col={3} row={3}/>
        {/* Zombies */}
        <Zombie col={5} row={0}/>
        <Zombie col={6} row={2}/>
        <Zombie col={7} row={3} eating/>
        {/* Cursor */}
        <Cursor col={1} row={0}/>
      </div>
      <div style={{
        display: "flex", justifyContent: "space-between",
        fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--silicon-400)",
        marginTop: 10,
      }}>
        <span>// snapshot · t = 12.4s · 28 / 48 shapes active</span>
        <span>VGA_HS · VGA_VS · BLANK_n</span>
      </div>
    </div>
  );
}

function Plant({ col, row }) {
  return (
    <div style={{
      position: "absolute",
      left: col * 80 + 12, top: 60 + row * 90 + 14,
      width: 56, height: 60, background: "#008000",
      border: "2px solid #000",
    }}>
      <div style={{ position:"absolute", right: -6, top: 12, width: 16, height: 16, background:"#006400", borderRadius: "50%", border: "2px solid #000" }}/>
    </div>
  );
}
function Pea({ col, row }) {
  return (
    <div style={{
      position: "absolute",
      left: col * 80 + 70, top: 60 + row * 90 + 38,
      width: 12, height: 12, background: "#00FF00",
      border: "2px solid #000", borderRadius: "50%",
    }}/>
  );
}
function Zombie({ col, row, eating }) {
  return (
    <div style={{
      position: "absolute",
      left: col * 80 + 14, top: 60 + row * 90 + 8,
      width: 50, height: 70, background: "#FF0000",
      border: "2px solid #000",
    }}>
      <div style={{ position:"absolute", left: 9, top: -10, width: 32, height: 22, background:"#8B0000", border: "2px solid #000" }}/>
      {eating && (
        <div style={{
          position: "absolute", left: -8, bottom: -10,
          fontFamily: "var(--font-press)", fontSize: 8, color: "#FFD700",
          background: "#000", padding: "2px 4px", border: "1px solid #FFD700",
          letterSpacing: "0.1em",
        }}>NOM</div>
      )}
    </div>
  );
}
function Cursor({ col, row }) {
  return (
    <div style={{
      position: "absolute",
      left: col * 80 + 4, top: 60 + row * 90 + 4,
      width: 72, height: 82,
      border: "3px solid #FFD700",
      boxShadow: "inset 0 0 0 1px #000",
    }}/>
  );
}

/* ───────── controls ───────── */
function ControlsCard() {
  return (
    <div style={{
      background: "#fff", border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)", padding: 24, minWidth: 280,
    }}>
      <div style={{
        fontFamily: "var(--font-press)", fontSize: 11, letterSpacing: "0.08em",
        marginBottom: 16, color: "var(--ink-900)",
      }}>// CONTROLS · /dev/input/event0</div>
      <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
        <KeyRow keys={["W","A","S","D"]} desc="Move cursor across the 4×8 lawn"/>
        <KeyRow keys={["E"]}              desc="Place peashooter — costs 50 sun"/>
        <KeyRow keys={["X"]}              desc="Remove plant from current cell"/>
        <KeyRow keys={["R"]}              desc="Restart after WIN or LOSE"/>
        <KeyRow keys={["ESC"]}            desc="Quit and close /dev/pvz"/>
      </div>
    </div>
  );
}

function KeyRow({ keys, desc }) {
  return (
    <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
      <div style={{ display: "flex", gap: 4 }}>
        {keys.map(k => (
          <kbd key={k} style={{
            fontFamily: "var(--font-press)", fontSize: 10,
            background: "var(--ink-50)",
            border: "2px solid var(--ink-900)",
            boxShadow: "2px 2px 0 var(--ink-900)",
            padding: "5px 8px", minWidth: 20, textAlign: "center",
          }}>{k}</kbd>
        ))}
      </div>
      <div style={{ fontSize: 13, color: "var(--ink-700)", lineHeight: 1.5 }}>{desc}</div>
    </div>
  );
}

/* ───────── 4×8 grid */
function FieldGrid() {
  return (
    <div style={{
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      background: "var(--silicon-900)",
      padding: 16,
    }}>
      <div style={{ display: "grid", gridTemplateColumns: "repeat(8, 1fr)", gridTemplateRows: "repeat(4, 60px)", gap: 2 }}>
        {Array.from({length:32}).map((_,i) => {
          const r = Math.floor(i / 8), c = i % 8;
          return (
            <div key={i} style={{
              background: (r + c) % 2 ? "#2D8B2D" : "#1B5E20",
              border: "1px solid rgba(0,0,0,0.4)",
              display: "flex", alignItems: "center", justifyContent: "center",
              fontFamily: "var(--font-mono)", fontSize: 10, color: "rgba(255,255,255,0.45)",
            }}>{`${r},${c}`}</div>
          );
        })}
      </div>
      <div style={{
        marginTop: 10, fontFamily: "var(--font-mono)", fontSize: 11,
        color: "var(--silicon-400)", display: "flex", justifyContent: "space-between",
      }}>
        <span>// shape index = r·8 + c · range 0..31</span>
        <span>{"4 × 8 = 32 cells"}</span>
      </div>
    </div>
  );
}

/* ───────── state cards */
function StateCard({ label, tone, blurb, edges }) {
  const colors = {
    lawn:   { bg: "var(--lawn-600)",   fg: "#fff" },
    sun:    { bg: "var(--sun-500)",    fg: "var(--ink-900)" },
    zombie: { bg: "var(--zombie-500)", fg: "#fff" },
  }[tone];
  return (
    <div style={{
      background: "#fff", border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
    }}>
      <div style={{
        background: colors.bg, color: colors.fg,
        padding: "10px 14px", fontFamily: "var(--font-mono)", fontSize: 14,
        borderBottom: "2px solid var(--ink-900)",
      }}>{label}</div>
      <div style={{ padding: "16px 18px" }}>
        <p style={{ fontSize: 14, color: "var(--ink-700)", lineHeight: 1.6, marginTop: 0 }}>{blurb}</p>
        <ul style={{ listStyle: "none", margin: 0, padding: 0, fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--zombie-700)" }}>
          {edges.map(e => <li key={e} style={{ padding: "4px 0" }}>{e}</li>)}
        </ul>
      </div>
    </div>
  );
}

/* ───────── wave timeline */
function WaveTimeline() {
  const spawns = [4, 9, 16, 22, 27]; // seconds
  const total = 30;
  return (
    <div style={{
      background: "var(--silicon-900)",
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      padding: 24,
    }}>
      <div style={{ position: "relative", height: 96, margin: "12px 24px 8px" }}>
        {/* track */}
        <div style={{
          position: "absolute", left: 0, right: 0, top: 48,
          height: 4, background: "var(--silicon-600)",
        }}/>
        {/* ticks */}
        {Array.from({length: total + 1}).map((_,i) => i % 5 === 0 && (
          <div key={i} style={{
            position: "absolute",
            left: `${(i / total) * 100}%`,
            top: 40, width: 2, height: 20, background: "var(--silicon-400)",
            transform: "translateX(-1px)",
          }}>
            <div style={{
              position: "absolute", top: 24, left: -6,
              fontFamily: "var(--font-mono)", fontSize: 10,
              color: "var(--silicon-400)",
            }}>{i}s</div>
          </div>
        ))}
        {/* zombies */}
        {spawns.map((s, i) => (
          <div key={i} style={{
            position: "absolute",
            left: `${(s / total) * 100}%`,
            top: 0, transform: "translateX(-50%)",
            display: "flex", flexDirection: "column", alignItems: "center", gap: 4,
          }}>
            <div style={{
              fontFamily: "var(--font-press)", fontSize: 9, color: "var(--zombie-500)",
              letterSpacing: "0.1em",
            }}>Z{i+1}</div>
            <div style={{
              width: 18, height: 26, background: "var(--zombie-500)",
              border: "2px solid var(--ink-900)",
            }}/>
            <div style={{ width: 2, height: 12, background: "var(--zombie-500)" }}/>
          </div>
        ))}
      </div>
      <div style={{
        display: "flex", justifyContent: "space-between",
        fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--silicon-200)",
        padding: "0 24px",
      }}>
        <span>// spawns at t = 4, 9, 16, 22, 27 (deterministic, srand(42))</span>
        <span style={{color:"var(--trace-cyan)"}}>survive all 5 → STATE_WIN</span>
      </div>
    </div>
  );
}

window.Demo = Demo;
