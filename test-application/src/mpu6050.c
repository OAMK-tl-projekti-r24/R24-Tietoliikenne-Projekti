#include "mpu6050.h"

LOG_MODULE_DECLARE(testapp);

uint8_t fssel_input;
int16_t gyro_offset_x = 0, gyro_offset_y = 0 , gyro_offset_z = 0;
int16_t accel_offset_x = 0, accel_offset_y = 0, accel_offset_z = 0;

bool mpu6050_init(const struct i2c_dt_spec *target_node) 
{
    uint8_t addr_data;
    uint8_t config_data;
    uint8_t smprate_data;
    uint8_t fssel_data; // Used to check written fssel configuration
    uint8_t power_state; // Used for checking correct write of power state (On)
    uint8_t fssel_selection = 0; // Selection of configuration with index number of fssel_options array
    uint8_t fs_sel[2] = {shared.gyro_config,fssel_input}; 

    /* Configure LED for calibration signaling (led0 for accel, led1 for gyro)*/
    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);

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


        /* Wake sensor up from sleep mode */
    int res_power = i2c_reg_write_byte_dt(target_node, init.power[0], init.power[1]);
    if(res_power != 0)
    {
	    LOG_ERR("Failed to power up");
        return false;
    }

    /* Write CLPF/External Frame Sync configuration */
    int res_config = i2c_reg_write_byte_dt(target_node, init.config[0], init.config[1]);
    if(res_config != 0)
    {
	LOG_ERR("Failed to write configuration");
    return false;
    }

    /* Write sample rate divider */
    int res_smprate = i2c_reg_write_byte_dt(target_node, init.smp_rate[0], init.smp_rate[1]);
    if(res_smprate!= 0)
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
    int res_fssel = i2c_reg_write_byte_dt(target_node, shared.gyro_config, fssel_input);
    if(res_fssel != 0)
    {
	    LOG_ERR("Failed to write full scale range");
        return false;
    }



    // Reset FIFO first
    i2c_reg_write_byte_dt(target_node, MPU6050_USER_CTRL, 0x04); // FIFO_RESET
    k_sleep(K_MSEC(10)); // short delay
    // Enable FIFO
    i2c_reg_write_byte_dt(target_node, MPU6050_USER_CTRL, 0x40); // FIFO_EN only
    // Enable accel in FIFO
    i2c_reg_write_byte_dt(target_node, MPU6050_FIFO_EN, 0x78);


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

    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);

    // Apply software offsets
    m.x = raw_x - gyro_offset_x;
    m.y = raw_y - gyro_offset_y;
    m.z = raw_z - gyro_offset_z;

    return m;
}

struct Accel readAccelFIFO(const struct i2c_dt_spec *node)
{
    struct Accel g = {0.0f, 0.0f, 0.0f};
    uint8_t buf[6];
    uint8_t fifo_h, fifo_l;
    uint16_t fifo_count;

    // Read FIFO_COUNT (high + low)
    if (i2c_reg_read_byte_dt(node, MPU6050_FIFO_COUNTH, &fifo_h) != 0) return g;
    if (i2c_reg_read_byte_dt(node, MPU6050_FIFO_COUNTL, &fifo_l) != 0) return g;
    fifo_count = ((uint16_t)fifo_h << 8) | fifo_l;

    if (fifo_count < 6) return g;
    fifo_count -= fifo_count % 6;

    // Read 6 bytes from FIFO if enough data
    if (fifo_count >= 6) {
        if (i2c_burst_read_dt(node, MPU6050_FIFO_R_W, buf, 6) == 0) {
            int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]) - accel_offset_x;
            int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]) - accel_offset_y;
            int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]) - accel_offset_z;

            // Convert to g (FS ±2g -> 1g = 16384)
            g.x = (float)raw_x / 16384.0f;
            g.y = (float)raw_y / 16384.0f;
            g.z = (float)raw_z / 16384.0f;
        }
    }

    return g;
}

bool mpu6050_calculate_gyro_offsets(const struct i2c_dt_spec *target_node)
{
    const int samples = 3000;
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    uint8_t buf[6];
    int valid_samples = 0;

    printk("Calculating MPU6050 gyro offsets...\n");
    gpio_pin_toggle_dt(&led1);

    for (int i = 0; i < samples; i++) {
        if (i2c_burst_read_dt(target_node, 0x43 /* GYRO_XOUT_H */, buf, 6) != 0) {
            printk("Failed to read gyro sample %d\n", i);
            k_sleep(K_MSEC(2));
            continue;
        }

        sum_x += (int16_t)((buf[0] << 8) | buf[1]);
        sum_y += (int16_t)((buf[2] << 8) | buf[3]);
        sum_z += (int16_t)((buf[4] << 8) | buf[5]);

        valid_samples++;
        k_sleep(K_MSEC(2));
    }

    if (valid_samples == 0) {
        printk("No valid gyro samples read\n");
        return false;
    }

    gyro_offset_x = sum_x / valid_samples;
    gyro_offset_y = sum_y / valid_samples;
    gyro_offset_z = sum_z / valid_samples;

    printk("Gyro offsets calculated: X=%d Y=%d Z=%d\n",
           gyro_offset_x, gyro_offset_y, gyro_offset_z);

    gpio_pin_toggle_dt(&led1);
    return true;
}

bool mpu6050_calculate_accel_offsets(const struct i2c_dt_spec *target_node)
{
    const int samples = 3000;
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    uint8_t buf[6];
    int valid_samples = 0;

    printk("Calculating MPU6050 accelerometer offsets...\n");
    gpio_pin_toggle_dt(&led0);

    for (int i = 0; i < samples; i++) {
        if (i2c_burst_read_dt(target_node, 0x3B /* ACCEL_XOUT_H */, buf, 6) != 0) {
            printk("Failed to read accel sample %d\n", i);
            k_sleep(K_MSEC(2));
            continue;
        }

        sum_x += (int16_t)((buf[0] << 8) | buf[1]);
        sum_y += (int16_t)((buf[2] << 8) | buf[3]);
        sum_z += (int16_t)((buf[4] << 8) | buf[5]);

        valid_samples++;
        k_sleep(K_MSEC(2));
    }

    if (valid_samples == 0) {
        printk("No valid accel samples read\n");
        return false;
    }

    accel_offset_x = sum_x / valid_samples;
    accel_offset_y = sum_y / valid_samples;

    // Adjust Z offset to account for gravity (~1g)
    accel_offset_z = (sum_z / valid_samples) - 16384; // assuming FS=±2g

    printk("Accel offsets calculated: X=%d Y=%d Z=%d\n",
           accel_offset_x, accel_offset_y, accel_offset_z);

    gpio_pin_toggle_dt(&led0);       
    return true;
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
    int res_test = i2c_reg_write_byte_dt(target_node, test_config[0], test_config[1]);
    if(res_test != 0)
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
    test_config[1] = fssel_input & ~0xE6;
    int res_notest = i2c_reg_write_byte_dt(target_node, test_config[0], test_config[1]);
    if(res_notest != 0)
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
    if((-14>deviation[0]>14) | (-14>deviation[1]>14) | (-14>deviation[2]>14))
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
    
    int res_test = i2c_reg_write_byte_dt(target_node, test_config[0], test_config[1]);
    if(res_test != 0)
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
    test_config[1] = fssel_input & ~0xE6;
    int res_notest = i2c_reg_write_byte_dt(target_node, test_config[0], test_config[1]);
    if(res_notest != 0)
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