#define DT_DRV_COMPAT zephyr_aht20

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(AHT20, CONFIG_CUSTOM_AHT20_LOG_LEVEL);

/* AHT20 Commands */
#define AHT20_CMD_INIT           0xBE  /* Initialization command */
#define AHT20_CMD_MEASURE        0xAC  /* Trigger measurement */
#define AHT20_CMD_RESET          0xBA  /* Soft reset */

/* Delay after reset */
#define AHT20_RESET_DELAY_MS     20
#define AHT20_MEASURE_DELAY_MS   80

struct aht20_config {
    struct i2c_dt_spec i2c; /* I2C device specification */
};

struct aht20_data {
    float temperature;
    float humidity;
};

/* I2C Write Command */
static int aht20_write_cmd(const struct device *dev, uint8_t cmd)
{
    const struct aht20_config *cfg = dev->config;
    return i2c_write_dt(&cfg->i2c, &cmd, 1);
}

/* I2C Read Measurement Data */
static int aht20_read_data(const struct device *dev, uint8_t *data, size_t len)
{
    const struct aht20_config *cfg = dev->config;
    return i2c_read_dt(&cfg->i2c, data, len);
}

/* Fetch Sensor Data */
static int aht20_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct aht20_data *data = dev->data;

    uint8_t cmd_measure[3] = {AHT20_CMD_MEASURE, 0x33, 0x00};
    uint8_t raw_data[6];
    int ret;

    /* Trigger measurement */
    ret = i2c_write_dt(&((const struct aht20_config *)dev->config)->i2c, cmd_measure, 3);
    if (ret < 0) {
        LOG_ERR("Failed to send measurement command");
        return ret;
    }

    k_sleep(K_MSEC(AHT20_MEASURE_DELAY_MS));

    /* Read measurement data */
    ret = aht20_read_data(dev, raw_data, sizeof(raw_data));
    if (ret < 0) {
        LOG_ERR("Failed to read measurement data");
        return ret;
    }

    /* Parse humidity and temperature data */
    uint32_t hum_raw = ((uint32_t)raw_data[1] << 12) | ((uint32_t)raw_data[2] << 4) | (raw_data[3] >> 4);
    uint32_t temp_raw = (((uint32_t)raw_data[3] & 0x0F) << 16) | ((uint32_t)raw_data[4] << 8) | raw_data[5];

    data->humidity = ((float)hum_raw / 1048576.0f) * 100.0f;   /* Convert to %RH */
    data->temperature = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f; /* Convert to °C */

    return 0;
}

/* Retrieve Channel Data */
static int aht20_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
    struct aht20_data *data = dev->data;

    if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
        val->val1 = (int32_t)data->temperature;
        val->val2 = (data->temperature - val->val1) * 1000000;
    } else if (chan == SENSOR_CHAN_HUMIDITY) {
        val->val1 = (int32_t)data->humidity;
        val->val2 = (data->humidity - val->val1) * 1000000;
    } else {
        return -ENOTSUP;
    }

    return 0;
}

/* Initialize AHT20 Sensor */
static int aht20_init(const struct device *dev)
{
    int ret;

    const struct aht20_config *cfg = dev->config;

    if (!device_is_ready(cfg->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    /* Send soft reset */
    ret = aht20_write_cmd(dev, AHT20_CMD_RESET);
    if (ret < 0) {
        LOG_ERR("Failed to reset AHT20 sensor");
        return ret;
    }

    k_sleep(K_MSEC(AHT20_RESET_DELAY_MS));
    LOG_INF("AHT20 initialized");

    return 0;
}

/* Define Driver API */
static const struct sensor_driver_api aht20_api = {
    .sample_fetch = aht20_sample_fetch,
    .channel_get = aht20_channel_get,
};

/* Device Instance Definition */
#define AHT20_DEFINE(inst)                                           \
    static struct aht20_data aht20_data_##inst;                      \
    static const struct aht20_config aht20_config_##inst = {         \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                           \
    };                                                               \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, aht20_init, NULL,             \
                                 &aht20_data_##inst,                 \
                                 &aht20_config_##inst, POST_KERNEL,  \
                                 CONFIG_SENSOR_INIT_PRIORITY,        \
                                 &aht20_api);

DT_INST_FOREACH_STATUS_OKAY(AHT20_DEFINE)
