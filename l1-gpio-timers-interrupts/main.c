// L1 — a blink driven by a periodic timer interrupt, not a sleep loop.
// The ISR toggles the LED and counts ticks; main() prints the measured period
// so you can compare the requested 500 ms against what the hardware delivers.
#include <stdio.h>
#include <inttypes.h>
#include "pico/stdlib.h"

#define LED1        16
#define PERIOD_MS   500

static volatile uint32_t tick_count = 0;
static volatile uint64_t last_edge_us = 0;
static volatile int64_t  measured_dt_us = 0;

// Runs in interrupt context: keep it short, touch only volatiles.
static bool on_tick(struct repeating_timer *t) {
    uint64_t now = time_us_64();
    measured_dt_us = (int64_t)(now - last_edge_us);
    last_edge_us = now;
    tick_count++;
    gpio_xor_mask(1u << LED1);   // toggle the LED atomically
    return true;                 // keep the timer running
}

int main(void) {
    stdio_init_all();
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);

    last_edge_us = time_us_64();
    struct repeating_timer timer;
    add_repeating_timer_ms(PERIOD_MS, on_tick, NULL, &timer);

    while (true) {
        // Snapshot volatiles, then report. Requested 500000 us; measure the truth.
        printf("tick=%" PRIu32 "  dt=%" PRId64 " us  (requested %d us)\n",
               tick_count, measured_dt_us, PERIOD_MS * 1000);
        sleep_ms(1000);
    }
}
