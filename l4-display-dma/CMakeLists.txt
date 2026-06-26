# L4 — Display and DMA: full ST7796 panel self-test (SPI0 + DMA pixel streaming)
es201_app(l4_display main.c st7796.c)
target_link_libraries(l4_display hardware_spi hardware_dma)
