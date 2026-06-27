# L4 — Display and DMA
es201_app(l4_display main.c st7796.c)                    # ST7796 panel self-test
target_link_libraries(l4_display hardware_spi hardware_dma)

es201_app(l4_touch touch_test.c st7796.c gt911.c)        # GT911 touch test (draws dots)
target_link_libraries(l4_touch hardware_spi hardware_dma hardware_i2c)

es201_app(l4_optimize optimize.c st7796.c)               # full-redraw vs dirty-rect demo
target_link_libraries(l4_optimize hardware_spi hardware_dma)
