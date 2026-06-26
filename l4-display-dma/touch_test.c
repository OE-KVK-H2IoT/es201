// L4 (touch) — GT911 touch paint, with smooth (interpolated) strokes.
// ===========================================================================
// WHAT THIS PROGRAM DOES
//   Reads finger touches from the GT911 capacitive controller and paints them
//   on the ST7796 display. K1 clears the canvas; K2 inverts the colours.
//
// THE INTERESTING PROBLEM: SAMPLING
//   The touch controller does not give you a continuous line — it gives you
//   *discrete samples*. Each time you poll it, you get "here are the 0..5
//   fingers touching right now, at these (x,y)." It updates at its own fixed
//   report rate (a sample only every few ms), no matter how fast you poll.
//
//   So if you just stamp one dot per sample, a SLOW finger looks continuous
//   (samples are close together) but a FAST finger looks like a dotted line:
//   between two reports the finger travelled many pixels, and nothing was
//   drawn in the gap.
//
//   THE FIX (interpolation): remember where each finger was on the *previous*
//   sample, and draw a straight LINE from that point to the new one. We can't
//   know the true path between samples, so we "cheat" and assume it was a
//   straight line — at these sample rates that is invisibly close to the truth
//   and turns the dotted line back into a continuous stroke.
// ===========================================================================
#include <stdio.h>
#include <stdlib.h>          // abs()
#include "pico/stdlib.h"
#include "st7796.h"          // display: fill_rect / fill_screen
#include "gt911.h"           // touch: gt911_read() -> array of (x,y) points

#define DOT        1    // brush radius in pixels (1 = thin pen, 3-4 = marker)
#define BTN_CLEAR  14   // K1 (active-low): clear the drawing
#define BTN_INVERT 15   // K2 (active-low): swap draw/background colours
#define MAX_ID     16   // GT911 track ids are small (0..4); clamp into this many slots

static void button_init(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);          // idle reads 1; pressed pulls to 0 (active-low)
}

// Stamp the brush (a small filled square of half-size DOT) centred at (x,y),
// clamped so it never draws off the edge of the panel.
static void draw_dot(int x, int y, uint16_t color) {
    int x0 = x - DOT, y0 = y - DOT;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    int x1 = x + DOT, y1 = y + DOT;
    if (x1 >= ST7796_WIDTH)  x1 = ST7796_WIDTH  - 1;
    if (y1 >= ST7796_HEIGHT) y1 = ST7796_HEIGHT - 1;
    st7796_fill_rect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
}

// Interpolation: stamp the brush at every pixel step along the straight line
// from (x0,y0) to (x1,y1). `steps` is the longer of the two axis distances, so
// the dots overlap and form an unbroken stroke regardless of slope.
static void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int steps = dx > dy ? dx : dy;
    if (steps == 0) { draw_dot(x0, y0, color); return; }   // same point: one dot
    for (int s = 0; s <= steps; s++) {
        draw_dot(x0 + (x1 - x0) * s / steps,
                 y0 + (y1 - y0) * s / steps, color);
    }
}

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);   // unbuffered: every printf flushes now
    sleep_ms(2000);                     // give the serial host time to attach
    printf("\nL4 touch paint — draw, K1 clears, K2 inverts\n");

    st7796_init();
    button_init(BTN_CLEAR);
    button_init(BTN_INVERT);

    uint16_t fg = ST_BLUE, bg = ST_WHITE;   // current draw colour / background
    st7796_fill_screen(bg);
    if (!gt911_init()) { printf("!! GT911 not found\n"); st7796_fill_screen(ST_RED); }
    else                 printf("GT911 ready\n");

    // Per-finger memory: the last point we saw for each track id, so we can draw
    // a line FROM it TO the new sample. `active` is false until the finger's
    // first sample, and is reset when the finger lifts (so a new touch doesn't
    // connect to where the previous one ended).
    struct { bool active; int x, y; } last[MAX_ID] = {0};
    gt911_point_t pts[5];
    bool clr_last = false, inv_last = false;

    while (true) {
        // --- buttons (edge-detected so one press = one action) ---
        bool clr = !gpio_get(BTN_CLEAR);    // active-low -> invert
        bool inv = !gpio_get(BTN_INVERT);
        if ((clr && !clr_last) || (inv && !inv_last)) {
            if (inv && !inv_last) { uint16_t t = fg; fg = bg; bg = t; }  // swap colours
            st7796_fill_screen(bg);
            for (int j = 0; j < MAX_ID; j++) last[j].active = false;     // break strokes
            printf(inv && !inv_last ? "K2: invert\n" : "K1: clear\n");
        }
        clr_last = clr; inv_last = inv;

        // --- touch: read this sample's points and connect each to its last ---
        int n = gt911_read(pts, 5);     // n = number of fingers down (0..5)
        uint16_t seen = 0;              // bitmask of track ids present this poll
        for (int i = 0; i < n; i++) {
            int id = pts[i].id & (MAX_ID - 1);
            int x = pts[i].x, y = pts[i].y;             // already in display range
            if (x >= ST7796_WIDTH)  x = ST7796_WIDTH  - 1;
            if (y >= ST7796_HEIGHT) y = ST7796_HEIGHT - 1;
            if (last[id].active) draw_line(last[id].x, last[id].y, x, y, fg);  // join
            else                 draw_dot(x, y, fg);                          // first
            last[id].active = true; last[id].x = x; last[id].y = y;
            seen |= (uint16_t)(1u << id);
        }
        // Any finger we tracked but didn't see this poll has lifted: end its stroke.
        for (int id = 0; id < MAX_ID; id++)
            if (!(seen & (1u << id))) last[id].active = false;

        sleep_ms(1);   // poll briskly; the GT911 still sets its own sample rate
    }
}
