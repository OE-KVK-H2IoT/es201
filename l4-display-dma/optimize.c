// L4 — Display optimization: full redraw vs dirty rectangles vs double buffer.
// ===========================================================================
// THE QUESTION: a full-screen fill is ~78 ms (bus-bound at the granted SPI
// clock), so repainting everything every frame caps you near ~13 fps and tears.
// What actually fixes it? This demo lets you compare three render strategies and
// MEASURE each (live fps), instead of guessing.
//
//   FULL   — clear the whole screen, draw the box. Simple; slow; flickers+tears.
//   DIRTY  — erase only the box's old footprint, draw the new one. ~50x less
//            data -> hundreds of fps, no flicker, far less tearing. The real win.
//   DBUF   — render the frame into a host-RAM framebuffer first, then blit the
//            WHOLE frame to the panel. Teaches a subtle truth: on a smart panel
//            (GRAM scanned out by the controller, no TE pin wired here) host
//            double buffering does NOT cure tearing — the blit still races the
//            scan — and it costs 300 KB of SRAM and a full-frame push, so it is
//            ~as slow as FULL. Off-screen rendering only hides the host's own
//            half-drawn frames; only TE/VSync removes scan tearing.
//
// OPTIONS (compile-time): change these, rebuild, measure — that is the lab loop.
//   ENABLE_DBUF : 1 = compile in the double-buffer mode + its 300 KB framebuffer.
//   START_MODE  : which strategy to start in (0 FULL, 1 DIRTY, 2 DBUF).
// K1 cycles the strategy live; K2 clears. LOG gates the serial chatter.
// ===========================================================================
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"

// These are overridable from the build command (run.sh ... KEY=VAL, or
// cmake -DKEY=VAL), so the #ifndef guards keep the in-file value as the default.
#ifndef LOG
#define LOG          1
#endif
#ifndef ENABLE_DBUF
#define ENABLE_DBUF  1     // set 0 to drop the double-buffer mode and save 300 KB SRAM
#endif
#ifndef START_MODE
#define START_MODE   0     // 0 = FULL, 1 = DIRTY, 2 = DBUF
#endif

#if LOG
#define LOGF(...) printf(__VA_ARGS__)
#else
#define LOGF(...) ((void)0)
#endif

#define BOX        40
#define BTN_MODE   14      // K1 (active-low): cycle render strategy
#define BTN_CLEAR  15      // K2 (active-low): clear

enum { MODE_FULL = 0, MODE_DIRTY = 1, MODE_DBUF = 2 };
#if ENABLE_DBUF
#define N_MODES 3
static uint16_t fb[ST7796_WIDTH * ST7796_HEIGHT];   // host framebuffer (300 KB)
#else
#define N_MODES 2
#endif
static const char *mode_name[] = { "FULL redraw", "DIRTY rect", "DOUBLE buffer" };

static void button_init(uint pin) {
    gpio_init(pin); gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin);
}

#if ENABLE_DBUF
// Draw a rectangle into the host framebuffer (plain memory writes — fast).
static void fb_rect(int x, int y, int w, int h, uint16_t color) {
    for (int yy = y; yy < y + h; yy++) {
        if (yy < 0 || yy >= ST7796_HEIGHT) continue;
        for (int xx = x; xx < x + w; xx++) {
            if (xx < 0 || xx >= ST7796_WIDTH) continue;
            fb[yy * ST7796_WIDTH + xx] = color;
        }
    }
}
#endif

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(2000);
    LOGF("\nL4 display optimization — K1 cycles strategy, K2 clears (%d modes)\n", N_MODES);

    st7796_init();
    button_init(BTN_MODE);
    button_init(BTN_CLEAR);

    uint32_t frame_bytes = (uint32_t)ST7796_WIDTH * ST7796_HEIGHT * 2;
    LOGF("SPI granted = %lu Hz -> full-frame ceiling ~%.1f fps (%lu bytes/frame)\n",
         (unsigned long)st7796_spi_hz(), st7796_spi_hz() / 8.0f / frame_bytes,
         (unsigned long)frame_bytes);
#if ENABLE_DBUF
    LOGF("double-buffer mode ON: host framebuffer = %lu bytes of SRAM\n",
         (unsigned long)sizeof(fb));
#endif

    const uint16_t bg = ST_BLACK, box = ST_CYAN;
    st7796_fill_screen(bg);

    int x = 60, y = 80, dx = 3, dy = 4, ox = x, oy = y;
    int mode = START_MODE;
    LOGF("[mode] %s\n", mode_name[mode]);

    uint64_t rate_t0 = time_us_64();
    uint32_t frames = 0;
    bool m_last = false, c_last = false;

    while (true) {
        // --- buttons ---
        bool m = !gpio_get(BTN_MODE), c = !gpio_get(BTN_CLEAR);
        if (m && !m_last) {
            mode = (mode + 1) % N_MODES;
            st7796_fill_screen(bg);
            LOGF("[mode] %s\n", mode_name[mode]);
        }
        if (c && !c_last) { st7796_fill_screen(bg); LOGF("[K2] clear\n"); }
        m_last = m; c_last = c;

        // --- move the box, bouncing off the edges ---
        ox = x; oy = y;
        x += dx; y += dy;
        if (x < 0 || x + BOX >= ST7796_WIDTH)  { dx = -dx; x += dx; }
        if (y < 0 || y + BOX >= ST7796_HEIGHT) { dy = -dy; y += dy; }

        // --- render with the selected strategy ---
        if (mode == MODE_DIRTY) {
            st7796_fill_rect(ox, oy, BOX, BOX, bg);     // erase only what we vacated
            st7796_fill_rect(x,  y,  BOX, BOX, box);    // draw only what's new
        }
#if ENABLE_DBUF
        else if (mode == MODE_DBUF) {
            fb_rect(0, 0, ST7796_WIDTH, ST7796_HEIGHT, bg);  // render off-screen (fast RAM)
            fb_rect(x, y, BOX, BOX, box);
            st7796_blit(0, 0, ST7796_WIDTH, ST7796_HEIGHT, fb);  // ...then push the WHOLE frame
        }
#endif
        else {  // MODE_FULL
            st7796_fill_screen(bg);
            st7796_fill_rect(x, y, BOX, BOX, box);
        }

        // --- live fps so each strategy's cost is a number ---
        frames++;
        uint64_t now = time_us_64();
        if (now - rate_t0 >= 1000000) {
            LOGF("[fps] %-13s: %lu fps\n", mode_name[mode], (unsigned long)frames);
            rate_t0 = now; frames = 0;
        }
    }
}
