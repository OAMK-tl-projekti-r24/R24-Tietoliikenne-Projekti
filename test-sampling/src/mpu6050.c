#include "mpu6050.h"

LOG_MODULE_DECLARE(testapp);

uint8_t fssel_input = 0; //
uint8_t accel_input = 0; //
int16_t gyro_offset_x = 0, gyro_offset_y = 0 , gyro_offset_z = 0; // Initialize offset values for the gyroscope
int16_t accel_offset_x = 0, accel_offset_y = 0, accel_offset_z = 0; // initialize offset values for the accelerometer
const struct gpio_dt_spec fifoint = GPIO_DT_SPEC_GET(DT_ALIAS(fifoint), gpios); // Interrupt pin
struct gpio_callback fifo_cb;
static struct k_work fifo_work;
const struct i2c_dt_spec *mpu_i2c_bus;

bool mpu6050_init(const struct i2c_dt_spec *target_node) 
{   
    uint8_t addr_data; // Used for reading the WHO_AM_I register (the fundamental device availability check)
    uint8_t config_data;
    uint8_t accel_data;
    uint8_t power_state; // Used for checking correct write of power state (On)
    uint8_t fssel_selection = 0; // Selection of configuration with index number of fssel_options array
    uint8_t accel_selection = 0;
    bool power = false;
    int res;

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
    res = i2c_reg_read_byte_dt(target_node, MPU6050_WHOAMI, &addr_data);
    if (res != 0) 
    {
        LOG_ERR("Failed to read WHO_AM_I register (err %d)\n", res);
        return false;
    }

    while (!power)
    {
        /* Reset entire sensor by setting DEVICE_RESET bit */
        res = i2c_reg_write_byte_dt(target_node, init.power, 0x80);
        if(res != 0)
        {
	    LOG_ERR("Failed to reset device");
        return false;
        }

        k_msleep(100);

        /* Reset subsensors and FIFO buffer */
        res = i2c_reg_write_byte_dt(target_node, init.fifo_usrctrl, 0x00);
        if(res != 0)
        {
	    LOG_ERR("Failed to disable buffers");
        return false;
        }

        k_msleep(100);

        /* Reset subsensors and FIFO buffer */
        res = i2c_reg_write_byte_dt(target_node, init.fifo_usrctrl, 0x04);
        if(res != 0)
        {
	    LOG_ERR("Failed to disable buffers");
        return false;
        }

        k_msleep(100);

        /* Set clock source as internal gyroscope Z axis for stability and power on sensor */
        res = i2c_reg_write_byte_dt(target_node, init.power, 0x03);
        if(res != 0)
        {
	    LOG_ERR("Failed to write clock source");
        return false;
        }

        k_msleep(50);

        /* Verify PWR_MGMT_1 register contents */
        res = i2c_reg_read_byte_dt(target_node, init.power, &power_state);
        if(res != 0 || power_state != 0x03) 
        {
        LOG_ERR("Register verification failed");
        return false;
        }
        power = true;
    }

    /* Write CLPF/External Frame Sync configuration */
    res = i2c_reg_write_byte_dt(target_node, init.config[0], init.config[1]);
    if(res != 0)
    {
	    LOG_ERR("Failed to write configuration");
        return false;
    }
    k_msleep(100);

    /* Write sample rate divider */
    res = i2c_reg_write_byte_dt(target_node, init.smp_rate[0], init.smp_rate[1]);
    if(res != 0)
    {
	LOG_ERR("Failed to write sample rate divider");
    return false;
    }
    k_msleep(50);

    /* Set Full Scale Range based on fssel_selection variable */
    switch(fssel_selection)
    {
    case 0:
        break;
    case 1:
        fssel_input = fssel_input | 0x08;
        break;
    case 2:
        fssel_input = fssel_input | 0x10;
        break;
    case 3:
        fssel_input = fssel_input | 0x18;
        break;
    default:
        break;
    }

    uint8_t fs_sel[2] = {shared.gyro_config,fssel_input}; 

    /* Write Full Scale Range configuration */
    res = i2c_reg_write_byte_dt(target_node, fs_sel[0], fs_sel[1]);
    if(res != 0)
    {
	    LOG_ERR("Failed to write full scale range");
        return false;
    }
    k_msleep(50);

     /* Set Full Scale Range based on fssel_selection variable */
    switch(accel_selection)
    {
    case 0:
        break;
    case 1:
        accel_input = accel_input | 0x08;
        break;
    case 2:
        accel_input = accel_input | 0x10;
        break;
    case 3:
        accel_input = accel_input | 0x18;
        break;
    default:
        break;
    }

    uint8_t accel[2] = {shared.accel_config,accel_input}; 

    /* Write Full Scale Range configuration */
    res = i2c_reg_write_byte_dt(target_node, accel[0], accel[1]);
    if(res != 0)
    {
	    LOG_ERR("Failed to write full scale range");
        return false;
    }

    i2c_reg_read_byte_dt(target_node, shared.accel_config, &accel_data);
    printk("Accelerometer config is %x\n", accel_data);


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

    /* Read register values */
    int ret = i2c_burst_read_dt(target_node, read_addr.gx, buf, 6);
    if (ret != 0)
    {
        LOG_ERR("Failed to read gyroscope values\n");
        m.x = m.y = m.z = 0;
        return m;
    }

    // Get raw values from the buffer
    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);

    // Apply software offsets
    m.x = raw_x - gyro_offset_x;
    m.y = raw_y - gyro_offset_y;
    m.z = raw_z - gyro_offset_z;

    return m;
}

