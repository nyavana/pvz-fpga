# Repo name suggestions

`4840-final-project` doesn't tell anyone what this actually is. We should rename it before it goes on our portfolios. Here are some options.

## What we're naming

PvZ clone on the DE1-SoC. FPGA does VGA rendering (shape engine, linebuffers, palette), HPS runs the game loop in C, kernel driver bridges the two over Avalon registers. The name should work on a GitHub profile without needing someone to click through to the README.

## Options

### Descriptive / technical

Says what it is. Clear, but reads like a course project.

| Name | Reasoning |
|------|-----------|
| `pvz-fpga` | Does what it says |
| `pvz-de1-soc` | Specific to our board |
| `pvz-cyclone` | References the Cyclone V family |
| `pvz-on-chip` | Emphasizes hardware implementation |

### Playful / punny

Plays on the overlap between plants/zombies and digital hardware. More memorable, though you need the repo description to fill in context.

| Name | Reasoning |
|------|-----------|
| `silicon-garden` | Silicon (chip) + garden (PvZ lawn). Short and easy to search for |
| `garden-of-gates` | Garden + logic gates |
| `field-programmable-garden` | Riff on "Field Programmable Gate Array" |
| `lawn-of-the-dead` | PvZ lawn + Dawn of the Dead |
| `gate-keepers` | Logic gates + tower defense |

### Short and brandable

Standalone names. Punchy, but the README has to do the explaining.

| Name | Reasoning |
|------|-----------|
| `turfwar` | Turf/lawn + war. Compact |
| `peagate` | Peashooter + logic gate |
| `sunblitz` | Sun economy + fast action |

### Mix (descriptive with personality)

Tries to be both memorable and descriptive.

| Name | Reasoning |
|------|-----------|
| `cyclone-garden` | Cyclone V + PvZ garden |
| `silicon-lawn-defense` | Hardware + gameplay together |
| `fpga-lawn-defense` | More explicit version |

## Recommendation

Purely descriptive names blend in with every other course project repo. Purely brandable names need someone to click the README to know what they're looking at. The playful or mixed names land somewhere better for a portfolio: they're interesting enough to click on, and "silicon" or "gates" or "cyclone" tells a technical reader there's hardware involved.

Favorites:

1. `silicon-garden` - easy to remember, "silicon" reads as hardware
2. `cyclone-garden` - more specific (Cyclone V is a known FPGA family)
3. `garden-of-gates` - "gates" is a pretty direct hardware reference

Whatever we go with, pair it with a one-liner like:

> Plants vs Zombies on FPGA: hardware sprite engine, VGA rendering, and Linux game loop on DE1-SoC (Cyclone V)

## Vote

Pick your favorite or suggest something else.
