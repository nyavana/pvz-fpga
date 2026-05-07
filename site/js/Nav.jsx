/* eslint-disable */

function Nav({ tab, setTab }) {
  const tabs = [
    { id: "home",     label: "Home" },
    { id: "demo",     label: "Demo" },
    { id: "hardware", label: "Hardware" },
    { id: "software", label: "Software" },
    { id: "team",     label: "Team" },
  ];
  return (
    <header style={{
      position: "sticky", top: 0, zIndex: 50,
      borderBottom: "2px solid var(--ink-900)",
      background: "var(--ink-50)",
      backgroundImage: "linear-gradient(180deg, var(--ink-50) 0%, var(--ink-50) 100%)",
    }}>
      <div style={{
        maxWidth: 1200, margin: "0 auto",
        padding: "12px 32px",
        display: "flex", alignItems: "center", gap: 24, flexWrap: "wrap",
      }}>
        <a href="#" onClick={(e)=>{e.preventDefault(); setTab("home");}} style={{ display: "flex", alignItems: "center", gap: 10, textDecoration: "none" }}>
          <img src="assets/brandmark.png" width="36" height="36" style={{ imageRendering: "pixelated" }} alt=""/>
          <span style={{
            fontFamily: "var(--font-press)",
            fontSize: 14, letterSpacing: "0.08em",
            color: "var(--ink-900)",
          }}>pvz<span style={{color:"var(--lawn-600)"}}>·</span>fpga</span>
        </a>
        <nav style={{ display: "flex", gap: 4, marginLeft: 20 }}>
          {tabs.map(t => {
            const active = tab === t.id;
            return (
              <button
                key={t.id}
                onClick={() => setTab(t.id)}
                style={{
                  fontFamily: "var(--font-press)",
                  fontSize: 11, letterSpacing: "0.08em",
                  padding: "10px 14px",
                  background: active ? "var(--ink-900)" : "transparent",
                  color: active ? "var(--lawn-200)" : "var(--ink-700)",
                  border: "2px solid " + (active ? "var(--ink-900)" : "transparent"),
                  cursor: "pointer",
                  textTransform: "uppercase",
                  position: "relative",
                  top: 0,
                }}>{t.label}</button>
            );
          })}
        </nav>
        <div style={{ marginLeft: "auto", display: "flex", gap: 10, alignItems: "center" }}>
          <SpecBadge variant="paper">v1.0 · MVP</SpecBadge>
          <PixelButton variant="ghost" as="a" href="https://github.com/nyavana/pvz-fpga" target="_blank" size="sm">GITHUB →</PixelButton>
        </div>
      </div>
    </header>
  );
}
window.Nav = Nav;
