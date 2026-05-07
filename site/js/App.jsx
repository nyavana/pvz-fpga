/* eslint-disable */

const { useState, useEffect } = React;

function App() {
  const [tab, setTab] = useState(() => {
    const h = window.location.hash.replace("#", "");
    return ["home","hardware","software","demo","team"].includes(h) ? h : "home";
  });

  useEffect(() => {
    window.location.hash = tab;
    window.scrollTo({ top: 0, behavior: "instant" });
  }, [tab]);

  useEffect(() => {
    const onHash = () => {
      const h = window.location.hash.replace("#", "");
      if (["home","hardware","software","demo","team"].includes(h)) setTab(h);
    };
    window.addEventListener("hashchange", onHash);
    return () => window.removeEventListener("hashchange", onHash);
  }, []);

  return (
    <div style={{ background: "var(--ink-50)", color: "var(--ink-900)", fontFamily: "var(--font-body)" }}>
      <Nav tab={tab} setTab={setTab}/>
      <main style={{ paddingTop: 60 /* nav height */ }}>
        {tab === "home"     && <Home setTab={setTab}/>}
        {tab === "hardware" && <Hardware/>}
        {tab === "software" && <Software/>}
        {tab === "demo"     && <Demo/>}
        {tab === "team"     && <Team/>}
      </main>
      <Footer setTab={setTab}/>
    </div>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App/>);
