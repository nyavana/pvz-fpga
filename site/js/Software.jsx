/* eslint-disable */

function Software() {
  return (
    <div>
      {/* HEADER */}
      <Section bg="var(--lawn-50)" top={84} bottom={72}>
        <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 56, alignItems: "end" }}>
          <div>
            <Eyebrow>// software · arm linux · c · kbuild</Eyebrow>
            <PixelTitle size={44} style={{ marginBottom: 22 }}>
              The game, in C.
            </PixelTitle>
            <Lede max="62ch">
              The HPS runs Linux. A C program (<Code>./pvz</Code>) calls a kernel module (<Code>pvz_driver.ko</Code>)
              over <Code>/dev/pvz</Code>. The kernel module pokes the FPGA's memory-mapped registers.
              The whole loop fits in seven small files and runs at a steady 60 Hz.
            </Lede>
          </div>
          <div style={{ display: "flex", flexDirection: "column", gap: 10, alignItems: "flex-start" }}>
            <SpecBadge variant="lawn">FPS · 60</SpecBadge>
            <SpecBadge variant="lawn">GRID · 4 × 8</SpecBadge>
            <SpecBadge variant="lawn">SUN · +25 / 8s</SpecBadge>
            <SpecBadge variant="lawn">PEA · 2 px / frame</SpecBadge>
            <SpecBadge variant="lawn">ZOMBIE · 1 px / 3 frames</SpecBadge>
          </div>
        </div>
      </Section>

      {/* SW STACK */}
      <Section bg="#fff" top={64} bottom={96}>
        <div style={{ marginBottom: 28 }}>
          <Eyebrow>// the stack</Eyebrow>
          <PixelTitle size={26}>Userspace, kernel, register file.</PixelTitle>
        </div>
        <SoftwareStack/>
      </Section>

      {/* GAME LOOP */}
      <Section bg="var(--ink-50)" top={96} bottom={96}>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.2fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow>// main.c</Eyebrow>
            <PixelTitle size={28} style={{ marginBottom: 18 }}>
              The 60 Hz loop.
            </PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              <Code>main.c</Code> opens <Code>/dev/pvz</Code>, the keyboard, and runs an
              ordinary <Code>while(1)</Code> with a sleep at the end. No vsync interrupt, no
              real-time scheduler — Linux's nanosleep is good enough for this one.
            </p>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              Every iteration is the same four steps: poll input, update state, render
              shapes, commit. The FPGA latches the new shape table at the next vertical blank.
            </p>
            <div style={{ display: "flex", gap: 8, marginTop: 18, flexWrap: "wrap" }}>
              <SpecBadge variant="paper">main.c</SpecBadge>
              <SpecBadge variant="paper">game.c</SpecBadge>
              <SpecBadge variant="paper">render.c</SpecBadge>
              <SpecBadge variant="paper">input.c</SpecBadge>
            </div>
          </div>
          <CodeBlock lang="c · sw/main.c (excerpt)" lines>{`int fd_pvz = open("/dev/pvz", O_RDWR);
int fd_kbd = input_open("/dev/input/event0");

game_state_t gs;
game_init(&gs);

const long ns_per_frame = 16666666; /* ~60 Hz */
struct timespec t0, t1;

while (gs.state == STATE_PLAYING) {
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* 1. read keyboard, update cursor / place / remove */
    input_poll(fd_kbd, &gs);

    /* 2. tick the world */
    game_update(&gs);

    /* 3. translate state → shape table over ioctl */
    render_frame(fd_pvz, &gs);

    /* 4. tell the FPGA "swap on next vsync" */
    ioctl(fd_pvz, PVZ_COMMIT_SHAPES);

    /* 5. sleep until the next 16.67 ms tick */
    clock_gettime(CLOCK_MONOTONIC, &t1);
    nanosleep(&(struct timespec){
        .tv_nsec = ns_per_frame - elapsed(&t0, &t1)
    }, NULL);
}`}</CodeBlock>
        </div>
      </Section>

      {/* GAME RULES */}
      <Section bg="#fff" top={96} bottom={96}>
        <div style={{ marginBottom: 36 }}>
          <Eyebrow>// game.h — the constants that define the game</Eyebrow>
          <PixelTitle size={26}>Numbers worth knowing.</PixelTitle>
        </div>
        <div style={{ display: "grid", gridTemplateColumns: "repeat(4,1fr)", gap: 16 }}>
          <ConstCard k="GRID" v="4 × 8" desc="Rows × columns. 32 cells total."/>
          <ConstCard k="CELL" v="80 × 90 px" desc="One grid cell on screen."/>
          <ConstCard k="GAME_AREA_Y" v="60 px" desc="Top of the playable area; HUD lives above."/>
          <ConstCard k="PLANT_COST" v="50 sun" desc="Peashooter price."/>
          <ConstCard k="PLANT_HP" v="3" desc="Hits before a plant is destroyed."/>
          <ConstCard k="PLANT_FIRE_COOLDOWN" v="120 frames" desc="2 seconds between peas."/>
          <ConstCard k="MAX_ZOMBIES" v="5" desc="Total zombies in MVP wave."/>
          <ConstCard k="ZOMBIE_HP" v="3" desc="Three peas to kill."/>
          <ConstCard k="ZOMBIE_SPEED" v="1 px / 3 frames" desc="≈ 20 px / second."/>
          <ConstCard k="ZOMBIE_EAT_COOLDOWN" v="60 frames" desc="One bite per second."/>
          <ConstCard k="PEA_SPEED" v="2 px / frame" desc="≈ 120 px / second."/>
          <ConstCard k="SUN_INCREMENT" v="+25 / 8s" desc="Passive sun economy."/>
        </div>
      </Section>

      {/* IOCTL INTERFACE */}
      <Section bg="var(--silicon-900)" color="var(--ink-50)" top={96} bottom={96} scanlines>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.4fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow color="var(--trace-cyan)">// pvz.h — kernel ↔ userspace</Eyebrow>
            <PixelTitle size={28} color="#fff" style={{ marginBottom: 18 }}>
              Three ioctls.<br/>
              <span style={{color:"var(--trace-cyan)"}}>One device node.</span>
            </PixelTitle>
            <Lede color="var(--silicon-200)" style={{ marginBottom: 18 }}>
              The kernel module is a misc device. Userspace opens <Code dark>/dev/pvz</Code> and writes
              shapes through structured ioctls. The driver translates each call into 1–3 word writes on
              the lightweight Avalon-MM bridge.
            </Lede>
            <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
              <IoctlRow code="PVZ_WRITE_BG"
                arg="pvz_bg_arg_t { row, col, color }"
                desc="One call per background cell. 32 cells × ~per-frame writes."/>
              <IoctlRow code="PVZ_WRITE_SHAPE"
                arg="pvz_shape_arg_t { index, type, visible, x, y, w, h, color }"
                desc="Up to 48 calls per frame — one per shape table entry."/>
              <IoctlRow code="PVZ_COMMIT_SHAPES"
                arg="(no arg)"
                desc="Writes 1 to SHAPE_COMMIT. Hardware latches shadow → active on next vsync."/>
            </div>
          </div>
          <CodeBlock lang="c · sw/pvz.h" lines>{`/* Avalon register byte offsets */
#define PVZ_BG_CELL      0x00
#define PVZ_SHAPE_ADDR   0x04
#define PVZ_SHAPE_DATA0  0x08
#define PVZ_SHAPE_DATA1  0x0C
#define PVZ_SHAPE_COMMIT 0x10

/* Shape types */
#define SHAPE_RECT    0
#define SHAPE_CIRCLE  1
#define SHAPE_DIGIT   2
#define SHAPE_SPRITE  3

typedef struct {
    unsigned char  index;
    unsigned char  type;
    unsigned char  visible;
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short h;
    unsigned char  color;
} pvz_shape_arg_t;

#define PVZ_MAGIC 'p'
#define PVZ_WRITE_BG      _IOW(PVZ_MAGIC, 1, pvz_bg_arg_t)
#define PVZ_WRITE_SHAPE   _IOW(PVZ_MAGIC, 2, pvz_shape_arg_t)
#define PVZ_COMMIT_SHAPES _IO( PVZ_MAGIC, 3)`}</CodeBlock>
        </div>
      </Section>

      {/* RENDER STAGE */}
      <Section bg="var(--ink-50)" top={96} bottom={96}>
        <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow>// render.c — translating game state to pixels</Eyebrow>
            <PixelTitle size={26} style={{ marginBottom: 18 }}>
              State → shape table.
            </PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              The C side never touches a pixel. <Code>render.c</Code> walks the game state in a fixed
              order and emits up to 48 shape descriptors. Indices are reserved by category so z-order
              is predictable: lawn cells first, plants, zombies, peas, then HUD on top.
            </p>
            <ZOrderTable/>
          </div>
          <CodeBlock lang="c · sw/render.c (sketch)" lines height={420}>{`/* render plants — indices 0..31 */
for (int r = 0; r < GRID_ROWS; r++) {
  for (int c = 0; c < GRID_COLS; c++) {
    plant_t *p = &gs->grid[r][c];
    pvz_shape_arg_t s = {
      .index   = r * GRID_COLS + c,
      .type    = SHAPE_RECT,
      .visible = (p->type == PLANT_PEASHOOTER),
      .x       = c * CELL_WIDTH + 12,
      .y       = GAME_AREA_Y + r * CELL_HEIGHT + 14,
      .w       = 56, .h = 60,
      .color   = 7, /* pal[7] = green */
    };
    ioctl(fd, PVZ_WRITE_SHAPE, &s);
  }
}

/* render zombies — indices 32..36 */
for (int i = 0; i < MAX_ZOMBIES; i++) {
  zombie_t *z = &gs->zombies[i];
  pvz_shape_arg_t s = {
    .index   = 32 + i,
    .type    = SHAPE_RECT,
    .visible = z->active,
    .x = z->x_pixel,
    .y = GAME_AREA_Y + z->row * CELL_HEIGHT + 10,
    .w = ZOMBIE_WIDTH, .h = ZOMBIE_HEIGHT,
    .color   = 5, /* pal[5] = red */
  };
  ioctl(fd, PVZ_WRITE_SHAPE, &s);
}

/* render peas, cursor, HUD digits ... */`}</CodeBlock>
        </div>
      </Section>

      {/* DRIVER */}
      <Section bg="#fff" top={96} bottom={96}>
        <div style={{ display: "grid", gridTemplateColumns: "1.1fr 1fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow>// pvz_driver.c — kernel module</Eyebrow>
            <PixelTitle size={26} style={{ marginBottom: 18 }}>
              A misc device, ~150 lines.
            </PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              The platform driver matches a device-tree compatible string of <Code>"csee4840,pvz_gpu-1.0"</Code>,
              ioremaps the FPGA peripheral region (around <Code>0xff200000</Code>), and registers a misc
              device at <Code>/dev/pvz</Code>. <Code>unlocked_ioctl</Code> dispatches the three commands.
            </p>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              Build with the kernel headers on the board's running kernel; load with <Code>insmod</Code>.
              Verify probe success with <Code>dmesg | tail</Code> and <Code>cat /proc/iomem</Code>.
            </p>
            <CodeBlock lang="bash">{`# on the DE1-SoC, after copying pvz_driver.ko & pvz to /
$ insmod pvz_driver.ko
$ dmesg | tail
[  142.310] pvz_driver: probed at 0xff200000
$ ls /dev/pvz
/dev/pvz
$ ./pvz`}</CodeBlock>
          </div>
          <CodeBlock lang="c · sw/pvz_driver.c (excerpt)" lines height={520}>{`static long pvz_ioctl(struct file *f,
                      unsigned int cmd,
                      unsigned long arg)
{
    pvz_shape_arg_t s;
    pvz_bg_arg_t    b;
    void __iomem   *base = pvz.base;

    switch (cmd) {
    case PVZ_WRITE_BG:
        if (copy_from_user(&b, (void __user *)arg, sizeof(b)))
            return -EFAULT;
        iowrite32((b.row << 16) | (b.col << 8) | b.color,
                  base + PVZ_BG_CELL);
        return 0;

    case PVZ_WRITE_SHAPE:
        if (copy_from_user(&s, (void __user *)arg, sizeof(s)))
            return -EFAULT;
        iowrite32(s.index, base + PVZ_SHAPE_ADDR);
        iowrite32(pack_data0(&s), base + PVZ_SHAPE_DATA0);
        iowrite32(pack_data1(&s), base + PVZ_SHAPE_DATA1);
        return 0;

    case PVZ_COMMIT_SHAPES:
        iowrite32(1, base + PVZ_SHAPE_COMMIT);
        return 0;
    }
    return -EINVAL;
}`}</CodeBlock>
        </div>
      </Section>

      {/* TESTS */}
      <Section bg="var(--lawn-50)" top={96} bottom={96} border={false}>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1.2fr", gap: 56, alignItems: "start" }}>
          <div>
            <Eyebrow>// tests · 12 cases · srand(42)</Eyebrow>
            <PixelTitle size={26} style={{ marginBottom: 18 }}>
              Twelve unit tests for the game loop.
            </PixelTitle>
            <p style={{ fontSize: 16, lineHeight: 1.7, color: "var(--ink-700)" }}>
              <Code>test_game</Code> is a pure-C harness over <Code>game.c</Code> with no hardware
              dependency. Run on any Linux machine. Deterministic via <Code>srand(42)</Code>.
              Plus on-board test programs (<Code>test_shapes</Code>, <Code>test_input</Code>) that
              exercise the driver directly.
            </p>
            <CodeBlock lang="bash">{`$ make test_game && ./test_game
[ ok ] test_init
[ ok ] test_place_plant
[ ok ] test_remove_plant
[ ok ] test_sun_economy
[ ok ] test_collision
[ ok ] test_zombie_death
[ ok ] test_lose_condition
[ ok ] test_win_condition
[ ok ] test_zombie_stops_at_plant
[ ok ] test_zombie_eats_and_destroys_plant
[ ok ] test_zombie_resumes_after_eating
[ ok ] test_two_zombies_eat_same_plant
12 passed, 0 failed`}</CodeBlock>
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 }}>
            {[
              ["test_init", "sun=100 · state=PLAYING · cursor (0,0) · grid empty"],
              ["test_place_plant", "plant lands · sun−=50 · rejects duplicate / low sun"],
              ["test_remove_plant", "removes plant · rejects empty cell"],
              ["test_sun_economy", "+25 every 480 frames"],
              ["test_collision", "pea hits zombie in same row · −1 HP · pea inactive"],
              ["test_zombie_death", "1 HP zombie dies on next hit"],
              ["test_lose_condition", "any zombie at x ≤ 0 → LOSE"],
              ["test_win_condition", "all 5 spawned and killed → WIN"],
              ["test_zombie_stops_at_plant", "zombie enters plant cell · starts eating"],
              ["test_zombie_eats_and_destroys_plant", "DOT until plant HP=0 · plant gone"],
              ["test_zombie_resumes_after_eating", "clears eat state · resumes walk"],
              ["test_two_zombies_eat_same_plant", "both eat one plant · both resume"],
            ].map(([n, d]) => (
              <div key={n} style={{
                background: "#fff", border: "2px solid var(--ink-900)",
                boxShadow: "var(--shadow-pixel-1)", padding: "10px 12px",
              }}>
                <div style={{ display: "flex", alignItems: "center", gap: 6, marginBottom: 4 }}>
                  <span style={{ fontFamily: "var(--font-press)", fontSize: 9, letterSpacing: "0.08em",
                                 background: "var(--lawn-600)", color: "#fff", padding: "3px 5px",
                                 border: "2px solid var(--ink-900)" }}>OK</span>
                  <span style={{ fontFamily: "var(--font-mono)", fontSize: 12, color: "var(--ink-900)", fontWeight: 600 }}>{n}</span>
                </div>
                <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--ink-600)", lineHeight: 1.5 }}>
                  {d}
                </div>
              </div>
            ))}
          </div>
        </div>
      </Section>
    </div>
  );
}

