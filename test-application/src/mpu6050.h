#include <zephyr/drivers/i2c.h>
#include <stdbool.h>

#define MPU6050_ADDR 0x68
#define MPU6050_WHOAMI 0x75


/* Self-test registers */
static const struct {
    const uint8_t x;
    const uint8_t y;
    const uint8_t z;
} selftest = {
    .x = 0x0D,
    .y = 0x0E,
    .z = 0x0F
};





/*Check I2C bus availability and check device availability by reading WHO_AM_I*/
bool mpu6050_init(const struct i2c_dt_spec *target_node);

/* Read self-test registers for gyroscope bits */
bool mpu6050_gyro_test(const struct i2c_dt_spec *target_node);

/* Read self-test registers for accelometer bits */
bool mpu6050_accel_test(const struct i2c_dt_spec *target_node);
