#include <zephyr/kernel.h>
#include "mpu6050.h"
#include "bluetooth.h" 
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

#define I2C1_NODE DT_NODELABEL(mpu6050)
static const struct i2c_dt_spec dev_i2c1 = I2C_DT_SPEC_GET(I2C1_NODE);

LOG_MODULE_REGISTER(testapp);

int main(void)
{

        if (!mpu6050_init(&dev_i2c1)) 
        {
        return 1;
        }

        if (!mpu6050_gyro_test(&dev_i2c1))
        {
        return 1;
        }

        if (!mpu6050_accel_test(&dev_i2c1)) 
        {
                return 1;
        }

        if (my_ble_init() != 0) 
        {
        return 1;
        }

        while(1)
        {
                struct Measurement m = readGyro(&dev_i2c1);
                printk("Gyro X: %d, Gyro Y: %d, Gyro Z: %d\n", m.x, m.y, m.z);
                my_ble_send_gyro(m);
                k_sleep(K_MSEC(1000));
        }

        return 0;
}