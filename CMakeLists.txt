cmake_minimum_required(VERSION 3.13)

# ---------------------------------------------------------------------------
# ES201 — Architecture of Embedded Systems: lab demo sources (RP2350 / Pico 2 W)
#
# One tree, many labs. Build everything:
#     cmake -B build && cmake --build build
# Build just one lab demo:
#     cmake --build build --target l3_pwm_tone
#
# Board: the lab hardware is a Pico 2 W, but these non-WiFi demos build and run
# the same as plain pico2 (proven to build with only the tinyusb + pioasm SDK
# submodules initialised). Override if you need WiFi:  -DPICO_BOARD=pico2_w
# ---------------------------------------------------------------------------
set(PICO_BOARD pico2 CACHE STRING "Target board (pico2 or pico2_w)")

include(pico_sdk_import.cmake)

project(es201_labs C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Helper: a USB-serial console app linked against pico_stdlib, with .uf2 output.
# Add extra hardware libs per lab with target_link_libraries(<name> ...).
function(es201_app name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} pico_stdlib)
    pico_enable_stdio_usb(${name} 1)    # printf over USB-CDC
    pico_enable_stdio_uart(${name} 0)
    pico_add_extra_outputs(${name})     # .uf2 / .hex / .map / .dis
endfunction()

add_subdirectory(l0-getting-started)
add_subdirectory(l1-gpio-timers-interrupts)
add_subdirectory(l2-adc-characterization)
add_subdirectory(l3-pwm-signal-generation)
add_subdirectory(l4-display-dma)          # SPI+DMA throughput skeleton (panel driver still TODO)
# add_subdirectory(l5-freertos-tasks)     # TODO: needs FreeRTOS-Kernel
# add_subdirectory(l6-capstone-mini-scope)
