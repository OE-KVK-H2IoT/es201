# L6 — Capstone: Mini-Scope (not yet wired)

Demo target: a tiny oscilloscope — free-running ADC capture via DMA into a ring
buffer, rendered on the TFT, with trigger + timebase controls. Integrates L2
(ADC), L4 (display + DMA), and the timing discipline from L1/L3.

**Blocked on L4** (display driver + confirmed pins) and reuses the ADC+DMA path.
Build this last, once L2 and L4 demos are verified on the board.

When ready: add `main.c` + `CMakeLists.txt` (link
`hardware_adc hardware_dma hardware_spi`) and uncomment
`add_subdirectory(l6-capstone-mini-scope)` in the top-level `CMakeLists.txt`.
