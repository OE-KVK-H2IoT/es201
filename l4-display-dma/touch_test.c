// L4 (touch) — interactive GT911 test: clear the panel, then draw a dot wherever
// you touch and print the coordinates over USB-serial. Reuses the ST7796 driver.
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"
#include "gt911.h"

#define DOT 6   // half-size of the dot drawn under your finger

int main(void) {
    stdio_init_all();
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
    while (true) {
        int n = gt911_read(pts, 5);
        for (int i = 0; i < n; i++) {
            uint16_t x = pts[i].x, y = pts[i].y;
            printf("touch id=%u  x=%u  y=%u  size=%u\n", pts[i].id, x, y, pts[i].size);
            // Draw a dot at the reported point (clamped to the panel). If the dot
            // lands away from your finger, touch and display axes differ — swap or
            // flip x/y here to calibrate (e.g. y = ST7796_HEIGHT-1 - y).
            if (x < ST7796_WIDTH && y < ST7796_HEIGHT) {
                uint16_t x0 = x > DOT ? x - DOT : 0;
                uint16_t y0 = y > DOT ? y - DOT : 0;
                st7796_fill_rect(x0, y0, DOT * 2, DOT * 2, ST_BLUE);
            }
        }
        sleep_ms(8);
    }
}
