#ifndef MPU6050_H
#define MPU6050_H

#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <math.h>
#include "measurement.h"

#define MPU6050_ADDR 0x68
#define MPU6050_WHOAMI 0x75

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

extern const struct gpio_dt_spec fifoint;
extern struct gpio_callback fifo_cb;


#define MPU6050_FIFO_R_W     0x74

/* Self-test registers */
static const struct 
{
    const uint8_t x;
    const uint8_t y;
    const uint8_t z;
    const uint8_t gyro_config;
    const uint8_t accel_config;
} selftest_addr = 
{
    .x = 0x0D,
    .y = 0x0E,
    .z = 0x0F
};


/* Used for registers and values used by multiple portions of the software*/
static const struct 
{
    const uint8_t gyro_config;
    const uint8_t accel_config;
} shared = 
{
    .gyro_config = 0x1B,
    .accel_config = 0x1C
};

/* Registers and values used during initialization*/
static const struct 
{
    const uint8_t config[2];
    const uint8_t smp_rate[2];
    const uint8_t power;
    const uint8_t fifoen[2];
    const uint8_t fifo_usrctrl;
} init = 
{
    .config = {0x1a,0x00}, // Configuration, enable/disable CLPF/External Frame Sync
    .smp_rate = {0x19,0}, // Divider for sample rate, sample rate calculated with (8KHz|1KHz)/(1+smp_rate)
    .power = 0x6B, // Register address and value for power state (0x00 for on)
    .fifoen = {0x23, 0x78}, // FIFO Enable register and enable bit for Accelometer
    .fifo_usrctrl = 0x6A // FIFO USER_CTRL register (used for FIFO Ops enable and buffer reset)
};

/* Registers to be read outside of init and/or testing */
static const struct 
{
    const uint8_t gx;
    const uint8_t gy;
    const uint8_t gz;
    const uint8_t fifo_counth;
    const uint8_t fifo_countl;
    const uint8_t fifo_rw;
} read_addr = 
{
    .gx = 0x43,
    .gy = 0x45,
    .gz = 0x47,
    .fifo_counth = 0x72,
    .fifo_countl = 0x73,
    .fifo_rw = 0x74
};


/*Check I2C bus availability and check device availability by reading WHO_AM_I*/
bool mpu6050_init(const struct i2c_dt_spec *target_node);


/* Read self-test registers for gyroscope bits */
bool mpu6050_gyro_test(const struct i2c_dt_spec *target_node);

/* Read self-test registers for accelometer bits */
bool mpu6050_accel_test(const struct i2c_dt_spec *target_node);

bool fifoint_enable(const struct i2c_dt_spec *target_node);

static void fifo_handler(struct k_work *work);

void readFIFO(const struct device *dev,struct gpio_callback *cb,uint32_t pins);

/* Read Accelometer values from FIFO buffer */
struct Measurement readAccelFIFO(const struct i2c_dt_spec *target_node);

/* Calibrate the MPU6050 gyroscope by calculating mean and writing to offset registers*/
bool mpu6050_calculate_gyro_offsets(const struct i2c_dt_spec *target_node);

bool mpu6050_calculate_accel_offsets(const struct i2c_dt_spec *target_node);

#endif