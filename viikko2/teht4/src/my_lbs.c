/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include "my_lbs.h"

LOG_MODULE_DECLARE(Lesson4_Exercise2);

static bool notify_mysensor_enabled;
static bool indicate_enabled;
uint16_t button_state;
static struct my_lbs_cb lbs_cb;
static uint16_t sensor_value;


/* STEP 4 - Define an indication parameter */
static struct bt_gatt_indicate_params ind_params;


/* STEP 3 - Implement the configuration change callback function */
static void mylbsbc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	indicate_enabled = (value & BT_GATT_CCC_INDICATE) != 0;

}

/* STEP 13 - Define the configuration change callback function for the MYSENSOR characteristic */
static void mylbsbc_ccc_mysensor_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_mysensor_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
}

// This function is called when a remote device has acknowledged the indication at its host layer
static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
    if (err) {
        printk("Indication failed (err %d)\n", err);
    } else {
        printk("Indication sent successfully\n");
    }
}


static ssize_t read_button(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	// get a pointer to button_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	//uint16_t *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (lbs_cb.button_cb) {
		// Call the application callback function to update the get the current value of the button
		 uint16_t le_value = sys_cpu_to_le16(button_state);
		//*value = sys_cpu_to_le16(lbs_cb.button_cb());
		//return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
		return bt_gatt_attr_read(conn, attr, buf, len, offset, &le_value, sizeof(le_value));
	}

	return 0;
}

static const struct bt_gatt_cpf my_button_cpf = {
    .format     = BT_GATT_CPF_FORMAT_UINT16, /* Unsigned 16-bit integer */
    .exponent   = 0,                         /* No scaling */
    .unit       = 0x2700,                    /* Unitless (generic) */
    .name_space = 1,                         /* Bluetooth SIG namespace */
    .description= 0,
};


static const struct bt_gatt_cpf my_sensor_cpf = {
    .format     = BT_GATT_CPF_FORMAT_UINT16, /* Unsigned 16-bit integer */
    .exponent   = 0,                         /* No scaling */
    .unit       = 0x2700,                    /* Unitless (generic) */
    .name_space = 1,                         /* Bluetooth SIG namespace */
    .description= 0,
};




/* LED Button Service Declaration */
BT_GATT_SERVICE_DEFINE(
	my_lbs_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_LBS),
	/* STEP 1 - Modify the Button characteristic declaration to support indication */
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON, BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ, read_button, NULL, &button_state),

	/* STEP 2 - Create and add the Client Characteristic Configuration Descriptor */
	BT_GATT_CCC(mylbsbc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CPF,
                   BT_GATT_PERM_READ,
                   bt_gatt_attr_read_cpf, NULL, (void *)&my_button_cpf),

	/* STEP 12 - Create and add the MYSENSOR characteristic and its CCCD  */
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_MYSENSOR, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL,
			       NULL, &sensor_value),

	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CPF,
                   BT_GATT_PERM_READ,
                   bt_gatt_attr_read_cpf, NULL, (void *)&my_sensor_cpf),

	BT_GATT_CCC(mylbsbc_ccc_mysensor_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);
/* A function to register application callbacks for the LED and Button characteristics  */
int my_lbs_init(struct my_lbs_cb *callbacks)
{
	if (callbacks) {
		lbs_cb.button_cb = callbacks->button_cb;
	}

	return 0;
}

/* STEP 5 - Define the function to send indications */
int my_lbs_send_button_state_indicate(struct bt_conn *conn)
{
	static uint16_t le_button_state;

	if (!indicate_enabled) {
		return -EACCES;
	}

	if (!conn) 
	{
    return -ENOTCONN; // no active connection
	}	


	le_button_state = sys_cpu_to_le16(button_state);
	memset(&ind_params, 0, sizeof(ind_params));

	ind_params.attr = &my_lbs_svc.attrs[2];
	ind_params.func = indicate_cb; // A remote device has ACKed at its host layer (ATT ACK)
	ind_params.destroy = NULL;
	ind_params.data = &le_button_state;
	ind_params.len = sizeof(le_button_state);
	return bt_gatt_indicate(conn, &ind_params);
}

/* STEP 14 - Define the function to send notifications for the MYSENSOR characteristic */
int my_lbs_send_sensor_notify(uint16_t sensor_value)
{
	if (!notify_mysensor_enabled) {
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &my_lbs_svc.attrs[5], &sensor_value, sizeof(sensor_value));
}
