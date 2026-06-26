// L2 — read one ADC channel (joystick X on GP26/ADC0) and characterise it.
// Prints raw 12-bit counts and the converted voltage, plus a running mean over
// a block of samples so you can see the noise floor. CSV-friendly output: pipe
// it to a file and analyse on the host (numpy/matplotlib).
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define ADC_GPIO   26   // GP26 = ADC input 0 (joystick X axis)
#define ADC_CHAN    0
#define BLOCK     100   // samples per reported block

int main(void) {
    stdio_init_all();
    adc_init();
    adc_gpio_init(ADC_GPIO);     // disable digital input on the ADC pin
    adc_select_input(ADC_CHAN);

    const float to_volts = 3.3f / (1 << 12);   // 12-bit, 3.3 V reference

    printf("block,mean_raw,mean_v\n");          // CSV header
    uint32_t block = 0;
    while (true) {
        uint32_t sum = 0;
        uint16_t lo = 0xFFFF, hi = 0;
        for (int i = 0; i < BLOCK; i++) {
            uint16_t raw = adc_read();
            sum += raw;
            if (raw < lo) lo = raw;
            if (raw > hi) hi = raw;
            sleep_us(200);
        }
        float mean = (float)sum / BLOCK;
        printf("%lu,%.1f,%.4f   (min=%u max=%u spread=%u)\n",
               (unsigned long)block++, mean, mean * to_volts, lo, hi, hi - lo);
        sleep_ms(200);
    }
}
