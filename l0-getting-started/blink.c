#include "pico/stdlib.h"

#define LED_USER1_PIN 16   // user LED 1 on the EP-0172 carrier
#define LED_USER2_PIN 17   // user LED 2 on the EP-0172 carrier

int main(void) {
    gpio_init(LED_USER1_PIN);
    gpio_set_dir(LED_USER1_PIN, GPIO_OUT);
    gpio_init(LED_USER2_PIN);
    gpio_set_dir(LED_USER2_PIN, GPIO_OUT);

    bool on = false;
    while (true) {
        on = !on;
        gpio_put(LED_USER1_PIN, on);
        gpio_put(LED_USER2_PIN, !on);   // opposite phase: easy to see both work
        sleep_ms(250);
    }
}
