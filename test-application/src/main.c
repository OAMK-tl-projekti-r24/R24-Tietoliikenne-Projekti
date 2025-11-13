#include <zephyr/kernel.h>
#include "mpu6050.h" 
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
LOG_MODULE_REGISTER(foo);

#define I2C1_NODE DT_NODELABEL(mpu6050)
static const struct i2c_dt_spec dev_i2c1 = I2C_DT_SPEC_GET(I2C1_NODE);

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

        return 0;
}
