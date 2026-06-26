// L4 (touch) — GT911 touch paint: draw a smooth stroke under your finger, and
// use the two user buttons to clear (K1) and invert the colours (K2).
// Smoothness comes from interpolating a line between consecutive touch samples,
// per finger, so fast movement doesn't leave gaps between the polled points.
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "st7796.h"
#include "gt911.h"

#define DOT        2    // brush radius in pixels (small)
#define BTN_CLEAR  14   // K1 (active-low): clear the drawing
#define BTN_INVERT 15   // K2 (active-low): swap draw/background colours
#define MAX_ID     16   // GT911 track ids are small; clamp into this many slots

static void button_init(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

static void draw_dot(int x, int y, uint16_t color) {
    int x0 = x - DOT, y0 = y - DOT;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    int x1 = x + DOT, y1 = y + DOT;
    if (x1 >= ST7796_WIDTH)  x1 = ST7796_WIDTH  - 1;
    if (y1 >= ST7796_HEIGHT) y1 = ST7796_HEIGHT - 1;
    st7796_fill_rect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
}

// Stamp the brush along a straight line from (x0,y0) to (x1,y1).
static void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int steps = dx > dy ? dx : dy;
    if (steps == 0) { draw_dot(x0, y0, color); return; }
    for (int s = 0; s <= steps; s++) {
        draw_dot(x0 + (x1 - x0) * s / steps,
                 y0 + (y1 - y0) * s / steps, color);
    }
}

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(2000);
    printf("\nL4 touch paint — draw, K1 clears, K2 inverts\n");

    st7796_init();
    button_init(BTN_CLEAR);
    button_init(BTN_INVERT);

    uint16_t fg = ST_BLUE, bg = ST_WHITE;
    st7796_fill_screen(bg);
    if (!gt911_init()) { printf("!! GT911 not found\n"); st7796_fill_screen(ST_RED); }
    else                 printf("GT911 ready\n");

    struct { bool active; int x, y; } last[MAX_ID] = {0};  // previous point per finger
    gt911_point_t pts[5];
    bool clr_last = false, inv_last = false;

    while (true) {
        // --- buttons (edge-detected, active-low) ---
        bool clr = !gpio_get(BTN_CLEAR);
        bool inv = !gpio_get(BTN_INVERT);
        if ((clr && !clr_last) || (inv && !inv_last)) {
            if (inv && !inv_last) { uint16_t t = fg; fg = bg; bg = t; }
            st7796_fill_screen(bg);
            for (int j = 0; j < MAX_ID; j++) last[j].active = false;  // break strokes
            printf(inv && !inv_last ? "K2: invert\n" : "K1: clear\n");
        }
        clr_last = clr; inv_last = inv;

        // --- touch: interpolate from each finger's previous point ---
        int n = gt911_read(pts, 5);
        uint16_t seen = 0;
        for (int i = 0; i < n; i++) {
            int id = pts[i].id & (MAX_ID - 1);
            int x = pts[i].x, y = pts[i].y;
            if (x >= ST7796_WIDTH)  x = ST7796_WIDTH  - 1;
            if (y >= ST7796_HEIGHT) y = ST7796_HEIGHT - 1;
            if (last[id].active) draw_line(last[id].x, last[id].y, x, y, fg);
            else                 draw_dot(x, y, fg);
            last[id].active = true; last[id].x = x; last[id].y = y;
            seen |= (uint16_t)(1u << id);
        }
        for (int id = 0; id < MAX_ID; id++)      // lifted fingers end their stroke
            if (!(seen & (1u << id))) last[id].active = false;

        sleep_ms(5);   // poll briskly; the GT911 sets its own sample rate
    }
}
