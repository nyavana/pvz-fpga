/* eslint-disable */
const { useState, useEffect, useRef, useMemo, useCallback } = React;

/* ──────────────────────────────────────── primitives ──────── */

function PixelButton({ children, variant = "primary", onClick, as = "button", href, target, size = "md" }) {
  const variants = {
    primary: { bg: "var(--lawn-600)", fg: "#fff" },
    sun:     { bg: "var(--sun-500)",  fg: "var(--ink-900)" },
    ghost:   { bg: "var(--ink-0)",    fg: "var(--ink-900)" },
    night:   { bg: "var(--ink-900)",  fg: "var(--lawn-200)" },
    zombie:  { bg: "var(--zombie-500)", fg: "#fff" },
  };
  const s = variants[variant] || variants.primary;
  const sizes = {
    sm: { pad: "8px 12px", fs: 10 },
    md: { pad: "14px 20px", fs: 12 },
    lg: { pad: "18px 26px", fs: 14 },
  }[size];
  const [pressed, setPressed] = useState(false);
  const Tag = as;
  return (
    <Tag
      href={href}
      target={target}
      onClick={onClick}
      onMouseDown={() => setPressed(true)}
      onMouseUp={() => setPressed(false)}
      onMouseLeave={() => setPressed(false)}
      style={{
        display: "inline-block",
        fontFamily: "var(--font-press)",
        fontSize: sizes.fs,
        letterSpacing: "0.08em",
        padding: sizes.pad,
        background: s.bg,
        color: s.fg,
        border: "2px solid var(--ink-900)",
        boxShadow: pressed ? "var(--shadow-pixel-1)" : "var(--shadow-pixel-2)",
        transform: pressed ? "translate(2px,2px)" : "none",
        borderRadius: 0,
        cursor: "pointer",
        textDecoration: "none",
        userSelect: "none",
        textAlign: "center",
        whiteSpace: "nowrap",
      }}
    >
      {children}
    </Tag>
  );
}

function SpecBadge({ children, variant = "lawn" }) {
  const styles = {
    lawn:    { bg: "var(--lawn-600)", fg: "#fff" },
    sun:     { bg: "var(--sun-500)", fg: "var(--ink-900)" },
    night:   { bg: "var(--ink-900)", fg: "var(--lawn-200)" },
    fpga:    { bg: "var(--silicon-900)", fg: "var(--trace-cyan)" },
    paper:   { bg: "#fff", fg: "var(--ink-900)" },
    zombie:  { bg: "var(--zombie-500)", fg: "#fff" },
    trace:   { bg: "transparent", fg: "var(--trace-cyan)", borderColor: "var(--trace-cyan)" },
  };
  const s = styles[variant] || styles.lawn;
  return (
    <span style={{
      fontFamily: "var(--font-press)",
      fontSize: 10,
      letterSpacing: "0.08em",
      padding: "7px 10px",
      background: s.bg,
      color: s.fg,
      border: "2px solid " + (s.borderColor || "var(--ink-900)"),
      whiteSpace: "nowrap",
      display: "inline-block",
    }}>{children}</span>
  );
}

function Eyebrow({ children, color = "var(--lawn-700)" }) {
  return (
    <div style={{
      fontFamily: "var(--font-press)",
      fontSize: 10,
      letterSpacing: "0.10em",
      textTransform: "uppercase",
      color,
      marginBottom: 12,
    }}>{children}</div>
  );
}

function Kbd({ children, w }) {
  return (
    <span style={{
      display: "inline-flex",
      alignItems: "center",
      justifyContent: "center",
      minWidth: w || 28, height: 28,
      padding: "0 8px",
      border: "2px solid var(--ink-900)",
      borderBottomWidth: 4,
      borderRadius: 4,
      background: "#fff",
      fontFamily: "var(--font-mono)",
      fontSize: 13,
      fontWeight: 600,
      color: "var(--ink-900)",
    }}>{children}</span>
  );
}

/* Display heading: pixel sentence-case, with optional accent on second line */
function PixelTitle({ children, size = 44, color = "var(--ink-900)", style = {} }) {
  return (
    <h1 style={{
      fontFamily: "var(--font-press)",
      fontSize: size,
      lineHeight: 1.12,
      letterSpacing: "0.04em",
      margin: 0,
      color,
      ...style,
    }}>{children}</h1>
  );
}

function Lede({ children, max = "58ch", color = "var(--ink-700)", style = {} }) {
  return (
    <p style={{
      fontSize: 19,
      lineHeight: 1.6,
      color,
      maxWidth: max,
      margin: 0,
      ...style,
    }}>{children}</p>
  );
}

/* Inline code chip */
function Code({ children, dark }) {
  return (
    <code style={{
      fontFamily: "var(--font-mono)",
      fontSize: "0.92em",
      background: dark ? "rgba(54,214,224,0.10)" : "var(--silicon-50)",
      color: dark ? "var(--trace-cyan)" : "var(--silicon-800)",
      padding: "2px 6px",
      border: "1px solid " + (dark ? "rgba(54,214,224,0.3)" : "var(--silicon-100)"),
      borderRadius: 2,
      whiteSpace: "nowrap",
    }}>{children}</code>
  );
}

