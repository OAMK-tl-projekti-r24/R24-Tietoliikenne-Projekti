#include "mpu6050.h"

LOG_MODULE_DECLARE(testapp);

uint8_t fssel_input;

bool mpu6050_init(const struct i2c_dt_spec *target_node) 
{
    uint8_t addr_data;
    uint8_t config_data;
    uint8_t smprate_data;
    uint8_t fssel_data; // Used to check written fssel configuration
    uint8_t power_state; // Used for checking correct write of power state (On)
    uint8_t fssel_selection = 0; // Selection of configuration with index number of fssel_options array
    uint8_t fs_sel[2] = {shared.gyro_config,fssel_input}; 

    /* Check microcontroller I2C bus */
    printk("Initializing MPU6050 driver\n");
    if (!device_is_ready(target_node->bus)) {
        LOG_ERR("I2C bus not ready\n");
        return false;
    }

    /* Check MPU6050 by reading WHO_AM_I register */
    int res_whois = i2c_reg_read_byte_dt(target_node, MPU6050_WHOAMI, &addr_data);
    if (res_whois != 0) 
    {
        LOG_ERR("Failed to read WHO_AM_I register (err %d)\n", res_whois);
        return false;
    }

    /* Write CLPF/External Frame Sync configuration */
    int res_config = i2c_write_read_dt(target_node, init.config, sizeof(init.config), &config_data, 1);
    if(res_config != 0 && config_data != init.config[0])
    {
	LOG_ERR("Failed to write configuration");
    return false;
    }

    /* Write sample rate divider */
    int res_smprate = i2c_write_read_dt(target_node, init.smp_rate, sizeof(init.smp_rate), &smprate_data, 1);
    if(res_smprate!= 0 && smprate_data != init.smp_rate[1])
    {
	LOG_ERR("Failed to write sample rate divider");
    return false;
    }

    /* Set Full Scale Range based on fssel_selection variable */
    switch(fssel_selection)
    {
    case 0:
        fssel_input = init.fssel_options[0];
        break;
    case 1:
        fssel_input = init.fssel_options[1];
        break;
    case 2:
        fssel_input = init.fssel_options[2];
        break;
    case 3:
        fssel_input = init.fssel_options[3];
        break;
    default:
        break;
    }

    /* Write Full Scale Range configuration */
    int res_fssel = i2c_write_read_dt(target_node, fs_sel, sizeof(fs_sel), &fssel_data, 1);
    if(res_fssel != 0 && fssel_data != fs_sel[0])
    {
	    LOG_ERR("Failed to write full scale range");
        return false;
    }

    /* Wake sensor up from sleep mode */
    int res_power = i2c_write_read_dt(target_node, init.power, sizeof(init.power), &power_state, 1);
    if(res_power != 0 && power_state != init.power[0])
    {
	    LOG_ERR("Failed to power up");
        return false;
    }

    /* Log written configuration */
    LOG_INF("The full scale range is ±%i°/s\n", 250*(fssel_selection+1));
    if((config_data & 0x07) != 0)
    {
        LOG_INF("The sample rate is %i Hz\n", (8000/(1+init.smp_rate[1]))); // TODO: Check config bits for DLPF and change base sample rate based on it.
    }
    else if((config_data & 0x07) == 0)
    {
        LOG_INF("The sample rate is %i Hz\n", (1000/(1+init.smp_rate[1]))); // TODO: Check config bits for DLPF and change base sample rate based on it.
    }
    LOG_INF("Data written to config: %x\n", config_data);
    printk("MPU6050 driver initialized\n");
    return true;
}


struct Measurement readGyro(const struct i2c_dt_spec *target_node)
{
    struct Measurement m;
    uint8_t buf[6];

