// L4 — touch-drag box: drag a box around the screen with your finger (GT911
// touch), and use the two buttons to change its colour and clear the screen.
// Combines three inputs/outputs: touch in, buttons in, display out. Draws with
// dirty rectangles (erase the box's old square, draw the new one) so dragging is
// smooth and cheap — the same L4 lesson, now driven by a finger instead of a timer.
#include <stdio.h>
#include "pico/stdlib.h"
#include "st7796.h"
#include "gt911.h"

#define BOX_SIZE    40
#define BTN_COLOR   14   // K1 (active-low): next box colour
#define BTN_CLEAR   15   // K2 (active-low): clear the screen

// A little palette the colour button cycles through.
static const uint16_t palette[] = {
    ST_CYAN, ST_YELLOW, ST_MAGENTA, ST_GREEN, ST_RED, ST_WHITE,
};
#define COLOR_COUNT (sizeof(palette) / sizeof(palette[0]))

static void button_init(uint pin) {
    gpio_init(pin); gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin);
}
static int clamp(int value, int lo, int hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("\nTouch-drag box — drag with a finger; K1 = next colour, K2 = clear.\n\n");

    st7796_init();
    button_init(BTN_COLOR);
    button_init(BTN_CLEAR);

    const uint16_t background_color = ST_BLACK;
    st7796_fill_screen(background_color);
    int color_index = 0;

    if (!gt911_init()) { printf("!! GT911 not found on I2C0\n"); st7796_fill_screen(ST_RED); }

    int box_x = (ST7796_WIDTH  - BOX_SIZE) / 2;   // start centred
    int box_y = (ST7796_HEIGHT - BOX_SIZE) / 2;
    int drawn_x = -1, drawn_y = -1;               // where the box is currently drawn

    gt911_point_t touch_points[5];
    bool color_was_pressed = false, clear_was_pressed = false;

    while (true) {
        // --- buttons ---
        bool color_pressed = !gpio_get(BTN_COLOR);
        bool clear_pressed = !gpio_get(BTN_CLEAR);
        if (color_pressed && !color_was_pressed) {
            color_index = (color_index + 1) % COLOR_COUNT;
            st7796_fill_rect(box_x, box_y, BOX_SIZE, BOX_SIZE, palette[color_index]);  // recolour in place
            printf("colour -> %d\n", color_index);
        }
        if (clear_pressed && !clear_was_pressed) {
            st7796_fill_screen(background_color);
            drawn_x = -1;                          // nothing on screen now
            printf("clear\n");
        }
        color_was_pressed = color_pressed; clear_was_pressed = clear_pressed;

        // --- touch: while a finger is down, centre the box under it ---
        int touch_count = gt911_read(touch_points, 5);   // -1 none new, 0 lifted, >0 touching
        if (touch_count > 0) {
            int finger_x = touch_points[0].x, finger_y = touch_points[0].y;
            box_x = clamp(finger_x - BOX_SIZE / 2, 0, ST7796_WIDTH  - BOX_SIZE);
            box_y = clamp(finger_y - BOX_SIZE / 2, 0, ST7796_HEIGHT - BOX_SIZE);
        }

        // --- redraw only if the box moved (dirty rectangles) ---
        if (box_x != drawn_x || box_y != drawn_y) {
            if (drawn_x >= 0)
                st7796_fill_rect(drawn_x, drawn_y, BOX_SIZE, BOX_SIZE, background_color);  // erase old
            st7796_fill_rect(box_x, box_y, BOX_SIZE, BOX_SIZE, palette[color_index]);       // draw new
            drawn_x = box_x; drawn_y = box_y;
        }
        sleep_ms(8);
    }
}