/* Code block — multiline */
function CodeBlock({ children, lang, lines = false, height }) {
  const text = String(children || "");
  const rows = text.split("\n");
  return (
    <div style={{
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      background: "var(--silicon-900)",
      maxHeight: height,
      overflow: height ? "auto" : "visible",
    }} className="pretty-scroll">
      <div style={{
        display: "flex", justifyContent: "space-between", alignItems: "center",
        background: "var(--silicon-800)", color: "var(--trace-cyan)",
        padding: "8px 14px",
        fontFamily: "var(--font-mono)", fontSize: 11, letterSpacing: "0.08em",
        borderBottom: "1px solid var(--silicon-600)",
      }}>
        <span>// {lang || "code"}</span>
        <span style={{ color: "var(--silicon-400)" }}>{rows.length} lines</span>
      </div>
      <pre style={{
        margin: 0, padding: 16,
        fontFamily: "var(--font-mono)", fontSize: 13, lineHeight: 1.55,
        color: "var(--silicon-100)",
        whiteSpace: "pre",
        overflowX: "auto",
      }} className="pretty-scroll">
        {lines ? rows.map((r,i) => (
          <div key={i} style={{ display: "flex" }}>
            <span style={{ color: "var(--silicon-600)", width: 28, flexShrink: 0, userSelect: "none" }}>{i+1}</span>
            <span><CodeColor line={r}/></span>
          </div>
        )) : <CodeColor line={text}/>}
      </pre>
    </div>
  );
}

/* Very small token-based syntax highlighting for code. */
function CodeColor({ line }) {
  const re = /(\/\/[^\n]*|\/\*[\s\S]*?\*\/|"[^"]*"|'[^']*'|0x[0-9a-fA-F]+|\b\d+\b|\b(?:int|void|char|short|unsigned|long|static|const|return|if|else|for|while|module|always_comb|always_ff|logic|input|output|wire|reg|assign|case|endcase|endmodule|begin|end|struct|typedef|include|define|ifndef|endif|extern|if|switch|do|break|continue|sizeof|true|false|null|NULL)\b)/g;
  const parts = [];
  let last = 0, m;
  while ((m = re.exec(line)) !== null) {
    if (m.index > last) parts.push({ t: line.slice(last, m.index), k: "x" });
    let k = "kw";
    if (m[0].startsWith("//") || m[0].startsWith("/*")) k = "cm";
    else if (m[0][0] === '"' || m[0][0] === "'") k = "st";
    else if (/^0x|^\d/.test(m[0])) k = "nu";
    parts.push({ t: m[0], k });
    last = m.index + m[0].length;
  }
  if (last < line.length) parts.push({ t: line.slice(last), k: "x" });
  const colors = {
    cm: "var(--silicon-400)",
    kw: "var(--trace-mag)",
    st: "var(--lawn-300)",
    nu: "var(--sun-300)",
    x:  "var(--silicon-100)",
  };
  return parts.map((p, i) => (
    <span key={i} style={{ color: colors[p.k] }}>{p.t}</span>
  ));
}

/* Section wrapper with consistent rhythm */
function Section({ id, bg = "var(--ink-50)", color, children, top = 96, bottom = 96, scanlines, border = true }) {
  return (
    <section id={id} style={{
      background: bg,
      color: color,
      padding: `${top}px 32px ${bottom}px`,
      borderBottom: border ? "2px solid var(--ink-900)" : "none",
      position: "relative",
      backgroundImage: scanlines ? "repeating-linear-gradient(0deg, rgba(255,255,255,0.04) 0 1px, transparent 1px 3px)" : undefined,
    }}>
      <div style={{ maxWidth: 1200, margin: "0 auto" }}>{children}</div>
    </section>
  );
}

/* Card with pixel frame + drop shadow */
function PixelCard({ children, style = {}, padding = 24 }) {
  return (
    <div style={{
      background: "#fff",
      border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-2)",
      padding,
      ...style,
    }}>{children}</div>
  );
}

/* "lawn" panel — checkered green section background */
function LawnPanel({ children, style = {}, cell = 80 }) {
  return (
    <div style={{
      backgroundColor: "var(--lawn-500)",
      backgroundImage:
        "linear-gradient(45deg, var(--lawn-600) 25%, transparent 25%, transparent 75%, var(--lawn-600) 75%)," +
        "linear-gradient(45deg, var(--lawn-600) 25%, transparent 25%, transparent 75%, var(--lawn-600) 75%)",
      backgroundSize: `${cell*2}px ${cell*2}px`,
      backgroundPosition: `0 0, ${cell}px ${cell}px`,
      ...style,
    }}>{children}</div>
  );
}

window.PixelButton = PixelButton;
window.SpecBadge = SpecBadge;
window.Eyebrow = Eyebrow;
window.Kbd = Kbd;
window.PixelTitle = PixelTitle;
window.Lede = Lede;
window.Code = Code;
window.CodeBlock = CodeBlock;
window.Section = Section;
window.PixelCard = PixelCard;
window.LawnPanel = LawnPanel;
