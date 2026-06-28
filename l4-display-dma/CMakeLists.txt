# L4 — Display and DMA
es201_app(l4_display main.c st7796.c)                    # ST7796 panel self-test
target_link_libraries(l4_display hardware_spi hardware_dma)

es201_app(l4_touch touch_test.c st7796.c gt911.c)        # GT911 touch test (draws dots)
target_link_libraries(l4_touch hardware_spi hardware_dma hardware_i2c)

es201_app(l4_optimize optimize.c st7796.c)               # full vs dirty vs double-buffer demo
target_link_libraries(l4_optimize hardware_spi hardware_dma)
# Forward -D cache vars to compile definitions, so START_MODE / ENABLE_DBUF / LOG
# can be overridden from the build command without editing the source.
foreach(opt START_MODE ENABLE_DBUF LOG TARGET_FPS)
    if(DEFINED ${opt})
        target_compile_definitions(l4_optimize PRIVATE ${opt}=${${opt}})
    endif()
endforeach()
