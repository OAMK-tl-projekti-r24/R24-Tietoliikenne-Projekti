#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    int16_t x;
    int16_t y;
    int16_t z;
    uint8_t button_val;
} Data;

struct Measurement {
    int16_t x;
    int16_t y;
    int16_t z;
};

#endif