#include <zephyr/drivers/i2c.h>
#include <stdbool.h>

#define MPU6050_ADDR 0x68
#define MPU6050_WHOAMI 0x75


/* Self-test registers */
static const struct {
    const uint8_t x;
    const uint8_t y;
    const uint8_t z;
    const uint8_t gyro_config;
} selftest_addr = {
    .x = 0x0D,
    .y = 0x0E,
    .z = 0x0F,
    .gyro_config = 0x1B
};


static const struct {
    const uint8_t config;
    const uint8_t smp_rate;
    const uint8_t power;
} init_addr = {
    .config = 0x1a,
    .smp_rate = 0x19,
    .power = 0x6B
};

static const struct {
    const uint8_t gx;
    const uint8_t gy;
    const uint8_t gz;
} read_addr = {
    .gx = 0x43,
    .gy = 0x45,
    .gz = 0x47
};

struct Measurement
{
   uint16_t x;
   uint16_t y;
   uint16_t z;
};




/*Check I2C bus availability and check device availability by reading WHO_AM_I*/
bool mpu6050_init(const struct i2c_dt_spec *target_node);

/* Read Gyroscope values */
struct Measurement readGyro(const struct i2c_dt_spec *target_node);

/* Read self-test registers for gyroscope bits */
bool mpu6050_gyro_test(const struct i2c_dt_spec *target_node);

/* Read self-test registers for accelometer bits */
bool mpu6050_accel_test(const struct i2c_dt_spec *target_node);
