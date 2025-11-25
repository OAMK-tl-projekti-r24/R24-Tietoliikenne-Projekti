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

#define STACKSIZE 2048
#define PRIORITY 5
static struct k_work adv_work;

K_THREAD_STACK_DEFINE(send_data_stack, STACKSIZE);
static struct k_thread send_data_thread_id;

LOG_MODULE_REGISTER(testapp);


static void adv_work_handler(struct k_work *work)
{
        if (my_ble_init() != 0) 
        {
        return;
        }
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

void send_data_thread(void)
{
        while(1)
        {
                struct Accel g = readAccelFIFO(&dev_i2c1);
                struct Measurement m = readGyro(&dev_i2c1);
                printk("Accel X: %f, Accel Y: %f, Accel Z: %f\n", g.x, g.y, g.z);
                my_ble_send_accel(g);
                my_ble_send_gyro(m);
                k_sleep(K_MSEC(50));
        }
}


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
        
        
        k_work_init(&adv_work, adv_work_handler);
	advertising_start();

        k_thread_create(&send_data_thread_id,
                    send_data_stack,
                    STACKSIZE,
                    send_data_thread,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

        /*
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
        */
}

