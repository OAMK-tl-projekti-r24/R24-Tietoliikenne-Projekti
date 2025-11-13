#include "mpu6050.h"

bool mpu6050_init(const struct i2c_dt_spec *target_node) 
{
    uint8_t addr_data;
    uint8_t config_data;
    uint8_t smprate_data;
    uint8_t smprt_div[2] = {init.smp_rate,1}; // Divider for sample rate, sample rate calculated with (8KHz|1KHz)/(1+smprt_div)
    uint8_t config[2] = {init.config,0x00}; // Configuration, enable/disable CLPF/External Frame Sync
    printk("Initializing MPU6050 driver\n");
    if (!device_is_ready(target_node->bus)) {
        printk("I2C bus not ready\n");
        return false;
    }

    int res_whois = i2c_reg_read_byte_dt(target_node, MPU6050_WHOAMI, &addr_data);
    if (res_whois != 0) 
    {
        printk("Failed to read WHO_AM_I register (err %d)\n", res_whois);
        return false;
    }

    int res_config = i2c_write_read_dt(target_node, config, sizeof(config), &config_data, 1);
    if(res_config != 0 && config_data != config[0])
    {
	printk("Failed to write configuration");
    return false;
    }

    int res_smprate = i2c_write_read_dt(target_node, smprt_div, sizeof(smprt_div), &smprate_data, 1);
    if(res_smprate!= 0 && smprate_data != config[0])
    {
	printk("Failed to write configuration");
    return false;
    }

    printk("The sample rate is %i Hz\n", (8000/(1+smprt_div[1]))); // TODO: Check config bits for DLPF and change base sample rate based on it.
    printk("Data written to config: %x\n", config_data);
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