/* ───────── stack diagram ───────── */
function SoftwareStack() {
  const layers = [
    {
      tag: "USER", color: "var(--lawn-600)",
      title: "./pvz — game executable",
      lines: ["main.c", "game.c · game.h", "render.c · render.h", "input.c · input.h"],
      note: "compiled with gcc -O2 -lpthread on the board",
    },
    {
      tag: "LIBC", color: "var(--ink-700)",
      title: "open · ioctl · nanosleep · read",
      lines: ["/dev/pvz       ← writes shapes", "/dev/input/event0  ← reads keyboard", "POSIX clock_gettime / nanosleep"],
      note: "no extra libraries beyond libc + libpthread",
    },
    {
      tag: "KERNEL", color: "var(--silicon-800)",
      title: "pvz_driver.ko — misc device",
      lines: ["platform_driver match: \"csee4840,pvz_gpu-1.0\"", "ioremap → ff200000", "miscdevice /dev/pvz", "unlocked_ioctl: WRITE_BG / WRITE_SHAPE / COMMIT"],
      note: "built with kbuild against /usr/src/linux-headers-$(uname -r)",
    },
    {
      tag: "AVALON-MM", color: "var(--trace-cyan)",
      title: "5 × 32-bit registers @ 0xff200000",
      lines: ["BG_CELL · SHAPE_ADDR · SHAPE_DATA0 · SHAPE_DATA1 · SHAPE_COMMIT"],
      note: "lightweight HPS-to-FPGA bridge",
      dark: true,
    },
    {
      tag: "FPGA", color: "var(--zombie-500)",
      title: "shape_table.sv → renderer → VGA",
      lines: ["shadow / active register file", "scanline rasterizer · 25 MHz pixel clock", "linebuffer · palette · VGA out"],
      note: "(see Hardware tab)",
    },
  ];
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 14 }}>
      {layers.map((l, i) => (
        <div key={l.tag} style={{
          background: l.dark ? "var(--silicon-900)" : "#fff",
          color: l.dark ? "var(--ink-50)" : "var(--ink-900)",
          border: "2px solid var(--ink-900)",
          boxShadow: "var(--shadow-pixel-1)",
          display: "grid", gridTemplateColumns: "120px 1fr auto", alignItems: "center",
          padding: 16, gap: 18,
        }}>
          <div style={{
            fontFamily: "var(--font-press)", fontSize: 11, letterSpacing: "0.08em",
            background: l.color, color: l.tag === "AVALON-MM" ? "var(--ink-900)" : "#fff",
            border: "2px solid var(--ink-900)",
            padding: "6px 9px", textAlign: "center",
          }}>{l.tag}</div>
          <div>
            <div style={{ fontWeight: 700, fontSize: 16, marginBottom: 6 }}>{l.title}</div>
            <div style={{ fontFamily: "var(--font-mono)", fontSize: 12, color: l.dark ? "var(--silicon-200)" : "var(--ink-600)", lineHeight: 1.7 }}>
              {l.lines.map((line, j) => <div key={j}>{line}</div>)}
            </div>
          </div>
          <div style={{
            fontFamily: "var(--font-mono)", fontSize: 11,
            color: l.dark ? "var(--silicon-400)" : "var(--ink-500)",
            maxWidth: 200, textAlign: "right", lineHeight: 1.5,
          }}>// {l.note}</div>
        </div>
      ))}
    </div>
  );
}

