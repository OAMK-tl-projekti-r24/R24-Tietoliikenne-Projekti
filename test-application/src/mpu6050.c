#include "mpu6050.h"

bool mpu6050_init(const struct i2c_dt_spec *target_node) 
{
    uint8_t addr_data;
    printk("Initializing MPU6050 driver\n");
    if (!device_is_ready(target_node->bus)) {
        printk("I2C bus not ready\n");
        return false;
    }

    int ret = i2c_reg_read_byte_dt(target_node, MPU6050_WHOAMI, &addr_data);
    if (ret != 0) 
    {
        printk("Failed to read WHO_AM_I register (err %d)\n", ret);
        return false;
    }

    printk("MPU6050 driver initialized\n");
    return true;
}



bool mpu6050_gyro_test(const struct i2c_dt_spec *target_node) 
{
    int registers[] = {selftest.x, selftest.y, selftest.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    char names[] = {'X', 'Y', 'Z'};
    uint8_t bits;
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &bits);
        if (ret != 0) {
            printk("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        bits = bits & 0x1f; // Discard first three bits
        printk("MPU6050 Gyroscope self-test register %c = 0x%x\n", names[i], bits);
    }
    return true;
}


bool mpu6050_accel_test(const struct i2c_dt_spec *target_node) 
{
    int registers[] = {selftest.x, selftest.y, selftest.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    char names[] = {'X', 'Y', 'Z'};
    uint8_t bits;
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &bits);
        if (ret != 0) {
            printk("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        bits = bits & 0xe0; // Discard last five bits
        printk("MPU6050 Accelometer self-test register %c = 0x%x\n", names[i], bits);
    }
    return true;
}