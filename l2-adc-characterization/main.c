// L2 — read BOTH joystick axes and show each as a raw value AND a text bar.
// X is on GP26 = ADC input 0, Y is on GP27 = ADC input 1. The RP2350 has a
// single ADC converter shared by the inputs, so you SELECT a channel, then read,
// and repeat per axis (round-robin). Each line prints the raw 12-bit numbers
// (easy to parse/log) followed by a proportional bar per axis (easy to eyeball).
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define ADC_X_GPIO 26   // joystick X axis -> ADC input 0
#define ADC_Y_GPIO 27   // joystick Y axis -> ADC input 1
#define ADC_X      0
#define ADC_Y      1
#define ADC_MAX    4095 // 12-bit full scale
#define BAR_W      24   // characters in each axis bar

// One converter: pick the input, then read its 12-bit (0..4095) result.
static uint16_t read_axis(uint input) {
    adc_select_input(input);
    return adc_read();
}

// Print "[####....]" filled in proportion to raw / full-scale.
static void put_bar(uint16_t raw) {
    int n = (int)((long)raw * BAR_W / ADC_MAX);
    putchar('[');
    for (int i = 0; i < BAR_W; i++) putchar(i < n ? '#' : '.');
    putchar(']');
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("\nL2 ADC — joystick X/Y (raw 12-bit 0..4095, 3.3 V ref). Move the stick.\n\n");

    adc_init();
    adc_gpio_init(ADC_X_GPIO);     // disable the pin's digital input/pulls for analog use
    adc_gpio_init(ADC_Y_GPIO);

    while (true) {
        uint16_t x = read_axis(ADC_X);
        uint16_t y = read_axis(ADC_Y);
        // Raw numbers first (parse/log-friendly), then a bar per axis (eyeball).
        printf("x=%4u y=%4u  X ", x, y); put_bar(x);
        printf("  Y ");                  put_bar(y);
        printf("\n");
        sleep_ms(50);
    }
}
