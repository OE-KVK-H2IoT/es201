// Minimal GT911 capacitive-touch driver for the 52Pi EP-0172.
// I2C0: SDA=GP8 SCL=GP9, RST=GP10, INT=GP11 (confirmed on the 52Pi wiki).
#pragma once
#include <stdint.h>

typedef struct {
    uint16_t x, y, size;
    uint8_t  id;
} gt911_point_t;

// Reset + bring up the controller. Returns 1 if it answers on I2C, else 0.
int gt911_init(void);

// Read currently-touched points into pts[0..max-1]; returns the count (0..max).
int gt911_read(gt911_point_t *pts, int max);
