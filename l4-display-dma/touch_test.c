// L4 (touch) — GT911 touch paint that NARRATES what it does, for learning.
// ===========================================================================
// HOW A CAPACITIVE TOUCHSCREEN GIVES YOU DATA
//   The GT911 is an I2C chip that scans its glass and, each time you ask, hands
//   back a SNAPSHOT: "these 0..5 fingers are down right now, at these (x,y)."
//   It refreshes that snapshot at its own fixed rate (a new sample every few
//   ms) — you cannot get points any faster than the hardware produces them.
//
//   So motion arrives as DISCRETE SAMPLES, not a continuous line:
//     * first time we see a finger  -> we only have a point        -> draw a DOT
//     * next samples of that finger -> we have "from" and "to"      -> draw a LINE
//   The line between two samples is interpolation: we didn't see the path the
//   finger took, so we assume a straight one. A SLOW finger gives tiny gaps
//   (looks continuous either way); a FAST finger gives big gaps (dots without
//   the line). The console below prints each event so you can SEE this happen:
//   every move line shows the pixel gap since the last sample — flick fast and
//   watch it jump. The number of move lines per second IS the controller's
//   report rate.
//
//   K1 clears the canvas; K2 inverts the colours.
// ===========================================================================
#include <stdio.h>
#include <stdlib.h>          // abs()
#include "pico/stdlib.h"
#include "st7796.h"
#include "gt911.h"

#define DOT        1    // brush radius in pixels
#define BTN_CLEAR  14   // K1 (active-low)
#define BTN_INVERT 15   // K2 (active-low)
#define MAX_ID     16

static void button_init(uint pin) {
    gpio_init(pin); gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin);
}

static void draw_dot(int x, int y, uint16_t color) {
    int x0 = x - DOT, y0 = y - DOT, x1 = x + DOT, y1 = y + DOT;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= ST7796_WIDTH)  x1 = ST7796_WIDTH  - 1;
    if (y1 >= ST7796_HEIGHT) y1 = ST7796_HEIGHT - 1;
    st7796_fill_rect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
}

// Stamp the brush at every pixel step along the line — this is the interpolation.
static void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int steps = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
    if (steps == 0) { draw_dot(x0, y0, color); return; }
    for (int s = 0; s <= steps; s++)
        draw_dot(x0 + (x1 - x0) * s / steps, y0 + (y1 - y0) * s / steps, color);
}

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(2000);

    printf("\n=== L4 Touch Paint (narrated) ===\n");
    printf("The GT911 gives discrete (x,y) samples at its own report rate.\n");
    printf("DOWN  = first contact  -> we draw a single DOT.\n");
    printf("move  = next samples   -> we draw a LINE across the pixel gap.\n");
    printf("Big gap = fast finger. Flick fast and watch the gap number jump.\n\n");

    st7796_init();
    button_init(BTN_CLEAR);
    button_init(BTN_INVERT);

    uint16_t fg = ST_BLUE, bg = ST_WHITE;
    st7796_fill_screen(bg);
    if (!gt911_init()) { printf("!! GT911 not found on I2C0\n"); st7796_fill_screen(ST_RED); }
    else                 printf("GT911 ready — touch the screen\n\n");

    // Per-finger memory: last point + how many samples this stroke has lasted.
    struct { bool active; int x, y; int count; } last[MAX_ID] = {0};
    gt911_point_t pts[5];
    bool clr_last = false, inv_last = false;

    while (true) {
        // --- buttons ---
        bool clr = !gpio_get(BTN_CLEAR);
        bool inv = !gpio_get(BTN_INVERT);
        if (clr && !clr_last) {
            st7796_fill_screen(bg);
            for (int j = 0; j < MAX_ID; j++) last[j].active = false;
            printf("[K1] clear — canvas wiped, all strokes reset\n");
        }
        if (inv && !inv_last) {
            uint16_t t = fg; fg = bg; bg = t;
            st7796_fill_screen(bg);
            for (int j = 0; j < MAX_ID; j++) last[j].active = false;
            printf("[K2] invert — draw=0x%04X background=0x%04X\n", fg, bg);
        }
        clr_last = clr; inv_last = inv;

        // --- touch ---
        // gt911_read returns -1 when there's no NEW sample yet (we poll far
        // faster than the controller reports). Crucial: skip those polls and
        // leave finger state alone — treating "no new data" as "finger up" is
        // what chops a smooth drag into single dots. Only a fresh sample (>=0)
        // updates who is down; a fresh sample with 0 fingers means a real lift.
        int n = gt911_read(pts, 5);
        if (n < 0) { sleep_ms(1); continue; }
        uint16_t seen = 0;
        for (int i = 0; i < n; i++) {
            int id = pts[i].id & (MAX_ID - 1);
            int x = pts[i].x, y = pts[i].y;
            if (x >= ST7796_WIDTH)  x = ST7796_WIDTH  - 1;
            if (y >= ST7796_HEIGHT) y = ST7796_HEIGHT - 1;

            if (!last[id].active) {
                // First time we see this finger: we have a point, not a path.
                printf("finger %d DOWN at (%3d,%3d) -> first contact, drawing a DOT\n", id, x, y);
                draw_dot(x, y, fg);
                last[id].count = 1;
            } else {
                int gap = abs(x - last[id].x) > abs(y - last[id].y)
                        ? abs(x - last[id].x) : abs(y - last[id].y);
                if (gap > 0) {
                    // We have "from" and "to": connect them so fast moves don't gap.
                    printf("finger %d move (%3d,%3d)->(%3d,%3d)  gap=%3dpx -> drawing a LINE (%d steps)\n",
                           id, last[id].x, last[id].y, x, y, gap, gap);
                    draw_line(last[id].x, last[id].y, x, y, fg);
                } else {
                    draw_dot(x, y, fg);            // finger held still: no narration spam
                }
                last[id].count++;
            }
            last[id].active = true; last[id].x = x; last[id].y = y;
            seen |= (uint16_t)(1u << id);
        }
        // Fingers we tracked but didn't see this snapshot have lifted.
        for (int id = 0; id < MAX_ID; id++) {
            if (last[id].active && !(seen & (1u << id))) {
                printf("finger %d UP -> stroke ended after %d samples\n", id, last[id].count);
                last[id].active = false;
            }
        }
        sleep_ms(1);
    }
}