/* Enable the FIFO*/
bool enable_fifo(const struct i2c_dt_spec *target_node)
{
    mpu_i2c_bus = target_node;
    int res;
            
        /* Configure FIFO to capture only accelerometer samples */
    res = i2c_reg_write_byte_dt(target_node, 0x23, 0x08);  
    if(res != 0)
    {
        LOG_ERR("FIFO configuration failed!\n");
        return false;
    }
    k_msleep(10);

    /* Enable FIFO */
    res = i2c_reg_write_byte_dt(target_node, 0x6A, 0x40);
    if(res != 0)
    {
        LOG_ERR("FIFO enable failed!\n");
        return false;
    }  


    /* Initialize work handler */
    k_work_init(&fifo_work, fifo_handler);

    /* Enable FIFO_OFLOW interrupts */
    res = i2c_reg_write_byte_dt(target_node, 0x38, 0x10);
    if(res != 0)
    {
        LOG_ERR("Failed to enable FIFO interrupt");
        return false;
    }

    return true;
}

void fifo_handler(struct k_work *work)
{
   uint8_t int_status;

    // Read INT_STATUS register
    if (i2c_reg_read_byte_dt(mpu_i2c_bus, 0x3A, &int_status) != 0) return;

    if (int_status & 0x10) {  

        // Reset FIFO
        i2c_reg_write_byte_dt(mpu_i2c_bus, init.fifo_usrctrl, 0x04); // reset FIFO
        k_msleep(1);
        i2c_reg_write_byte_dt(mpu_i2c_bus, init.fifo_usrctrl, 0x40); // re-enable FIFO
    }
}

void readFIFO(const struct device *dev,struct gpio_callback *cb,uint32_t pins)
{   
    k_work_submit(&fifo_work);
}

struct Measurement readAccelFIFO(const struct i2c_dt_spec *target_node) { 
    /* Initialize accel struct*/ 
    struct Measurement m = {0};
    uint8_t fifo_count_h, fifo_count_l;
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    int samples = 0;

    if (i2c_reg_read_byte_dt(target_node, read_addr.fifo_counth, &fifo_count_h) != 0 ||
        i2c_reg_read_byte_dt(target_node, read_addr.fifo_countl, &fifo_count_l) != 0) {
        LOG_ERR("Failed to read FIFO count");
        return m;
    }

    uint16_t fifo_count = ((uint16_t)fifo_count_h << 8) | fifo_count_l;

    while (fifo_count >= 6) {
        uint8_t fifo_data[6];
        if (i2c_burst_read_dt(target_node, read_addr.fifo_rw, fifo_data, 6) != 0) {
            LOG_ERR("Failed to read FIFO sample");
            break;
        }

        sum_x += (int16_t)((fifo_data[0] << 8) | fifo_data[1]) - accel_offset_x;
        sum_y += (int16_t)((fifo_data[2] << 8) | fifo_data[3]) - accel_offset_y;
        sum_z += (int16_t)((fifo_data[4] << 8) | fifo_data[5]) - accel_offset_z;

        fifo_count -= 6;
        samples++;
    }

    if (samples > 0) {
        m.x = sum_x / samples;
        m.y = sum_y / samples;
        m.z = sum_z / samples;
    }

    return m;
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
        if (i2c_burst_read_dt(target_node, 0x43, buf, 6) != 0) {
            LOG_ERR("Failed to read gyro sample %d\n", i);
            k_sleep(K_MSEC(2));
            continue;
        }

        sum_x += (int16_t)((buf[0] << 8) | buf[1]);
        sum_y += (int16_t)((buf[2] << 8) | buf[3]);
        sum_z += (int16_t)((buf[4] << 8) | buf[5]);

