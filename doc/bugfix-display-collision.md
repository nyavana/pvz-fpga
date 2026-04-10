# Bugfix: display and collision bugs (2026-04-10)

## Summary

We found three bugs after the first successful board run following Milestone 1. All fixes are software-only -- no hardware (SystemVerilog) changes needed.

| Bug | Root Cause | Fix |
|-----|-----------|-----|
| Text rendering broken (digits show as shapes) | Hardware draw loop uses `s_w` as pixel width bound, but SHAPE_DIGIT encodes the digit value in `s_w[3:0]`. Passing digit 0-9 as `w` meant the draw loop iterated only 0-9 pixels instead of the full 20-pixel glyph width | Pass `w = 32 + digit` so `s_w[3:0]` still holds the digit value but the draw loop covers 32 pixels (more than the 20-pixel glyph) |
| Zombies pass through plants | `check_collisions()` only handled pea-zombie hits. No zombie-plant collision code existed at all | Added grid-based collision detection in `update_zombies()`, plus a zombie eating state machine with periodic damage and plant destruction |
| HUD text rendered behind plants | HUD digits used shape indices 1-4, plants used 5-36. The painter's algorithm draws later indices on top, so plants overwrote the HUD | Reordered shape index allocation so plants get the lowest indices and HUD gets the highest |

---

## Changes by file

### `sw/game.h`

- Added `int hp` field to `plant_t` (hit points remaining)
- Added `int eating` and `int eat_timer` fields to `zombie_t` (eating state machine)
- Added constants: `PLANT_HP` (3), `ZOMBIE_EAT_COOLDOWN` (60 frames)

### `sw/game.c`

- `game_place_plant()`: initializes `hp = PLANT_HP` when placing a plant
- `update_spawning()`: initializes `eating = 0` and `eat_timer = 0` when spawning a zombie
- `update_zombies()`: rewritten to include zombie-plant collision detection and eating logic:
  - Each frame, if a zombie is eating, it re-checks that the plant still exists (handles the case where another zombie destroyed it)
  - While eating, the zombie does not move. The `eat_timer` counts down; at zero, the plant takes 1 HP damage and the timer resets
  - When a plant's HP reaches 0, it is removed from the grid and the zombie resumes moving
  - After each movement step, the zombie checks if its grid column contains a plant in the same row; if so, it enters the eating state

### `sw/render.c`

- Shape index reallocation (the z-order fix):
  - Old: cursor=0, HUD=1-4, plants=5-36, zombies=37-41, peas=42-47
  - New: plants=0-31, zombies=32-36, peas=37-42, HUD=43-46, cursor=47
- Digit width fix: `write_shape()` for SHAPE_DIGIT now passes `w = 32 + digit` instead of just `digit`. The hardware draw loop uses `s_w` as the pixel width bound, so this gives it enough pixels to cover the 20-pixel glyph. `s_w[3:0]` still equals the digit value for `decode_7seg()`.
- Updated header comment to match new allocation

### `sw/test/test_game.c`

Added four new unit tests (61 assertions total, up from 49):

| Test | What it verifies |
|------|-----------------|
| `test_zombie_stops_at_plant` | Zombie enters a cell with a plant, starts eating, and stops moving |
| `test_zombie_eats_and_destroys_plant` | Zombie deals damage over time, plant is removed when HP reaches 0 |
| `test_zombie_resumes_after_eating` | Zombie clears eating state and resumes moving when the plant it was eating is gone |
| `test_two_zombies_eat_same_plant` | Two zombies eat the same plant simultaneously; both resume after it is destroyed |

---

## Root cause analysis

### Bug 1: text rendering -- draw loop width mismatch

The hardware rendering pipeline in `shape_renderer.sv` iterates pixels from `draw_x = s_x` to `draw_x = s_x + s_w` for every shape type. For SHAPE_DIGIT, the digit value (0-9) is encoded in `s_w[3:0]` and passed to `decode_7seg()` to get the segment bitmask. But the same `s_w` field is also used as the pixel width bound.

When the software passed `w = digit_value` (e.g., 5 for the digit "5"), the draw loop only iterated 5 pixels. The 7-segment glyph is 20 pixels wide, so most of it was clipped. Digit "0" produced zero pixels (invisible), and all other digits were severely truncated, making them look like random shapes.

We now pass `w = 32 + digit`. Since `32 & 0xF == 0`, the digit value is preserved in the low 4 bits (`s_w[3:0]`), but the draw loop iterates 32+ pixels -- more than enough for the 20-pixel glyph. `check_7seg()` returns no-hit for `local_x >= 20`, so pixels beyond the glyph are transparent.

### Bug 2: no zombie-plant collision

The original `update_zombies()` just decremented `x_pixel` every `ZOMBIE_SPEED_FRAMES` frames. The only check was the lose condition (`x_pixel <= 0`). `check_collisions()` only tested pea-zombie overlap. There was no zombie-plant interaction at all.

Each zombie now has two states: moving and eating. When its grid column (`x_pixel / CELL_WIDTH`) contains a plant in the same row, it transitions to eating. While eating, the zombie stays put and deals 1 HP damage every `ZOMBIE_EAT_COOLDOWN` (60) frames. Plant hits 0 HP, gets removed, zombie goes back to moving.

### Bug 3: HUD z-order

The shape renderer processes entries 0 through 47 in order; later entries overwrite earlier ones. The original allocation put HUD at indices 1-4 and plants at 5-36, so plants drew on top of the digits. Swapping the allocation (plants at the bottom, HUD near the top) fixes it.

---

## What to do next

### Board verification (required before closing this bugfix)

1. Push these changes to trigger CI, or cross-compile locally
2. On the DE1-SoC, deploy and run:
   ```bash
   ./deploy.sh download
   ./deploy.sh install
   ./deploy.sh run
   ```
3. Verify on the VGA monitor:
   - Sun counter digits are readable 7-segment numbers at the top of the screen
   - HUD digits are visible even when plants are placed in the top-left grid cells
   - Cursor highlight is visible over everything
   - Zombies stop when they reach a plant and visually stay in place
   - After a few seconds of eating, the plant disappears
   - After eating a plant, the zombie resumes moving left

### Known limitations

- No visual eating feedback. The zombie just stops; there's no animation or color change. Could flash the plant color or add a health bar later.
- Zombie-plant collision uses integer grid lookup. If a zombie's `x_pixel` lands exactly on a grid column boundary, it may start eating one frame before visually overlapping the plant. Looks slightly off but works correctly.
- Plant HP is not displayed. The player can't tell how much HP a plant has left. One option: change the plant color as it takes damage (green -> yellow -> red).
- Only peashooters have HP. When other plant types are added, they'll need their own HP values. `PLANT_HP` is shared for now.

### Next steps for Milestone 2

1. Sprites -- replace primitive shapes with bitmap sprites (see proposal and bubble-bobble reference design)
2. Audio -- sound effects for shooting, eating, and win/lose via the Wolfson codec
3. USB gamepad -- replace keyboard input with USB HID gamepad polling via libusb
4. More plant types -- Sunflower (generates extra sun), Wall-nut (high HP, no attack)
5. Visual polish -- plant damage indication, zombie eating animation, sun drop animation
