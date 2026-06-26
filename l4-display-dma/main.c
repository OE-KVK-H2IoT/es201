// L4 — Display and DMA: a self-test for the EP-0172 ST7796 panel.
// Drives the real panel (pins confirmed from the 52Pi wiki) and cycles through
// a visual test pattern, reporting DMA fill throughput over USB-serial so you
// can see both that the display works *and* how fast DMA pushes pixels.
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);   // unbuffered: flush every printf immediately
    sleep_ms(2000);                 // let the USB-serial host attach
    printf("L4 ST7796 display self-test\n");

    st7796_init();
    printf("init done: %dx%d, SPI0 @ GP2/3, CS5 DC6 RST7\n",
           ST7796_WIDTH, ST7796_HEIGHT);

    const uint16_t cyc[]   = { ST_RED, ST_GREEN, ST_BLUE, ST_WHITE, ST_BLACK };
    const char    *names[] = { "RED", "GREEN", "BLUE", "WHITE", "BLACK" };
    const uint16_t bars[]  = { ST_RED, ST_GREEN, ST_BLUE, ST_YELLOW,
                               ST_CYAN, ST_MAGENTA, ST_WHITE, ST_BLACK };
    const uint32_t scr_bytes = (uint32_t)ST7796_WIDTH * ST7796_HEIGHT * 2;

    while (true) {
        // 1) Solid-colour cycle with throughput measurement.
        for (int i = 0; i < 5; i++) {
            uint64_t t0 = time_us_64();
            st7796_fill_screen(cyc[i]);
            uint32_t us = (uint32_t)(time_us_64() - t0);
            printf("fill %-5s  %5lu us  %.1f MB/s\n",
                   names[i], (unsigned long)us, scr_bytes / (float)us);
            sleep_ms(400);
        }

        // 2) Eight vertical colour bars.
        int bw = ST7796_WIDTH / 8;
        for (int b = 0; b < 8; b++)
            st7796_fill_rect(b * bw, 0, bw, ST7796_HEIGHT, bars[b]);
        printf("colour bars\n");
        sleep_ms(1500);

        // 3) Vertical gradient (red at top -> blue at bottom), drawn in strips.
        for (int y = 0; y < ST7796_HEIGHT; y += 4) {
            uint8_t t = (uint8_t)(255 * y / ST7796_HEIGHT);   // 0..255 down the screen
            uint16_t color = RGB565(255 - t, 0, t);
            st7796_fill_rect(0, y, ST7796_WIDTH, 4, color);
        }
        printf("gradient\n");
        sleep_ms(1500);
    }
}
