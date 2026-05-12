# 07 — End-to-end frame walkthrough

This chapter answers: "When the player presses the A button on the
gamepad to place a peashooter, what code runs in what order to make a
peashooter appear at that grid cell?"

![Frame flow](diagrams/07-frame-flow.svg)

The scenario: the cursor is currently at row 1, column 3. Sun count is
≥ 50. The selected plant is the peashooter. The player presses the A
button on an Xbox 360 gamepad plugged into the board's USB port.

## Step 1 — USB → kernel evdev

The xpad kernel driver (already loaded by the Linux distribution we
boot) sees the button-down report from the gamepad and posts a
`struct input_event` to `/dev/input/event0`. The event payload is
roughly:

```
type  = EV_KEY
code  = BTN_SOUTH   (== A button on Xbox layout)
value = 1           (press, not release)
```

This is all kernel-side, outside our codebase. The HPS schedules our
process whenever it gets CPU time.

## Step 2 — `input_poll` returns `INPUT_SPACE`

In our main loop, `sw/main.c:114` calls `process_input(&gs)`. That
calls `input_poll()` (`sw/input.c:31`), which does a non-blocking
`read` from the input fd. The `EV_KEY` event for `BTN_SOUTH` matches
the switch case at `sw/input.c:62`:

```c
case KEY_SPACE: case BTN_SOUTH:  return INPUT_SPACE;
```

so the function returns `INPUT_SPACE`. Note that on the keyboard, the
**Space key** triggers the same path — A on a gamepad and Space on
a keyboard are interchangeable by construction.

## Step 3 — `process_input` calls `game_place_plant`

Back in `sw/main.c:38-67`, the switch on the input code dispatches:

```c
case INPUT_SPACE:
    game_place_plant(gs);
    break;
```

`game_place_plant` (`sw/game.c:34-51`) reads the cursor position
(row 1, col 3), checks that `grid[1][3].type == PLANT_NONE` and
`sun >= 50`. Assuming both pass, it sets:

```c
gs->grid[1][3].type           = PLANT_PEASHOOTER;
gs->grid[1][3].fire_cooldown  = 120;
gs->grid[1][3].hp             = 3;
gs->sun                      -= 50;
```

Notice no rendering has happened yet — this only mutates the in-memory
`game_state_t` struct on the HPS.

## Step 4 — `game_update` runs the simulation tick

`sw/main.c:119` then calls `game_update(&gs)` (`sw/game.c:288`).
This runs the seven per-frame functions in order (sun, spawning,
firing, projectiles, zombies, collisions, win check). For the new
peashooter at (1, 3):

