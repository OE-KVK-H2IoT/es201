// L2 + L4 — joystick-controlled box: read the joystick (the L2 ADC) and use it
// to drive a box around the display (the L4 panel). This is the two labs joined:
// analog input in, pixels out. It draws with dirty rectangles (erase the box's
// old spot, draw the new one) so only a tiny area changes each move — smooth and
// cheap, exactly the L4 lesson.
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "st7796.h"

#define JOY_X_INPUT  1    // joystick X (horizontal) -> ADC input 1 (GP27)
#define JOY_Y_INPUT  0    // joystick Y (vertical)   -> ADC input 0 (GP26)
#define JOY_X_GPIO   27
#define JOY_Y_GPIO   26
#define ADC_FULLSCALE 4095
#define BOX_SIZE     30   // box side, in pixels

// One ADC converter shared by the inputs: select the channel, then read it.
static uint16_t read_joystick(uint adc_input) {
    adc_select_input(adc_input);
    return adc_read();              // 0..4095
}

// Map a 0..4095 joystick reading onto a 0..travel screen position.
static int joystick_to_screen(uint16_t reading, int travel) {
    return (int)((long)reading * travel / ADC_FULLSCALE);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("\nJoystick box — move the stick to drive the box.\n\n");

    st7796_init();
    adc_init();
    adc_gpio_init(JOY_X_GPIO);     // hand the pins to the ADC (no digital input/pulls)
    adc_gpio_init(JOY_Y_GPIO);

    const uint16_t background_color = ST_BLACK, box_color = ST_CYAN;
    st7796_fill_screen(background_color);

    int previous_x = -1, previous_y = -1;   // last drawn position (-1 = nothing drawn yet)
    while (true) {
        uint16_t joystick_x = read_joystick(JOY_X_INPUT);
        uint16_t joystick_y = read_joystick(JOY_Y_INPUT);

        // Turn the two readings into a box position. The box can travel from 0 to
        // (screen size - box size) so it never runs off the edge. If an axis feels
        // reversed, flip it, e.g.:
        //   box_x = (ST7796_WIDTH - BOX_SIZE) - joystick_to_screen(joystick_x, ...);
        int box_x = joystick_to_screen(joystick_x, ST7796_WIDTH  - BOX_SIZE);
        int box_y = joystick_to_screen(joystick_y, ST7796_HEIGHT - BOX_SIZE);

        // Only repaint when the box actually moved (dirty-rectangle drawing):
        // erase the old square, draw the new one. Tiny ADC noise makes it twitch a
        // pixel even when you hold still — a natural place to add a deadzone later.
        if (box_x != previous_x || box_y != previous_y) {
            if (previous_x >= 0)
                st7796_fill_rect(previous_x, previous_y, BOX_SIZE, BOX_SIZE, background_color);
            st7796_fill_rect(box_x, box_y, BOX_SIZE, BOX_SIZE, box_color);
            previous_x = box_x; previous_y = box_y;
            printf("joystick x=%4u y=%4u  ->  box (%3d, %3d)\n",
                   joystick_x, joystick_y, box_x, box_y);
        }
        sleep_ms(20);   // ~50 updates per second
    }
}
