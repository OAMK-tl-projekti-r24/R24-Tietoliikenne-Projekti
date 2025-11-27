#ifndef MY_BLUETOOTH_H
#define MY_BLUETOOTH_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include "measurement.h"
#include <zephyr/sys/byteorder.h>

struct Data
{
    int16_t x;
    int16_t y;
    int16_t z;
    uint8_t button_val;
};

int my_ble_init(void);
int ble_send_accel(struct Data send);

#endif