/* ───────── const cards ───────── */
function ConstCard({ k, v, desc }) {
  return (
    <div style={{
      background: "#fff", border: "2px solid var(--ink-900)",
      boxShadow: "var(--shadow-pixel-1)", padding: "16px 18px",
    }}>
      <div style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--zombie-700)" }}>#define {k}</div>
      <div style={{ fontFamily: "var(--font-term)", fontSize: 30, lineHeight: 1.1, color: "var(--ink-900)", margin: "8px 0 6px" }}>{v}</div>
      <div style={{ fontSize: 12, color: "var(--ink-600)", lineHeight: 1.55 }}>{desc}</div>
    </div>
  );
}

/* ───────── ioctl rows ───────── */
function IoctlRow({ code, arg, desc }) {
  return (
    <div style={{
      background: "var(--silicon-800)", border: "2px solid var(--silicon-600)",
      padding: "12px 14px",
    }}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", gap: 10, flexWrap: "wrap" }}>
        <code style={{ fontFamily: "var(--font-mono)", fontSize: 13, color: "var(--trace-cyan)" }}>{code}</code>
        <code style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--silicon-400)" }}>{arg}</code>
      </div>
      <div style={{ fontSize: 12, color: "var(--silicon-100)", marginTop: 6, lineHeight: 1.55 }}>{desc}</div>
    </div>
  );
}

