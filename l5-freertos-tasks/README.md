# L5 — FreeRTOS Tasks (not yet wired)

Demo target: two preemptive tasks (blink + serial reporter) and a queue between
them, then pin tasks to the two RP2350 cores (SMP).

**Blocked on:** the FreeRTOS kernel, which is not part of the Pico SDK.

1. Add `FreeRTOS-Kernel` (the official repo has an RP2350 SMP port) as a
   submodule under this folder, or point `FREERTOS_KERNEL_PATH` at a checkout.
2. Add a `FreeRTOSConfig.h` (SMP enabled: `configNUMBER_OF_CORES 2`).
3. `CMakeLists.txt` includes the kernel's `FreeRTOS_Kernel_import.cmake` and
   links `FreeRTOS-Kernel-Heap4` (+ the RP2350 SMP port lib).

When ready: add `main.c` + `CMakeLists.txt` and uncomment
`add_subdirectory(l5-freertos-tasks)` in the top-level `CMakeLists.txt`.
