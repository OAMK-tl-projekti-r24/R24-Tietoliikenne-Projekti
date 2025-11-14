#include "bluetooth.h"
static bool notify_gyro_enabled = false;


/* Callback function for client subscribing/removing subscription */
static void gyro_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_gyro_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
}



#define BT_UUID_GYRO_SERVICE_VAL  BT_UUID_128_ENCODE(0x12345678,0x1234,0x5678,0x1234,0x56789abcdef0)
#define BT_UUID_GYRO_CHAR_VAL     BT_UUID_128_ENCODE(0x12345679,0x1234,0x5678,0x1234,0x56789abcdef0)

static struct bt_uuid_128 gyro_service_uuid = BT_UUID_INIT_128(BT_UUID_GYRO_SERVICE_VAL);
static struct bt_uuid_128 gyro_char_uuid    = BT_UUID_INIT_128(BT_UUID_GYRO_CHAR_VAL);

/* */
static uint16_t gyro_value = 0;

/* -------------------------------------------------------------------------- */
/*  GATT Service                                                               */
/* -------------------------------------------------------------------------- */

BT_GATT_SERVICE_DEFINE(gyro_svc,
    BT_GATT_PRIMARY_SERVICE(&gyro_service_uuid),

    BT_GATT_CHARACTERISTIC(&gyro_char_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, &gyro_value),

    BT_GATT_CCC(gyro_ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);


int my_ble_send_gyro(uint16_t val)
{
    if (!notify_gyro_enabled) {
        return -EACCES;
    }

    return bt_gatt_notify(NULL, &gyro_svc.attrs[1], &val, sizeof(val));
}



static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("BLE: Failed to connect (%u)\n", err);
    } else {
        printk("BLE: Connected\n");
    }
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("BLE: Disconnected (reason %u)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};

int my_ble_init(void)
{
    int err = bt_enable(NULL);
    if (err) {
        printk("BLE init failed (%d)\n", err);
        return err;
    }

    printk("BLE initialized\n");

    /* Start ADV_CONN */
    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, NULL, 0, NULL, 0);
    if (err) {
        printk("Advertising failed (%d)\n", err);
        return err;
    }

    printk("Advertising started\n");

    return 0;
}
