// L4 — Display optimization: "only refresh what is necessary".
// ===========================================================================
// THE PROBLEM
//   A full-screen fill on this panel is ~78 ms (≈3.9 MB/s over SPI), so redrawing
//   the WHOLE screen every frame caps you at ~12 fps and tends to TEAR (you are
//   still pushing pixels when the panel scans them out — see Reference §7).
//
// THE FIX: DIRTY RECTANGLES
//   Almost nothing changes between frames. A bouncing box only vacates one small
//   area and enters another. So instead of repainting 320x480 = 153,600 pixels,
//   repaint only the two small rectangles that actually changed: erase the box's
//   OLD footprint, draw it at the NEW spot. ~3,200 pixels instead of 153,600 —
//   roughly 50x less data, 50x more fps, and far less to tear.
//
// THE BIG IDEA (why this is worth learning)
//   "Send only what changed" is the same trick everywhere in graphics:
//     * Video codecs send P-frames — only the macroblocks that moved, not whole frames.
//     * GUI/Wayland/DRM track "damage" regions and repaint only those.
//     * Our solid fill is already run-length encoding in disguise: DMA streams one
//       colour value N times instead of N copies (a count + a value = RLE).
//   Different problems, one idea: exploit what stayed the same. This demo lets you
//   FEEL it — press K1 to toggle full-redraw vs dirty-rect and watch the fps line.
//
//   K1 toggles the mode. K2 clears. LOG gates the serial chatter.
// ===========================================================================
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"

#define LOG 1
#if LOG
#define LOGF(...) printf(__VA_ARGS__)
#else
#define LOGF(...) ((void)0)
#endif

#define BOX        40   // box side in pixels
#define BTN_MODE   14   // K1 (active-low): toggle full-redraw <-> dirty-rect
#define BTN_CLEAR  15   // K2 (active-low): clear

static void button_init(uint pin) {
    gpio_init(pin); gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin);
}

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(2000);
    LOGF("\nL4 display optimization — K1 toggles FULL-redraw vs DIRTY-rect\n");
    LOGF("Watch fps: full redraw repaints 320x480 every frame; dirty repaints two %dx%d rects.\n\n",
         BOX, BOX);

    st7796_init();
    button_init(BTN_MODE);
    button_init(BTN_CLEAR);

    // The honest ceiling: a full frame is W*H*2 bytes; at the SPI clock the
    // hardware ACTUALLY granted (often less than requested) that sets a hard
    // max fps no amount of code cleverness can beat. Measure it, don't assume it.
    uint32_t frame_bytes = (uint32_t)ST7796_WIDTH * ST7796_HEIGHT * 2;
    LOGF("SPI granted = %lu Hz -> full-frame ceiling ~%.1f fps (%lu bytes/frame)\n",
         (unsigned long)st7796_spi_hz(),
         st7796_spi_hz() / 8.0f / frame_bytes, (unsigned long)frame_bytes);

    const uint16_t bg = ST_BLACK, box = ST_CYAN;
    st7796_fill_screen(bg);

    int x = 60, y = 80, dx = 3, dy = 4;     // box position + velocity
    int ox = x, oy = y;                      // previous position (its old footprint)
    int dirty = 0;                           // 0 = full redraw, 1 = dirty rectangles

    uint64_t rate_t0 = time_us_64();
    uint32_t frames = 0;
    bool m_last = false, c_last = false;

    while (true) {
        // --- buttons ---
        bool m = !gpio_get(BTN_MODE), c = !gpio_get(BTN_CLEAR);
        if (m && !m_last) {
            dirty = !dirty;
            st7796_fill_screen(bg);          // repaint once so the modes start clean
            LOGF("[mode] %s\n", dirty ? "DIRTY rectangles (repaint only what changed)"
                                       : "FULL redraw (repaint the whole screen)");
        }
        if (c && !c_last) { st7796_fill_screen(bg); LOGF("[K2] clear\n"); }
        m_last = m; c_last = c;

        // --- move the box, bouncing off the edges ---
        ox = x; oy = y;
        x += dx; y += dy;
        if (x < 0 || x + BOX >= ST7796_WIDTH)  { dx = -dx; x += dx; }
        if (y < 0 || y + BOX >= ST7796_HEIGHT) { dy = -dy; y += dy; }

        // --- render: the whole point of the demo ---
        if (dirty) {
            // Touch only what changed: erase the old footprint, draw the new one.
            st7796_fill_rect(ox, oy, BOX, BOX, bg);
            st7796_fill_rect(x,  y,  BOX, BOX, box);
        } else {
            // Brute force: repaint everything, every frame.
            st7796_fill_screen(bg);
            st7796_fill_rect(x, y, BOX, BOX, box);
        }

        // --- live fps so the cost of each strategy is a number, not a vibe ---
        frames++;
        uint64_t now = time_us_64();
        if (now - rate_t0 >= 1000000) {
            LOGF("[fps] %s: %lu fps\n", dirty ? "dirty" : "full ", (unsigned long)frames);
            rate_t0 = now; frames = 0;
        }
    }
}
