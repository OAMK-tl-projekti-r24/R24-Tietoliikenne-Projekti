#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>

struct Measurement {
    int16_t x;
    int16_t y;
    int16_t z;
};


struct Accel {
    float x;
    float y;
    float z;
};

#endif