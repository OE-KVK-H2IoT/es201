// L2 — read BOTH joystick axes and print their raw 12-bit values live.
// X is on GP26 = ADC input 0, Y is on GP27 = ADC input 1. The RP2350 has a
// single ADC converter shared by the inputs, so you SELECT a channel, then read,
// and repeat per axis (round-robin). Output is CSV-friendly: pipe it to a file
// and plot X vs Y on the host. A trailing note shows the min..max each axis has
// reached, so swinging the stick to its corners *characterises its real range*.
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define ADC_X_GPIO 26   // joystick X axis -> ADC input 0
#define ADC_Y_GPIO 27   // joystick Y axis -> ADC input 1
#define ADC_X      0
#define ADC_Y      1

// One converter: pick the input, then read its 12-bit (0..4095) result.
static uint16_t read_axis(uint input) {
    adc_select_input(input);
    return adc_read();
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("\nL2 ADC — joystick X/Y raw readout (12-bit 0..4095, 3.3 V ref)\n");
    printf("Move the stick: each axis tracks its own value; centre is ~2048.\n\n");

    adc_init();
    adc_gpio_init(ADC_X_GPIO);     // disable the pin's digital input/pulls for analog use
    adc_gpio_init(ADC_Y_GPIO);

    const float to_v = 3.3f / (1 << 12);
    uint16_t xlo = 0xFFFF, xhi = 0, ylo = 0xFFFF, yhi = 0;   // range seen so far

    printf("x_raw,y_raw,x_volt,y_volt\n");   // CSV header
    while (true) {
        uint16_t x = read_axis(ADC_X);
        uint16_t y = read_axis(ADC_Y);
        if (x < xlo) xlo = x;  if (x > xhi) xhi = x;
        if (y < ylo) ylo = y;  if (y > yhi) yhi = y;

        printf("%4u,%4u,%.3f,%.3f   (range  x:%u-%u  y:%u-%u)\n",
               x, y, x * to_v, y * to_v, xlo, xhi, ylo, yhi);
        sleep_ms(100);
    }
}
