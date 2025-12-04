#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    float x;
    float y;
    float z;
} Data;

struct __packed Measurement {
    float x;
    float y;
    float z;
};

#endif