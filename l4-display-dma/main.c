// L4 — Display and DMA: a self-test for the EP-0172 ST7796 panel.
// Drives the real panel and cycles a visual test pattern while reporting DMA
// fill throughput, so you see both that the display works *and* how fast DMA
// pushes pixels.
//
// LOGGING SWITCH
//   printf over serial takes real time, which can skew timing you are trying to
//   measure (and clutters a logic-analyzer session). So all narration goes
//   through LOGF(), gated by LOG:
//     LOG 1  -> print the throughput numbers (default; good for understanding)
//     LOG 0  -> silent, for clean timing (measure with a GPIO toggle + analyzer)
//   Note the fill timing itself is always honest: we bracket time_us_64() around
//   *only* the fill, and print afterwards — so the MB/s figure is accurate even
//   with LOG on. LOG mainly controls the serial chatter and the loop cadence.
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"

#define LOG 1
#if LOG
#define LOGF(...) printf(__VA_ARGS__)
#else
#define LOGF(...) ((void)0)
#endif

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);   // unbuffered: flush every printf immediately
    sleep_ms(2000);                     // let the serial host attach
    LOGF("L4 ST7796 display self-test (LOG=%d)\n", LOG);

    st7796_init();
    LOGF("init done: %dx%d, SPI0 @ GP2/3, CS5 DC6 RST7\n", ST7796_WIDTH, ST7796_HEIGHT);

    const uint16_t cyc[]   = { ST_RED, ST_GREEN, ST_BLUE, ST_WHITE, ST_BLACK };
    const char    *names[] = { "RED", "GREEN", "BLUE", "WHITE", "BLACK" };
    const uint16_t bars[]  = { ST_RED, ST_GREEN, ST_BLUE, ST_YELLOW,
                               ST_CYAN, ST_MAGENTA, ST_WHITE, ST_BLACK };
    const uint32_t scr_bytes = (uint32_t)ST7796_WIDTH * ST7796_HEIGHT * 2;  // one full frame

    while (true) {
        // 1) Solid-colour cycle. Time only the fill; print the throughput after.
        for (int i = 0; i < 5; i++) {
            uint64_t t0 = time_us_64();
            st7796_fill_screen(cyc[i]);
            uint32_t us = (uint32_t)(time_us_64() - t0);
            LOGF("fill %-5s  %5lu us  %.1f MB/s\n",
                 names[i], (unsigned long)us, scr_bytes / (float)us);
            sleep_ms(400);
        }

        // 2) Eight vertical colour bars (addressing check across the width).
        int bw = ST7796_WIDTH / 8;
        for (int b = 0; b < 8; b++)
            st7796_fill_rect(b * bw, 0, bw, ST7796_HEIGHT, bars[b]);
        LOGF("colour bars\n");
        sleep_ms(1500);

        // 3) Vertical gradient (red at top -> blue at bottom), drawn in strips.
        for (int y = 0; y < ST7796_HEIGHT; y += 4) {
            uint8_t t = (uint8_t)(255 * y / ST7796_HEIGHT);
            st7796_fill_rect(0, y, ST7796_WIDTH, 4, RGB565(255 - t, 0, t));
        }
        LOGF("gradient\n");
        sleep_ms(1500);

        // 4) LIVE THROUGHPUT: flip the whole screen as fast as the bus allows for
        //    ~2 s, with NO work in the loop, and report sustained frames/s + MB/s.
        //    Nothing prints inside the measured window, so the number is clean.
        uint64_t t0 = time_us_64();
        uint32_t frames = 0;
        while (time_us_64() - t0 < 2000000) {
            st7796_fill_screen((frames & 1) ? ST_BLACK : ST_WHITE);
            frames++;
        }
        uint32_t us = (uint32_t)(time_us_64() - t0);
        float fps = frames * 1e6f / us;
        LOGF("[throughput] %lu full-screen flips in %lu ms -> %.1f fps, %.1f MB/s\n",
             (unsigned long)frames, (unsigned long)(us / 1000),
             fps, fps * scr_bytes / 1e6f);
    }
}
