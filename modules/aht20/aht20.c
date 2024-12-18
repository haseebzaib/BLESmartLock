#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(aht20, CONFIG_CUSTOM_AHT20_LOG_LEVEL);

struct aht20_data {
	const struct device *i2c;
	float temperature;
	float humidity;
};

struct aht20_config {
	struct i2c_dt_spec i2c_spec;
};

static int aht20_init(const struct device *dev)
{
	struct aht20_data *data = dev->data;
	const struct aht20_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c_spec.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
	data->i2c = cfg->i2c_spec.bus;

	/* Soft reset (example command) */
	uint8_t cmd_reset[1] = {0xBA};
	int ret = i2c_write(data->i2c, cmd_reset, sizeof(cmd_reset),
			    cfg->i2c_spec.addr);
	if (ret < 0) {
		LOG_ERR("Failed to reset AHT20");
		return ret;
	}
	k_sleep(K_MSEC(20));

	LOG_INF("AHT20 init done");
	return 0;
}

static int aht20_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct aht20_data *data = dev->data;
	const struct aht20_config *cfg = dev->config;
	int ret;

	/* Trigger measurement */
	uint8_t cmd_measure[3] = {0xAC, 0x33, 0x00};
	ret = i2c_write(data->i2c, cmd_measure, sizeof(cmd_measure),
			cfg->i2c_spec.addr);
	if (ret < 0) {
		LOG_ERR("Failed to start measurement");
		return ret;
	}
	k_sleep(K_MSEC(80)); /* wait for measurement to complete */

	/* Read data */
	uint8_t raw_data[6];
	ret = i2c_read(data->i2c, raw_data, sizeof(raw_data), cfg->i2c_spec.addr);
	if (ret < 0) {
		LOG_ERR("Failed to read measurement data");
		return ret;
	}

	/* Extract humidity (20 bits) */
	uint32_t hum_raw = ((uint32_t)raw_data[1] << 12) |
			   ((uint32_t)raw_data[2] << 4)  |
			   (raw_data[3] >> 4);

	/* Extract temperature (20 bits) */
	uint32_t temp_raw = ((raw_data[3] & 0x0F) << 16) |
			    ((uint32_t)raw_data[4] << 8)  |
			    ((uint32_t)raw_data[5]);

	data->humidity    = ((float)hum_raw  / 1048576.0f) * 100.0f;
	data->temperature = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;

	return 0;
}

static int aht20_channel_get(const struct device *dev,
			     enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct aht20_data *data = dev->data;

	if (chan == SENSOR_CHAN_HUMIDITY) {
		val->val1 = (int32_t)data->humidity;
		val->val2 = (int32_t)((data->humidity - val->val1) * 1000000);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		val->val1 = (int32_t)data->temperature;
		val->val2 = (int32_t)((data->temperature - val->val1) * 1000000);
	} else {
		return -ENOTSUP;
	}
	return 0;
}

static const struct sensor_driver_api aht20_api = {
	.sample_fetch = aht20_sample_fetch,
	.channel_get  = aht20_channel_get,
};

#define AHT20_INST(n)								\
	static struct aht20_data aht20_data_##n;					\
	static const struct aht20_config aht20_config_##n = {			\
		.i2c_spec = I2C_DT_SPEC_INST_GET(n),				\
	};									\
	DEVICE_DT_INST_DEFINE(n, aht20_init, NULL, &aht20_data_##n,		\
			      &aht20_config_##n, POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY, &aht20_api);

DT_INST_FOREACH_STATUS_OKAY(AHT20_INST)
