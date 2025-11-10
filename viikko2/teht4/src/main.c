/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <dk_buttons_and_leds.h>
#include "my_lbs.h"
#include "adc.h"

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONN |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	800, /* Min Advertising Interval 500ms (800*0.625ms) */
	801, /* Max Advertising Interval 500.625ms (801*0.625ms) */
	NULL); /* Set to NULL for undirected advertising */

LOG_MODULE_REGISTER(Lesson4_Exercise2, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define USER_BUTTON DK_BTN1_MSK

#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_LED_BLINK_INTERVAL 1000
/* STEP 17 - Define the interval at which you want to send data at */
#define NOTIFY_INTERVAL 333
static struct k_work adv_work;
static struct bt_conn *default_conn = NULL;


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}
static void advertising_start(void)
{
	k_work_submit(&adv_work);
}
static void recycled_cb(void)
{
	printk("Connection object available from previous conn. Disconnect is complete!\n");
	advertising_start();
}

static uint16_t app_button_cb(void)
{
	return button_state;
}

/* STEP 18.1 - Define the thread function  */
void send_data_thread(void)
{
	while (1) {
		/* Simulate data */
		struct Measurement m = readADCValue();
		/* Send notification, the function sends notifications only if a client is subscribed */
		my_lbs_send_sensor_notify(m.x);
		k_sleep(K_MSEC(NOTIFY_INTERVAL));

		my_lbs_send_sensor_notify(m.y);
		k_sleep(K_MSEC(NOTIFY_INTERVAL));

		my_lbs_send_sensor_notify(m.z);
		k_sleep(K_MSEC(NOTIFY_INTERVAL));
	}
}

static struct my_lbs_cb app_callbacks = {
	//.led_cb = app_led_cb,
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state_mask, uint32_t has_changed)
{
    static bool last_button_state = false;

    if (has_changed & USER_BUTTON) {
        bool current_state = (button_state_mask & USER_BUTTON) ? true : false;

        // Only send an indication if the button state actually changed
        if (current_state != last_button_state) {
            last_button_state = current_state;

            // Update your counter/state
            if (current_state) {
                button_state++;
                if (button_state > 5) {
                    button_state = 0;
                }
            }

            // Send the indication
			if (default_conn != NULL) 
			{
            	my_lbs_send_button_state_indicate(default_conn);
			}
        }
    }
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    printk("Connected\n");

    // Keep a reference to the connected client
    default_conn = bt_conn_ref(conn);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
    printk("Disconnected (reason %u)\n", reason);
}


struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.recycled = recycled_cb,
};

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}

int main(void)
{
	int err;

	LOG_INF("Starting Lesson 4 - Exercise 2 \n");

	if(initializeADC() != 0)
	{
	printk("ADC initialization failed!");
	return 1;
	}

	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return -1;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}
	bt_conn_cb_register(&connection_callbacks);

	err = my_lbs_init(&app_callbacks);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return -1;
	}
	LOG_INF("Bluetooth initialized\n");
	k_work_init(&adv_work, adv_work_handler);
	advertising_start();
}
/* STEP 18.2 - Define and initialize a thread to send data periodically */
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
