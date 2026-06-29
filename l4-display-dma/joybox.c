// L2 + L4 — joystick-controlled box: read the joystick (the L2 ADC) and use it
// to drive a box around the display (the L4 panel). This is the two labs joined:
// analog input in, pixels out. It draws with dirty rectangles (erase the box's
// old spot, draw the new one) so only a tiny area changes each move.
//
// BIAS ELIMINATION: a joystick at rest does NOT read mid-scale (2048) — it sits
// at its own offset (~2031/1981 on our unit). So at boot we measure that resting
// value and subtract it, exactly like zeroing a gyro at startup in the BSc IMU
// lab: a *systematic* offset you remove by calibration (unlike random noise,
// which you can only average down). After that, "stick centred" maps to
// "box centred", whatever the hardware's offset happens to be.
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "st7796.h"

#define JOY_X_INPUT     1    // joystick X (horizontal) -> ADC input 1 (GP27)
#define JOY_Y_INPUT     0    // joystick Y (vertical)   -> ADC input 0 (GP26)
#define JOY_X_GPIO      27
#define JOY_Y_GPIO      26
#define BOX_SIZE        30   // box side, in pixels
#define CAL_SAMPLES     256  // samples averaged to measure the resting bias
#define FULL_DEFLECTION 1800 // counts from rest to a stick extreme (maps to a screen edge)

// One ADC converter shared by the inputs: select the channel, then read it.
static uint16_t read_joystick(uint adc_input) {
    adc_select_input(adc_input);
    return adc_read();              // 0..4095
}

// Bias calibration: average N readings while the stick is at rest. The average is
// the axis's resting offset — the same "measure the zero, then subtract it" step
// you used to null gyro bias on the IMU. Averaging also beats the noise down so
// the captured zero is stable.
static uint16_t measure_bias(uint adc_input) {
    uint32_t sum = 0;
    for (int i = 0; i < CAL_SAMPLES; i++) { sum += read_joystick(adc_input); sleep_ms(2); }
    return (uint16_t)(sum / CAL_SAMPLES);
}

// Map a reading to a 0..travel box position with the resting value (bias) at the
// centre: deflection from rest, scaled so a full throw reaches the edge, clamped.
static int centered_position(uint16_t reading, uint16_t rest, int travel) {
    int deflection = (int)reading - (int)rest;
    int pos = travel / 2 + deflection * (travel / 2) / FULL_DEFLECTION;
    return pos < 0 ? 0 : (pos > travel ? travel : pos);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("\nJoystick box — move the stick to drive the box.\n\n");

    st7796_init();
    adc_init();
    adc_gpio_init(JOY_X_GPIO);     // hand the pins to the ADC (no digital input/pulls)
    adc_gpio_init(JOY_Y_GPIO);

    // Measure the resting bias FIRST — leave the stick centred during boot.
    printf("calibrating bias — leave the stick centred...\n");
    uint16_t rest_x = measure_bias(JOY_X_INPUT);
    uint16_t rest_y = measure_bias(JOY_Y_INPUT);
    // Log the measured bias as key=value (distinct keys, so a logger/plot can
    // pick it out without confusing it for a live sample).
    printf("bias_x=%u bias_y=%u\n", rest_x, rest_y);

    const uint16_t background_color = ST_BLACK, box_color = ST_CYAN;
    st7796_fill_screen(background_color);

    int previous_x = -1, previous_y = -1;   // last drawn position (-1 = nothing drawn yet)
    while (true) {
        uint16_t joystick_x = read_joystick(JOY_X_INPUT);
        uint16_t joystick_y = read_joystick(JOY_Y_INPUT);

        // Map to a box position with the measured bias removed, so rest -> centre.
        // If an axis feels reversed, negate the deflection (swap rest and reading
        // in centered_position, or flip the sign inside it).
        int box_x = centered_position(joystick_x, rest_x, ST7796_WIDTH  - BOX_SIZE);
        int box_y = centered_position(joystick_y, rest_y, ST7796_HEIGHT - BOX_SIZE);

        // Only repaint when the box actually moved (dirty-rectangle drawing):
        // erase the old square, draw the new one. Tiny ADC noise makes it twitch a
        // pixel even when you hold still — a natural place to add a deadzone later.
        if (box_x != previous_x || box_y != previous_y) {
            if (previous_x >= 0)
                st7796_fill_rect(previous_x, previous_y, BOX_SIZE, BOX_SIZE, background_color);
            st7796_fill_rect(box_x, box_y, BOX_SIZE, BOX_SIZE, box_color);
            previous_x = box_x; previous_y = box_y;
            // Clean key=value so serial-log.sh + host/plot.py can plot it:
            //   x,y   = raw (carry the bias, sit near ~2031/1981 at rest)
            //   cx,cy = bias-corrected (sit near 0 at rest) — the offset removed
            //   boxx,boxy = resulting box position
            printf("x=%u y=%u cx=%d cy=%d boxx=%d boxy=%d\n",
                   joystick_x, joystick_y,
                   (int)joystick_x - rest_x, (int)joystick_y - rest_y, box_x, box_y);
        }
        sleep_ms(20);   // ~50 updates per second
    }
}
