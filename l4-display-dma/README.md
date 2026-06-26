# L4 — Display and DMA

Full self-test for the 52Pi EP-0172's 3.5" 320×480 **ST7796** SPI panel, with DMA
pixel streaming.

## What it does

`l4_display` (`main.c` + `st7796.c`) drives the real panel and loops a visual
test pattern, narrating over USB-serial:

1. **Solid-colour cycle** (red/green/blue/white/black) — each full-screen fill is
   DMA-streamed and its **throughput is printed** (MB/s), the "DMA" half of the lab.
2. **Eight colour bars** — confirms addressing across the width.
3. **Vertical gradient** — many small fills, red→blue down the screen.

If the panel stays dark, nothing draws, or colours look wrong, that's your
debugging exercise — check the pins, reset timing, and the MADCTL/BGR note.

## Pins (confirmed — 52Pi EP-0172 wiki)

| Signal | GPIO | | Signal | GPIO |
|---|---|---|---|---|
| SCK  | GP2 (SPI0) | | DC  | GP6 |
| MOSI | GP3 (SPI0) | | RST | GP7 |
| CS   | GP5 | | Backlight | hardwired on |

> Red/blue swapped? The panel is initialised BGR (`MADCTL 0x48`); flip the BGR bit
> to `0x40` in `st7796_init()`.

Build & flash: `./run.sh flash l4_display` (from `src/es201/`).

## Touch test — `l4_touch`

`touch_test.c` + `gt911.c` add the **GT911 capacitive touch** (I²C0: SDA=GP8,
SCL=GP9, RST=GP10, INT=GP11). It clears the screen white, then draws a dot
wherever you touch and prints `x/y/size` over serial. A red screen at startup
means the controller didn't answer on I²C.

> If the dot lands away from your finger, the touch and display axes differ —
> swap or flip x/y in `touch_test.c` to calibrate (the spot is commented).

Build & flash: `./run.sh flash l4_touch`.
