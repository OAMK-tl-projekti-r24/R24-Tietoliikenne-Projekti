#ifndef MY_BLUETOOTH_H
#define MY_BLUETOOTH_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include "measurement.h"
#include <zephyr/sys/byteorder.h>

int my_ble_init(void);
int my_ble_send_gyro(struct Measurement val);
int my_ble_send_accel(struct Measurement val);
int my_ble_send_button(uint8_t value);

#endif
