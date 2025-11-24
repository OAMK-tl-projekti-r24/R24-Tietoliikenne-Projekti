#include <zephyr/kernel.h>
#include "mpu6050.h"
#include "bluetooth.h"
#include "button.h" 
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

    gpio_pin_configure_dt(&button1, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&button_cb, update_press, BIT(button1.pin));
    gpio_add_callback(button1.port, &button_cb);



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

        if (!mpu6050_calculate_accel_offsets(&dev_i2c1)) 
        {
        return 1;
        }

        if (!mpu6050_calculate_gyro_offsets(&dev_i2c1)) 
        {
        return 1;
        }

        if (my_ble_init() != 0) 
        {
        return 1;
        }

        while(1)
        {
                struct Accel m = readAccelFIFO(&dev_i2c1);
                printk("Accel X: %f, Accel Y: %f, Accel Z: %f\n", m.x, m.y, m.z);
                my_ble_send_accel(m);
                k_sleep(K_MSEC(50));
        }

        return 0;
}