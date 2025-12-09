#include "bluetooth.h"
#include "button.h"

LOG_MODULE_DECLARE(testapp);

static bool notify_gyro_enabled = false;
static bool notify_accel_enabled = false;
static bool notify_button_enabled = false;
extern uint8_t counter;


#define BT_UUID_SERVICE_VAL  BT_UUID_128_ENCODE(0x12345678,0x1234,0x5678,0x1234,0x56789abcdef0)
#define BT_UUID_GYRO_CHAR_VAL     BT_UUID_128_ENCODE(0x12345679,0x1234,0x5678,0x1234,0x56789abcdef0)
#define BT_UUID_ACCEL_CHAR_VAL     BT_UUID_128_ENCODE(0x12345676,0x1234,0x5678,0x1234,0x56789abcdef0)
#define BT_UUID_BUTTON_CHAR_VAL     BT_UUID_128_ENCODE(0x12345675,0x1234,0x5678,0x1234,0x56789abcdef0)

static struct bt_uuid_128 gyro_service_uuid = BT_UUID_INIT_128(BT_UUID_SERVICE_VAL);
static struct bt_uuid_128 gyro_char_uuid    = BT_UUID_INIT_128(BT_UUID_GYRO_CHAR_VAL);
static struct bt_uuid_128 accel_char_uuid    = BT_UUID_INIT_128(BT_UUID_ACCEL_CHAR_VAL);
static struct bt_uuid_128 button_char_uuid    = BT_UUID_INIT_128(BT_UUID_BUTTON_CHAR_VAL);

static void gyro_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void accel_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void button_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

/* values for characteristics */
static uint16_t gyro_value = 0;
//static uint16_t accel_value = 0;
static uint8_t accel_value[12];
static uint8_t button_value = 0;


/* GATT service definition and characteristic definitions */
BT_GATT_SERVICE_DEFINE(my_svc,
    BT_GATT_PRIMARY_SERVICE(&gyro_service_uuid),

    /* GYRO characteristic */
    BT_GATT_CHARACTERISTIC(&gyro_char_uuid.uuid,
            BT_GATT_CHRC_NOTIFY,
            BT_GATT_PERM_NONE,
            NULL, NULL, &gyro_value),

    BT_GATT_CCC(gyro_ccc_cfg_changed,
            BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CUD,
            BT_GATT_PERM_READ,
            NULL, NULL, "Gyroscope Data"),

    /* ACCEL characteristic */
    BT_GATT_CHARACTERISTIC(&accel_char_uuid.uuid,
            BT_GATT_CHRC_NOTIFY,
            BT_GATT_PERM_NONE,
            NULL, NULL, &accel_value),

    BT_GATT_CCC(accel_ccc_cfg_changed,
            BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CUD,
            BT_GATT_PERM_READ,
            NULL, NULL, "Accelerometer Data"),

        /* Button characteristic */
    BT_GATT_CHARACTERISTIC(&button_char_uuid.uuid,
            BT_GATT_CHRC_NOTIFY,
            BT_GATT_PERM_NONE,
            NULL, NULL, &button_value),

    BT_GATT_CCC(button_ccc_cfg_changed,
            BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CUD,
            BT_GATT_PERM_READ,
            NULL, NULL, "Button Data")        
);

/* Callback functions for client subscribing/removing subscription */
static void gyro_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_gyro_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
}

static void accel_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_accel_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
}

static void button_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_button_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
    if (notify_button_enabled) {
        button_value = counter;
        bt_gatt_notify(NULL, &my_svc.attrs[10], &button_value, sizeof(button_value));
    }
}

/* Send Gyroscope values */
int my_ble_send_gyro(struct Measurement val)
{
    if (!notify_gyro_enabled) {
        return -EACCES;
    }


    /* Convert to Big-Endian (values make more sense)*/
    val.x = sys_cpu_to_be16(val.x);
    val.y = sys_cpu_to_be16(val.y);
    val.z = sys_cpu_to_be16(val.z);

    return bt_gatt_notify(NULL, &my_svc.attrs[2], &val, sizeof(val));
}

/* Send Accelometer values (float, no burst) */
int my_ble_send_accel(struct Accel val)
{
    if (!notify_accel_enabled) {
        return -EACCES;
    }

    return bt_gatt_notify(NULL, &my_svc.attrs[6], &val, sizeof(val));
}

/* Send button value */
int my_ble_send_button(uint8_t value)
{
    if (!notify_button_enabled) {
        return -EACCES;
    }

    button_value = value;

    return bt_gatt_notify(NULL, &my_svc.attrs[10], &button_value, sizeof(button_value));
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("BLE: Failed to connect (%u)\n", err);
    } else {
        LOG_INF("BLE: Connected\n");
        static struct bt_le_conn_param conn_param = {
            .interval_min = 12,   // 15 ms
            .interval_max = 12,   // 15 ms
            .latency = 0,
            .timeout = 400,
        };

        bt_conn_le_param_update(conn, &conn_param);

    }
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("BLE: Disconnected (reason %u)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};

int my_ble_init(void)
{
    printk("BLE initializing\n");

    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("BLE init failed (%d)\n", err);
        return err;
    }

    /* Connectable BLE GATT parameters */
    static const struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
        .options = BT_LE_ADV_OPT_USE_NAME| BT_LE_ADV_OPT_CONNECTABLE,  
    };

    /* Start advertising with aforementioned parameters */
    err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed (%d)", err);
        return err;
    }


    LOG_INF("Advertising started\n");
    printk("BLE initialized\n");


    return 0;
}
