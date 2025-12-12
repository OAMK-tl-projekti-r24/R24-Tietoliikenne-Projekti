#include <zephyr/kernel.h>
#include "mpu6050.h"
#include "bluetooth.h"
#include "button.h" 
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include "confusion.h"

#define I2C1_NODE DT_NODELABEL(mpu6050)
static const struct i2c_dt_spec dev_i2c1 = I2C_DT_SPEC_GET(I2C1_NODE);
const struct device *data_uart = DEVICE_DT_GET(DT_NODELABEL(uart2));

#define STACKSIZE 2048
#define PRIORITY 5
static struct k_work adv_work;

#define SAMPLE_BUF_SIZE 128
volatile uint16_t sample_buf[SAMPLE_BUF_SIZE];
volatile int sample_count = 0;


K_THREAD_STACK_DEFINE(send_data_stack, STACKSIZE);
static struct k_thread send_data_thread_id;
extern uint8_t counter;

LOG_MODULE_REGISTER(testapp);

/* Start BLE in a separate thread */
static void adv_work_handler(struct k_work *work)
{
        if (my_ble_init() != 0) 
        {
        return;
        }
}

/* Start BLE thread */
static void advertising_start(void)
{
	k_work_submit(&adv_work);
}


/*
void send_data_thread(void *arg1, void *arg2, void *arg3)
{
    if (!device_is_ready(data_uart)) {
        printk("UART2 not ready!\n");
        return;
    }

    printk("UART2 ready, starting send_data_thread\n");

    while (1) {
        // Read a measurement from the MPU6050 sensor
        struct Measurement m = readAccelFIFO(&dev_i2c1);

        // Optional: skip invalid/empty samples
        // if (m is invalid) continue;

        // Send frame marker
        uart_poll_out(data_uart, 0xAA);
        uart_poll_out(data_uart, 0x55);

        // Send struct bytes
        uint8_t *mptr = (uint8_t *)&m;
        for (int i = 0; i < sizeof(m); i++) {
            uart_poll_out(data_uart, mptr[i]);
        }

        k_msleep(1);
    }
}*/


void send_data_thread(void *arg1, void *arg2, void *arg3)
{
        while(1)
        {
                struct Measurement m = readAccelFIFO(&dev_i2c1);
                makeOneClassificationAndUpdateConfusionMatrix(m.x, m.y, m.z, counter);
                printConfusionMatrix();

                k_msleep(500);
        }
}

int main(void)
{

    gpio_pin_configure_dt(&button1, GPIO_INPUT | GPIO_PULL_UP);
    /* Falling edge interrrupt for */
    gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb, update_press, BIT(button1.pin));
    gpio_add_callback(button1.port, &button_cb);



        if (!mpu6050_init(&dev_i2c1)) 
        {
        return 1;
        }
        /*
        if (!mpu6050_gyro_test(&dev_i2c1))
        {
        return 1;
        }

        if (!mpu6050_accel_test(&dev_i2c1)) 
        {
                return 1;
        }*/

        if (!mpu6050_calculate_accel_offsets(&dev_i2c1)) 
        {
        return 1;
        }

        if (!enable_fifo(&dev_i2c1)) 
        {
        return 1;
        }

        /*
        if (!mpu6050_calculate_gyro_offsets(&dev_i2c1)) 
        {
        return 1;
        }*/
        
        
        //k_work_init(&adv_work, adv_work_handler);
	//advertising_start();

        gpio_pin_configure_dt(&fifoint, GPIO_INPUT | GPIO_PULL_UP);
        gpio_pin_interrupt_configure_dt(&fifoint, GPIO_INT_EDGE_TO_INACTIVE);
        gpio_init_callback(&fifo_cb, readFIFO, BIT(fifoint.pin));
        gpio_add_callback(fifoint.port, &fifo_cb);

        k_thread_create(&send_data_thread_id,
                    send_data_stack,
                    STACKSIZE,
                    send_data_thread,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

}

