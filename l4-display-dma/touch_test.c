// L4 (touch) — interactive GT911 test: clear the panel, then draw a dot wherever
// you touch and print the coordinates over USB-serial. Reuses the ST7796 driver.
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"
#include "gt911.h"

#define DOT 6   // half-size of the dot drawn under your finger

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(2000);
    printf("L4 GT911 touch test\n");

    st7796_init();
    st7796_fill_screen(ST_WHITE);

    if (!gt911_init()) {
        printf("!! GT911 did not answer on I2C0 (check GP8/GP9 + reset)\n");
        st7796_fill_screen(ST_RED);     // red screen = touch controller not found
    } else {
        printf("GT911 ready — touch the screen (dots appear; coords on serial)\n");
    }

    gt911_point_t pts[5];
    uint64_t last_beat = time_us_64();
    while (true) {
        int n = gt911_read(pts, 5);
        for (int i = 0; i < n; i++) {
            uint16_t x = pts[i].x, y = pts[i].y;
            printf("touch id=%u  x=%u  y=%u  size=%u\n", pts[i].id, x, y, pts[i].size);
            // Clamp into the panel so a dot always shows, even if the touch
            // controller's coordinate range differs from the display's. Once you
            // know the raw ranges (read them above), calibrate here — e.g. swap
            // x/y, or flip with  y = ST7796_HEIGHT-1 - y.
            uint16_t dx = x < ST7796_WIDTH  ? x : ST7796_WIDTH  - 1;
            uint16_t dy = y < ST7796_HEIGHT ? y : ST7796_HEIGHT - 1;
            uint16_t x0 = dx > DOT ? dx - DOT : 0;
            uint16_t y0 = dy > DOT ? dy - DOT : 0;
            st7796_fill_rect(x0, y0, DOT * 2, DOT * 2, ST_BLUE);
        }
        if (time_us_64() - last_beat > 2000000) {   // heartbeat so you see it's alive
            last_beat = time_us_64();
            printf("...alive, touches this poll: %d\n", n);
        }
        sleep_ms(8);
    }
}