- `update_firing` (`game.c:175-192`) finds a peashooter at row 1
  col 3 with `fire_cooldown = 120`. There's a zombie in row 1
  (let's say). It decrements `fire_cooldown` to 119. Cooldown is
  still positive so nothing fires this frame.

None of the other helpers change anything at this cell. After this
call, `gs` reflects the world state after one tick of simulation.

## Step 5 — `render_frame` pushes the new scene to hardware

`sw/main.c:122` calls `render_frame(&gs)` (`sw/render.c:108-124`).
Since `state == STATE_PLAYING`, it walks through every renderer
helper:

`render_plants` (`render.c:32-47`) loops over all 32 grid cells.
When it hits row 1, col 3 with `type == PLANT_PEASHOOTER`, it sets
bit `(1 * 8 + 3) = bit 11` in `pea_bits`. After the full pass it
calls `write_reg(PVZ_REG_PLANTS, pea_bits)`, which builds:

```c
pvz_write_arg_t w = { .word_index = 0, .value = pea_bits };
ioctl(fd, PVZ_WRITE_REG, &w);
```

## Step 6 — kernel driver issues `iowrite32` to the bridge

The ioctl traps into the kernel. `pvz_ioctl` (`sw/pvz_driver.c:30`)
runs:

```c
copy_from_user(&w, ...);              // pull arg from userspace
iowrite32(w.value, dev.virtbase + w.word_index * 4);
                                      // word_index = 0 → byte offset 0
```

`dev.virtbase` is the kernel-virtual address of the FPGA peripheral,
established by `of_iomap` at module-probe time
(`pvz_driver.c:83`). The CPU's store turns into a write transaction
on the lightweight HPS-to-FPGA bridge, addressed to byte 0 of the
peripheral.

## Step 7 — Avalon bus delivers the write to `pvz_top`

The bridge converts the byte address into Avalon's word units
(`addressUnits = WORDS`, `pvz_top_hw.tcl:52`) and presents the
slave with:

```
address    = 6'd0
writedata  = pea_bits
write      = 1
chipselect = 1
```

`pvz_top.sv:114-116` matches `address == 6'd0` and latches:

```systemverilog
plant_present <= writedata;
```

On the next 50 MHz clock edge the register file has the new value.
`plant_present` is wired straight into `entity_drawer` via
`pvz_top.sv:211`. Nothing buffers this — there is no shadow / active
register on this branch.

## Step 8 — `entity_drawer` renders the new sprite as the beam scans

The VGA beam doesn't care about any of the above and is in the
middle of scanning, well, some line. The next time `(px, py)` lands
inside cell (1, 3) — i.e. `px ∈ [64+3*64, 64+4*64) = [256, 320)` and
`py ∈ [112+1*64, 112+2*64) = [176, 240)` — the following happens in
two clock stages:

**Stage 1** (`entity_drawer.sv:144-166`):

```
gx       = 256 .. 319  (px - GRID_X)
gy       = 176 .. 239  (py - GRID_Y)
cell_col = gx[8:6]     = 3
cell_row = gy[7:6]     = 1
in_cell_x = gx[5:0]    = 0 .. 63
in_cell_y = gy[5:0]    = 0 .. 63
plant_idx = {cell_row, cell_col} = 5'b01011 = 11
plant_here = plant_present[11] = 1
plant_rd_addr = {in_cell_y, in_cell_x}
```

The address goes into the peashooter sprite ROM
(`pvz_top.sv:179-183`).

**Stage 2** (`entity_drawer.sv:282-358`): one clock later, the ROM
has delivered the pixel and the registered `plant_here_d` is high.
The mux at line 332 fires:

```systemverilog
if (plant_here_d && plant_rd_pixel != COL_TRANSPARENT)
    color_out = plant_rd_pixel;
```

`color_out` rolls out the side of `entity_drawer`, into
`color_palette`, which translates the 8-bit index into 24-bit RGB
(`pvz_top.sv:236-241`). Outside of blanking, that RGB drives the
board's VGA DAC pins (`pvz_top.sv:243-253`).

## Step 9 — pixel reaches the monitor

The VGA DAC turns the 24-bit value into analog R/G/B voltages on the
board's VGA connector. The monitor latches them as the part of the
current line it's painting. After 16.667 ms, the player sees a new
peashooter at (row 1, col 3).

## Step 10 — main loop sleeps and repeats

Back on the HPS, `render_frame` has continued past `render_plants`
through `render_zombies`, `render_peas`, `render_cursor`,
`render_sun`, `render_selected` (`render.c:111-116`), firing about
20 more ioctls. Total cost is dozens of microseconds.

`sw/main.c:147-149` computes how long the frame took and `usleep`s
the remainder of the 16.667 ms budget. The loop body runs again at
the next 60 Hz tick.

## What's actually parallel here

It's easy to mentally serialize this whole walkthrough, but most of
it is happening in parallel:

- The HPS runs steps 2–6 once per frame, in a roughly 100 µs burst.
- The Avalon bus is sitting idle 99% of the time.
- The FPGA runs steps 7–9 continuously — `entity_drawer` produces a
  new pixel every two 50 MHz clocks, 12.6 million pixels per second.
  The register write from step 7 is just one signal change that the
  next pixel happens to see.

The HPS does not wait for the FPGA at any point. The FPGA does not
wait for the HPS either; it just reads whatever is in the register
file. This decoupling is why the protocol works without any
handshake.
