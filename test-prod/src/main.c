#include <zephyr/kernel.h>
#include "mpu6050.h"
#include "bluetooth.h"
#include "button.h" 
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
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

static Data buffer[SAMPLES_PER_BURST];

K_THREAD_STACK_DEFINE(send_data_stack, STACKSIZE);
static struct k_thread send_data_thread_id;

LOG_MODULE_REGISTER(testapp);

void send_data_thread(void)
{
    while (1) {

        // 1. Fill buffer
        for (int i = 0; i < SAMPLES_PER_BURST; i++) {
            struct Measurement m = readAccelFIFO(&dev_i2c1);
            buffer[i].x = m.x;
            buffer[i].y = m.y;
            buffer[i].z = m.z;
            buffer[i].button_val = counter;
            k_busy_wait(125); // ~8 kHz sampling
        }

        // 2. Send burst
        for (int i = 0; i < SAMPLES_PER_BURST; i++) {
            uart_poll_out(data_uart, 0xAA);
            uart_poll_out(data_uart, 0x55);
            uint8_t *p = (uint8_t *)&buffer[i];
            for (int j = 0; j < sizeof(Data); j++) {
                uart_poll_out(data_uart, p[j]);
            }
        }

        // 3. Wait until next burst (optional, could be zero if timing matches)
        // k_usleep(BURST_INTERVAL_US); // not strictly needed
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

    // Initialize MPU6050
    if (!mpu6050_init(&dev_i2c1)) return 1;
    if (!mpu6050_accel_test(&dev_i2c1)) return 1;
    if (!mpu6050_calculate_accel_offsets(&dev_i2c1)) return 1;

    // Initialize button callback (optional)
    gpio_pin_configure_dt(&button1, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_INACTIVE);
    gpio_init_callback(&button_cb, update_press, BIT(button1.pin));
    gpio_add_callback(button1.port, &button_cb);

    // Start data thread
    k_thread_create(&send_data_thread_id,
                    send_data_stack,
                    STACKSIZE,
                    send_data_thread,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    return 0;
}
