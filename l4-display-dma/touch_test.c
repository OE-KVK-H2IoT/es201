// L4 (touch) — GT911 touch paint: draw dots where you touch, and use the two
// user buttons to clear the screen (K1) and invert the colours (K2).
// Reuses the ST7796 display driver and the GT911 touch driver.
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"
#include "gt911.h"

#define DOT        5    // half-size of the dot drawn under your finger
#define BTN_CLEAR  14   // K1 (active-low): clear the drawing
#define BTN_INVERT 15   // K2 (active-low): swap draw/background colours

static void button_init(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);          // idle = 1, pressed = 0
}

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(2000);
    printf("\nL4 touch paint — touch to draw, K1 clears, K2 inverts\n");

    st7796_init();
    button_init(BTN_CLEAR);
    button_init(BTN_INVERT);

    uint16_t fg = ST_BLUE, bg = ST_WHITE;   // draw colour, background colour
    st7796_fill_screen(bg);

    if (!gt911_init()) {
        printf("!! GT911 did not answer on I2C0\n");
        st7796_fill_screen(ST_RED);
    } else {
        printf("GT911 ready\n");
    }

    gt911_point_t pts[5];
    bool clr_last = false, inv_last = false;
    while (true) {
        // --- buttons (edge-detected, active-low) ---
        bool clr = !gpio_get(BTN_CLEAR);
        bool inv = !gpio_get(BTN_INVERT);
        if (clr && !clr_last) { st7796_fill_screen(bg); printf("K1: clear\n"); }
        if (inv && !inv_last) {                         // swap colours and repaint
            uint16_t t = fg; fg = bg; bg = t;
            st7796_fill_screen(bg);
            printf("K2: invert (draw=%04X bg=%04X)\n", fg, bg);
        }
        clr_last = clr; inv_last = inv;

        // --- touch ---
        int n = gt911_read(pts, 5);
        for (int i = 0; i < n; i++) {
            uint16_t x = pts[i].x, y = pts[i].y;
            if (x >= ST7796_WIDTH)  x = ST7796_WIDTH  - 1;
            if (y >= ST7796_HEIGHT) y = ST7796_HEIGHT - 1;
            uint16_t x0 = x > DOT ? x - DOT : 0;
            uint16_t y0 = y > DOT ? y - DOT : 0;
            uint16_t w  = (x0 + 2 * DOT <= ST7796_WIDTH)  ? 2 * DOT : ST7796_WIDTH  - x0;
            uint16_t h  = (y0 + 2 * DOT <= ST7796_HEIGHT) ? 2 * DOT : ST7796_HEIGHT - y0;
            st7796_fill_rect(x0, y0, w, h, fg);
        }
        sleep_ms(10);
    }
}