    /* Read register values (TODO FIFO buffer) */
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
    uint8_t test_config[2] = {shared.gyro_config,(fssel_input | 0xE0)};
    uint8_t config_write;
    int registers[] = {selftest_addr.x, selftest_addr.y, selftest_addr.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    char names[] = {'X', 'Y', 'Z'};
    int8_t trim[3];
    int8_t str[3];
    int8_t deviation[3];
    uint8_t test_bits[3];
    uint8_t raw_bits[3];
    
    /* Enable test mode*/
    int res_test = i2c_write_read_dt(target_node, test_config, sizeof(test_config), &config_write, 1);
    if(res_test != 0 && config_write != test_config[1])
    {
	    LOG_ERR("Failed to enable tests!");
        return false;
    }
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &raw_bits[i]);
        if (ret != 0) {
            LOG_ERR("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        raw_bits[i] = raw_bits[i] & 0x1f; // Discard first three bits
        LOG_INF("MPU6050 Gyroscope self-test register %c = 0x%x\n", names[i], raw_bits[i]);
    }
    /* Disable test mode */
    test_config[1] = (fssel_input &= ~0xE6);
    int res_notest = i2c_write_read_dt(target_node, test_config, sizeof(test_config), &config_write, 1);
    if(res_notest != 0 && config_write != test_config[1])
    {
	    LOG_ERR("Failed to disable tests!");
        return false;
    }
    k_sleep(K_MSEC(250));
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &test_bits[i]);
        if (ret != 0) {
            LOG_ERR("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        test_bits[i] = test_bits[i] & 0x1f; // Discard first three bits
        LOG_INF("MPU6050 Gyroscope self-test (disabled) register %c = 0x%x\n", names[i], raw_bits[i]);
    }

    /* Calculate trim deviation for Gyroscope */
    for(int i=0; i < register_size; i++)
    {
        str[i] = test_bits[i] - raw_bits[i];              // Self-Test Response
        trim[i] = (25*131*pow(1.046, (test_bits[i]-1)));  // 25*131*1.046^(test-1)
        deviation[i] = ((str[i] - trim[i]) / trim[i]);    // trim deviation from Factory defaults in percents
    }
    LOG_INF("Gyroscope X deviation: %i %, Y deviation: %i %, Z deviation: %i %\n", deviation[0], deviation[1], deviation[2]);

    /* Warn user if values under/over tresholds */
    if(-14>deviation[0]>14 | -14>deviation[1]>14 | -14>deviation[2]>14)
    {
        LOG_WRN("Self-test values exceeded/went under treshold. Please check the sensor");
    }
    return true;
}


bool mpu6050_accel_test(const struct i2c_dt_spec *target_node) 
{
    uint8_t test_config[2] = {shared.accel_config,(fssel_input | 0xE0)};
    uint8_t config_write;
    int registers[] = {selftest_addr.x, selftest_addr.y, selftest_addr.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    char names[3] = {'X', 'Y', 'Z'};
    uint8_t extra_bits[3];
    int8_t trim[3];
    int8_t str[3];
    int8_t deviation[3];
    uint8_t test_bits[3];
    uint8_t raw_bits[3];
    
    int res_test = i2c_write_read_dt(target_node, test_config, sizeof(test_config), &config_write, 1);
    if(res_test != 0 && config_write != test_config[1])
    {
	    LOG_ERR("Failed to enable tests!");
        return false;
    }
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &test_bits[i]);
        if (ret != 0) {
            LOG_ERR("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }

        int extra_ret = i2c_reg_read_byte_dt(target_node, 0x10, &extra_bits[i]);
        if (extra_ret != 0) {
            LOG_ERR("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        test_bits[i] &= 0xE0; // Remove last five bits
        extra_bits[i] = (extra_bits[i] >> (6 - i*2)) & 0x03; // extract extra bits
        extra_bits[i] <<= 2; // set extra bits to LSB
        test_bits[i] = (test_bits[i] >> 3) | extra_bits[i]; // merge bits
        LOG_INF("MPU6050 Accelometer self-test register %c = 0x%x\n", names[i], raw_bits[i]);
    }
    memset(extra_bits, 0, sizeof(extra_bits)); // Empty extra bit array
    test_config[1] = (fssel_input &= ~0xE6);
    int res_notest = i2c_write_read_dt(target_node, test_config, sizeof(test_config), &config_write, 1);
    if(res_notest != 0 && config_write != test_config[1])
    {
	    LOG_ERR("Failed to disable tests!");
        return false;
    }
    k_sleep(K_MSEC(250));
    for(int i=0; i < register_size; i++)
    {
        int ret = i2c_reg_read_byte_dt(target_node, registers[i], &raw_bits[i]);
        if (ret != 0) {
            LOG_ERR("Failed to read Self-test register (err %d)\n", ret);
            return false;
        }
        raw_bits[i] &= 0xE0; // Remove last five bits
        extra_bits[i] = (extra_bits[i] >> (6 - i*2)) & 0x03; // extract extra bits
        extra_bits[i] <<= 2; // set extra bits to LSB
        raw_bits[i] = (raw_bits[i] >> 3) | extra_bits[i]; // merge bits
        LOG_INF("MPU6050 Accelometer self-test (disabled) register %c = 0x%x\n", names[i], raw_bits[i]);
    }


    /*Calculate trim deviation for Accelometer */
    for(int i=0; i < register_size; i++)
    {
        str[i] = test_bits[i] - raw_bits[i];
        trim[i] = 4096*0.34*pow((0.92/0.34),((test_bits[i]-1) / (pow(2,5)-2)));
        deviation[i] = ((str[i] - trim[i]) / trim[i]); 
    }

    LOG_INF("Accelometer X deviation: %i %, Y deviation: %i %, Z deviation: %i %\n", deviation[0], deviation[1], deviation[2]);

    /* Warn user if values under/over tresholds */
    if(-14>deviation[0]>14 | -14>deviation[1]>14 | -14>deviation[2]>14)
    {
        LOG_WRN("Self-test values exceeded/went under treshold. Please check the sensor");
    }
    return true;
}