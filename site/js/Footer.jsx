/* eslint-disable */

function Footer({ setTab }) {
  return (
    <footer style={{ background: "var(--ink-900)", color: "var(--ink-200)", padding: "48px 32px" }}>
      <div style={{ maxWidth: 1200, margin: "0 auto", display: "grid", gridTemplateColumns: "1.4fr 1fr 1fr 1fr", gap: 32 }}>
        <div>
          <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 14 }}>
            <img src="assets/brandmark.png" width="32" style={{ imageRendering: "pixelated" }}/>
            <div style={{ fontFamily: "var(--font-press)", fontSize: 12, letterSpacing: "0.08em", color: "#fff" }}>pvz·fpga</div>
          </div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--silicon-200)", lineHeight: 1.6 }}>
            // Plants vs Zombies, on a chip.<br/>
            // Columbia CSEE 4840 · Embedded Systems<br/>
            // Spring 2026
          </div>
        </div>
        <FooterCol title="The site" items={[
          ["Home",     () => setTab("home")],
          ["Demo",     () => setTab("demo")],
          ["Hardware", () => setTab("hardware")],
          ["Software", () => setTab("software")],
          ["Team",     () => setTab("team")],
        ]}/>
        <FooterCol title="Source" items={[
          ["GitHub repo",    "https://github.com/nyavana/pvz-fpga"],
          ["README",         "https://github.com/nyavana/pvz-fpga#readme"],
          ["Proposal",       "https://github.com/nyavana/pvz-fpga/blob/main/doc/proposal/proposal.md"],
          ["Milestone 1",    "https://github.com/nyavana/pvz-fpga/blob/main/doc/milestone1.md"],
        ]}/>
        <FooterCol title="Course" items={[
          ["CSEE 4840",      "https://www.cs.columbia.edu/~sedwards/classes/2026/4840-spring/index.html"],
          ["Columbia SEAS",  "https://www.engineering.columbia.edu/"],
          ["DE1-SoC manual", "https://github.com/nyavana/pvz-fpga/blob/main/doc/manual/DE1-SoC_User_manual.md"],
        ]}/>
      </div>
      <div style={{
        maxWidth: 1200, margin: "32px auto 0",
        paddingTop: 18, borderTop: "1px solid var(--silicon-800)",
        display: "flex", gap: 18, justifyContent: "space-between", alignItems: "center", flexWrap: "wrap",
        fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--silicon-400)",
      }}>
        <span>// build a6eb347 · MVP shipped · post-MVP bugfixes applied</span>
        <span>0xff200000 · /dev/pvz · 60 Hz</span>
      </div>
    </footer>
  );
}

function FooterCol({ title, items }) {
  return (
    <div>
      <div style={{ fontFamily: "var(--font-press)", fontSize: 10, letterSpacing: "0.08em", color: "var(--lawn-300)", marginBottom: 14 }}>
        {title.toUpperCase()}
      </div>
      <ul style={{ listStyle: "none", margin: 0, padding: 0, display: "flex", flexDirection: "column", gap: 8 }}>
        {items.map(([label, target]) => {
          const isFn = typeof target === "function";
          return (
            <li key={label}>
              <a
                href={isFn ? "#" : target}
                target={isFn ? undefined : "_blank"}
                rel={isFn ? undefined : "noopener noreferrer"}
                onClick={isFn ? (e) => { e.preventDefault(); target(); window.scrollTo(0,0); } : undefined}
                style={{ color: "var(--lawn-200)", textDecoration: "none", fontSize: 13 }}
              >{label} →</a>
            </li>
          );
        })}
      </ul>
    </div>
  );
}

window.Footer = Footer;
