#include "mpu6050.h"

LOG_MODULE_DECLARE(testapp);

bool mpu6050_init(const struct i2c_dt_spec *target_node) 
{
    uint8_t addr_data;
    uint8_t config_data;
    uint8_t smprate_data;
    uint8_t fssel_data;
    uint8_t fssel_input;
    uint8_t power_state;
    uint8_t fssel_options[3] = {0x00,0x08,0x10,0x18};
    uint8_t fssel_selection = 0;
    uint8_t smprt_div[2] = {init_addr.smp_rate,1}; // Divider for sample rate, sample rate calculated with (8KHz|1KHz)/(1+smprt_div)
    uint8_t config[2] = {init_addr.config,0x00}; // Configuration, enable/disable CLPF/External Frame Sync
    uint8_t fs_sel[2] = {selftest_addr.gyro_config,fssel_input}; 
    uint8_t power[2] = {init_addr.power,0x00};

    printk("Initializing MPU6050 driver\n");
    if (!device_is_ready(target_node->bus)) {
        LOG_ERR("I2C bus not ready\n");
        return false;
    }

    int res_whois = i2c_reg_read_byte_dt(target_node, MPU6050_WHOAMI, &addr_data);
    if (res_whois != 0) 
    {
        LOG_ERR("Failed to read WHO_AM_I register (err %d)\n", res_whois);
        return false;
    }

    int res_config = i2c_write_read_dt(target_node, config, sizeof(config), &config_data, 1);
    if(res_config != 0 && config_data != config[0])
    {
	LOG_ERR("Failed to write configuration");
    return false;
    }

    int res_smprate = i2c_write_read_dt(target_node, smprt_div, sizeof(smprt_div), &smprate_data, 1);
    if(res_smprate!= 0 && smprate_data != config[0])
    {
	LOG_ERR("Failed to write sample rate divider");
    return false;
    }

    switch(fssel_selection)
    {
    case 0:
        fssel_input = fssel_options[0];
        break;
    case 1:
        fssel_input = fssel_options[1];
        break;
    case 2:
        fssel_input = fssel_options[2];
        break;
    case 3:
        fssel_input = fssel_options[3];
        break;
    default:
        break;
    }

    int res_fssel = i2c_write_read_dt(target_node, fs_sel, sizeof(fs_sel), &fssel_data, 1);
    if(res_fssel != 0 && fssel_data != fs_sel[0])
    {
	    LOG_ERR("Failed to write full scale range");
        return false;
    }

    int res_power = i2c_write_read_dt(target_node, power, sizeof(power), &power_state, 1);
    if(res_power != 0 && power_state != power[0])
    {
	    LOG_ERR("Failed to power up");
        return false;
    }

    
    LOG_INF("The full scale range is ±%i°/s\n", 250*(fssel_selection+1));
    if((config_data & 0x07) != 0)
    {
        LOG_INF("The sample rate is %i Hz\n", (8000/(1+smprt_div[1]))); // TODO: Check config bits for DLPF and change base sample rate based on it.
    }
    else if((config_data & 0x07) == 0)
    {
        LOG_INF("The sample rate is %i Hz\n", (1000/(1+smprt_div[1]))); // TODO: Check config bits for DLPF and change base sample rate based on it.
    }
    LOG_INF("Data written to config: %x\n", config_data);
    printk("MPU6050 driver initialized\n");
    return true;
}


struct Measurement readGyro(const struct i2c_dt_spec *target_node)
{
    struct Measurement m;
    uint8_t buf[6];

    int ret = i2c_burst_read_dt(target_node, read_addr.gx, buf, 6);
    if (ret != 0)
    {
        printk("Failed to read gyroscope values\n");
        m.x = m.y = m.z = 0;
        return m;
    }

    m.x = (buf[0] << 8) | buf[1];
    m.y = (buf[2] << 8) | buf[3];
    m.z = (buf[4] << 8) | buf[5];

    return m;
}


bool mpu6050_gyro_test(const struct i2c_dt_spec *target_node) 
{
    int registers[] = {selftest_addr.x, selftest_addr.y, selftest_addr.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    char names[] = {'X', 'Y', 'Z'};
    uint8_t bits;
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &bits);
        if (ret != 0) {
            LOG_ERR("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        bits = bits & 0x1f; // Discard first three bits
        LOG_INF("MPU6050 Gyroscope self-test register %c = 0x%x\n", names[i], bits);
    }
    return true;
}


bool mpu6050_accel_test(const struct i2c_dt_spec *target_node) 
{
    int registers[] = {selftest_addr.x, selftest_addr.y, selftest_addr.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    char names[] = {'X', 'Y', 'Z'};
    uint8_t bits;
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &bits);
        if (ret != 0) {
            LOG_ERR("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        bits = bits & 0xe0; // Discard last five bits
        LOG_INF("MPU6050 Accelometer self-test register %c = 0x%x\n", names[i], bits);
    }
    return true;
}