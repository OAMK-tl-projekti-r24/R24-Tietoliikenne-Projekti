#ifndef MY_BLUETOOTH_H
#define MY_BLUETOOTH_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>

int my_ble_init(void);
int my_ble_send_gyro(uint16_t value);

#endif
