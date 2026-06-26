# L4 — Display and DMA

Demo target: drive the carrier's 3.5" 320×480 TFT (ST7796 over SPI) and move a
framebuffer to it with DMA, measuring throughput.

## What builds today

`l4_spi_dma` (`main.c`) — a **compiling SPI + DMA throughput demo**. It sets up a
DMA channel paced by the SPI TX DREQ, bursts a buffer out, and prints the
transfer time + Mbit/s as CSV. This exercises the real L4 *mechanism* (background
data movement, CPU free during transfer) and is measurable with a logic analyzer.

## Still TODO before it drives the real panel

1. **Confirmed wiring.** The EP-0172 TFT pins (SPI SCK/MOSI/CS/DC/RST + the I²C
   touch controller) are **not yet confirmed** — `PIN_SCK`/`PIN_MOSI` in `main.c`
   are placeholders. Verify against the 52Pi EP-0172 wiki.
2. **An ST7796 driver.** Add the init sequence + command/data (DC pin) handling,
   then push a real framebuffer instead of the test pattern.

Build it: `./run.sh flash l4_spi_dma` (from `src/es201/`).