/* ───────── z-order table ───────── */
function ZOrderTable() {
  const rows = [
    ["0–31",   "Plants",   "var(--lawn-600)",  "4 × 8 grid; visible iff occupied"],
    ["32–36",  "Zombies",  "var(--zombie-500)", "5 entries, one per zombie slot"],
    ["37–44",  "Peas",     "var(--lawn-300)",  "up to 8 active projectiles"],
    ["45",     "Cursor",   "var(--sun-500)",   "yellow box at cursor cell"],
    ["46–47",  "HUD digits","var(--ink-900)",  "sun counter, 7-seg digits"],
  ];
  return (
    <div style={{ marginTop: 18, border: "2px solid var(--ink-900)", boxShadow: "var(--shadow-pixel-1)" }}>
      <div style={{
        background: "var(--silicon-900)", color: "var(--trace-cyan)",
        padding: "8px 12px", fontFamily: "var(--font-mono)", fontSize: 11,
        borderBottom: "2px solid var(--ink-900)",
      }}>// shape table — 48 entries, painter's order</div>
      <table style={{ width: "100%", borderCollapse: "collapse", fontFamily: "var(--font-mono)", fontSize: 12, background: "#fff" }}>
        <tbody>
          {rows.map(([range, name, color, desc], i) => (
            <tr key={range} style={{ background: i % 2 ? "var(--ink-50)" : "#fff" }}>
              <td style={{ padding: "8px 12px", color: "var(--zombie-700)", borderBottom: "1px solid var(--ink-200)", width: 90 }}>{range}</td>
              <td style={{ padding: "8px 12px", borderBottom: "1px solid var(--ink-200)", color: "var(--ink-900)", fontWeight: 600 }}>
                <span style={{
                  display: "inline-block", width: 10, height: 10, marginRight: 8,
                  background: color, border: "1px solid var(--ink-900)",
                }}/>{name}
              </td>
              <td style={{ padding: "8px 12px", borderBottom: "1px solid var(--ink-200)", color: "var(--ink-600)" }}>{desc}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

window.Software = Software;