        valid_samples++;
        k_sleep(K_MSEC(50));
    }

    if (valid_samples == 0) {
        LOG_ERR("No valid gyro samples read\n");
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

    /* Notify user about calibration start via logs and LED */
    printk("Calculating MPU6050 accelerometer offsets...\n");
    gpio_pin_toggle_dt(&led0);


    /* Take samples */
    for (int i = 0; i < samples; i++) {
        if (i2c_burst_read_dt(target_node, 0x3B, buf, 6) != 0) {
            LOG_ERR("Failed to read accel sample %d\n", i);
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
        LOG_ERR("No valid accel samples read\n");
        return false;
    }

    /* calculate offset */
    accel_offset_x = sum_x / valid_samples;
    accel_offset_y = sum_y / valid_samples;

    /* Adjust Z offset to account for gravity (~1g) */
    accel_offset_z = (sum_z / valid_samples) - 16384; // assuming FS=±2g

    printk("Accel offsets calculated: X=%d Y=%d Z=%d\n",
           accel_offset_x, accel_offset_y, accel_offset_z);
    /* Notify user of calibration end via LED */
    gpio_pin_toggle_dt(&led0);       
    return true;
}

bool mpu6050_gyro_test(const struct i2c_dt_spec *target_node) 
{
    uint8_t test_value = fssel_input | 0xE0;
    uint8_t config_write;
    int registers[] = {selftest_addr.x, selftest_addr.y, selftest_addr.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    int8_t trim[3];
    int8_t str[3];
    int8_t deviation[3];
    uint8_t test_bits[3];
    uint8_t raw_bits[3];
    
    /* Enable test mode*/
    int res_test = i2c_reg_write_byte_dt(target_node, shared.gyro_config, test_value);
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
    }
    /* Disable test mode */
    int res_notest = i2c_reg_write_byte_dt(target_node, shared.gyro_config, fssel_input);
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
    }

    /* Calculate trim deviation for Gyroscope */
    for(int i=0; i < register_size; i++)
    {
        str[i] = test_bits[i] - raw_bits[i];              // Self-Test Response
        trim[i] = (25*131*pow(1.046, (test_bits[i]-1)));  // 25*131*1.046^(test-1)
        deviation[i] = ((str[i] - trim[i]) / trim[i]);    // trim deviation from Factory defaults in percents
    }
    LOG_INF("Gyroscope X deviation: %i%%, Y deviation: %i%%, Z deviation: %i%%\n", deviation[0], deviation[1], deviation[2]);

    /* Warn user if values under/over tresholds */
    if(!(deviation[0] > -14 && deviation[0] < 14) || !(deviation[1] > -14 && deviation[1] < 14) || !(deviation[2] > -14 && deviation[2] < 14))
    {
        LOG_WRN("Self-test values exceeded/went under treshold. Please check the sensor");
    }
    return true;
}


bool mpu6050_accel_test(const struct i2c_dt_spec *target_node) 
{
    uint8_t test_value = accel_input | 0xE0;
    uint8_t config_write;
    int registers[] = {selftest_addr.x, selftest_addr.y, selftest_addr.z};
    int register_size = sizeof(registers) / sizeof(registers[0]);
    uint8_t extra_bits[3];
    int8_t trim[3];
    int8_t str[3];
    int8_t deviation[3];
    uint8_t test_bits[3];
    uint8_t raw_bits[3];
    
    int res_test = i2c_reg_write_byte_dt(target_node, shared.accel_config, test_value);
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
    }
    memset(extra_bits, 0, sizeof(extra_bits)); // Empty extra bit array
    int res_notest = i2c_reg_write_byte_dt(target_node, shared.accel_config, accel_input);
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
    }


    i2c_reg_read_byte_dt(target_node, shared.accel_config, &config_write);
    printk("Accelerometer config after test is %x\n", config_write);

    /*Calculate trim deviation for Accelometer */
    for(int i=0; i < register_size; i++)
    {
        str[i] = test_bits[i] - raw_bits[i];
        trim[i] = 4096*0.34*pow((0.92/0.34),((test_bits[i]-1) / (pow(2,5)-2)));
        deviation[i] = ((str[i] - trim[i]) / trim[i]); 
    }

    LOG_INF("Accelometer X deviation: %i%%, Y deviation: %i%%, Z deviation: %i%%\n", deviation[0], deviation[1], deviation[2]);

    /* Warn user if values under/over tresholds */
    if(!(deviation[0] > -14 && deviation[0] < 14) || !(deviation[1] > -14 && deviation[1] < 14) || !(deviation[2] > -14 && deviation[2] < 14))
    {
        LOG_WRN("Self-test values exceeded/went under treshold. Please check the sensor");
    }
    return true;
}