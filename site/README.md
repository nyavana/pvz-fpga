# pvz-fpga website

Static site. Plain HTML + JS + CSS — no build step required.

## Deploy on Vercel

1. Push this directory to a Git repo.
2. Import in Vercel.
3. Set **Root Directory** to `site/`.
4. Framework preset: **Other**.
5. Build command: leave **empty**.
6. Output directory: leave **empty** (Vercel will serve `site/` as static).

That's it — Vercel serves `index.html`, the JS modules, and the assets directly from CDN.

## Local preview

Any static server works:

```bash
cd site
python3 -m http.server 8000
# or
npx serve .
```

## Files

- `index.html` — entry point, loads React + Babel via CDN.
- `js/*.jsx` — components, transpiled in-browser by Babel standalone.
- `colors_and_type.css` — design tokens (palette, type, spacing, shadows).
- `fonts/` — Press Start 2P, VT323, Inter, JetBrains Mono.
- `assets/` — sprites + brandmark.
- `vercel.json` — clean URLs.
