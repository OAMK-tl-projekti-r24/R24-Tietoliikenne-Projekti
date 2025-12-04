#include <zephyr/kernel.h>
#include "mpu6050.h"
#include "button.h" 
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#define STACKSIZE 4096
#define PRIORITY 5
#define SAMPLES_PER_BURST 1000   // 1000 samples per burst (~100ms)
#define BURST_INTERVAL_US 125000 // 100ms

#define I2C1_NODE DT_NODELABEL(mpu6050)
static const struct i2c_dt_spec dev_i2c1 = I2C_DT_SPEC_GET(I2C1_NODE);
const struct device *data_uart = DEVICE_DT_GET(DT_NODELABEL(uart2));

extern volatile uint8_t counter;

LOG_MODULE_REGISTER(testapp);


#define FIFO_BUFFER_SIZE 510
extern volatile bool fifo_data_ready;
extern volatile uint16_t read_size;
extern uint8_t fifo_raw_buffer[FIFO_BUFFER_SIZE];
extern int16_t accel_offset_x;
extern int16_t accel_offset_y;
extern int16_t accel_offset_z;


K_THREAD_STACK_DEFINE(send_data_stack, STACKSIZE);
static struct k_thread send_data_thread_id;

void send_data_thread(void)
{
        while (1) {
        if (!fifo_data_ready) {
            k_sleep(K_MSEC(1)); 
            continue;
        }

        int samples = read_size / 6; // 6 bytes per sample
        for (int i = 0; i < samples; i++) {
            uint8_t *p = &fifo_raw_buffer[i*6];

            int16_t raw_x = (p[0] << 8) | p[1];
            int16_t raw_y = (p[2] << 8) | p[3];
            int16_t raw_z = (p[4] << 8) | p[5];

            /*Apply offsets */
            raw_x -= accel_offset_x;
            raw_y -= accel_offset_y;
            raw_z -= accel_offset_z;

            struct Measurement m;
            m.x = (float)raw_x / 16384.0f;
            m.y = (float)raw_y / 16384.0f;
            m.z = 1.0f + (float)raw_z / 16384.0f; // add 1g for gravity

            /* Package data and send via UART */
            uart_poll_out(data_uart, 0xAA);
            uart_poll_out(data_uart, 0x55);
            uint8_t *mptr = (uint8_t *)&m;
            for (int j = 0; j < sizeof(m); j++)
                uart_poll_out(data_uart, mptr[j]);
        }

        // Mark buffer as processed
        fifo_data_ready = false;
    }
} 

int main(void)
{
    
    if (!device_is_ready(data_uart)) {
        LOG_ERR("UART2 not ready");
        return 1;
    }

    struct uart_config cfg = {
        .baudrate = 921600,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
    };
    uart_configure(data_uart, &cfg); 

        // Initialize button callback (optional)
    gpio_pin_configure_dt(&button1, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_INACTIVE);
    gpio_init_callback(&button_cb, update_press, BIT(button1.pin));
    gpio_add_callback(button1.port, &button_cb);



    // Initialize MPU6050
    if (!mpu6050_init(&dev_i2c1)) return 1;
    if (!mpu6050_accel_test(&dev_i2c1)) return 1;
    if (!mpu6050_calculate_accel_offsets(&dev_i2c1)) return 1;
    if (!fifoint_enable(&dev_i2c1)) return 1;

        // Initialize MPU6050 sensor interrupt
    gpio_pin_configure_dt(&fifoint, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&fifoint, GPIO_INT_EDGE_TO_INACTIVE);
    gpio_init_callback(&fifo_cb, readFIFO, BIT(fifoint.pin));
    gpio_add_callback(fifoint.port, &fifo_cb);



    
    // Start data thread
    k_thread_create(&send_data_thread_id,
                    send_data_stack,
                    STACKSIZE,
                    send_data_thread,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT); 

    return 0;
}
