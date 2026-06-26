// L4 (touch) — GT911 touch test: clear the panel, then draw a blue dot wherever
// you touch and print the coordinates over serial. Reuses the ST7796 driver.
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"
#include "gt911.h"

#define DOT 5   // half-size of the dot drawn under your finger

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(2000);
    printf("\nL4 GT911 touch test — draw on the screen\n");

    st7796_init();
    st7796_fill_screen(ST_WHITE);

    if (!gt911_init()) {
        printf("!! GT911 did not answer on I2C0\n");
        st7796_fill_screen(ST_RED);
    } else {
        printf("GT911 ready — touch to draw\n");
    }

    gt911_point_t pts[5];
    while (true) {
        int n = gt911_read(pts, 5);
        for (int i = 0; i < n; i++) {
            uint16_t x = pts[i].x, y = pts[i].y;
            // Touch and display share the 0..319 / 0..479 range. If the dot comes
            // out mirrored relative to your finger, flip the offending axis here:
            //   x = ST7796_WIDTH  - 1 - x;     // mirror left<->right
            //   y = ST7796_HEIGHT - 1 - y;     // mirror top<->bottom
            printf("touch %u: x=%u y=%u size=%u\n", pts[i].id, x, y, pts[i].size);

            if (x >= ST7796_WIDTH)  x = ST7796_WIDTH  - 1;
            if (y >= ST7796_HEIGHT) y = ST7796_HEIGHT - 1;
            uint16_t x0 = x > DOT ? x - DOT : 0;
            uint16_t y0 = y > DOT ? y - DOT : 0;
            uint16_t w  = (x0 + 2 * DOT <= ST7796_WIDTH)  ? 2 * DOT : ST7796_WIDTH  - x0;
            uint16_t h  = (y0 + 2 * DOT <= ST7796_HEIGHT) ? 2 * DOT : ST7796_HEIGHT - y0;
            st7796_fill_rect(x0, y0, w, h, ST_BLUE);
        }
        sleep_ms(10);
    }
}
