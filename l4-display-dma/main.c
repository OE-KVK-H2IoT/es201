// L4 — move a buffer to SPI with DMA and measure the throughput.
//
// SCOPE / HONESTY: the 52Pi EP-0172 ST7796 panel pins are NOT yet confirmed,
// so the SPI pins below are PLACEHOLDERS and this demo does NOT run the ST7796
// init sequence — it will not draw to the panel yet. What it DOES do is exercise
// the real SPI + DMA data path and measure transfer time, which is the core L4
// mechanism (background data movement while the CPU is free). Put a logic
// analyzer on the SPI clock to see the burst. Add the panel driver + confirmed
// pins to turn this into a real framebuffer push. See README.md.
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

// TODO: confirm against the 52Pi EP-0172 wiki before driving the real panel.
#define SPI_PORT  spi0
#define PIN_SCK   18    // placeholder
#define PIN_MOSI  19    // placeholder

#define BUF_LEN   4096  // bytes per DMA burst

static uint8_t framebuf[BUF_LEN];

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the USB-serial host time to attach

    uint baud = spi_init(SPI_PORT, 16 * 1000 * 1000);   // 16 MHz to start
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    for (int i = 0; i < BUF_LEN; i++) framebuf[i] = (uint8_t)i;

    int chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(SPI_PORT, true)); // pace by SPI TX
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);

    printf("len_bytes,us,mbit_per_s\n");                 // CSV header
    while (true) {
        uint64_t t0 = time_us_64();
        dma_channel_configure(chan, &c,
            &spi_get_hw(SPI_PORT)->dr,  // dst: SPI data register (fixed)
            framebuf,                   // src: framebuffer (incrementing)
            BUF_LEN, true);             // start now
        dma_channel_wait_for_finish_blocking(chan);
        uint64_t t1 = time_us_64();

        uint32_t us = (uint32_t)(t1 - t0);
        float mbps = us ? (BUF_LEN * 8.0f) / us : 0.0f;  // bits/us == Mbit/s
        printf("%d,%lu,%.2f   (SPI baud=%u)\n", BUF_LEN, (unsigned long)us, mbps, baud);
        sleep_ms(500);
    }
}
