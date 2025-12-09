#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    float x;
    float y;
    float z;
} Data;

struct Measurement {
    float x;
    float y;
    float z;
} __attribute__((packed));

#endif