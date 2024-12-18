#define DT_DRV_COMPAT zephyr_rtcmcp7940



#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <RTCmcp7940.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>
#include <time.h>


LOG_MODULE_REGISTER(RTCMCP7940, CONFIG_CUSTOM_RTCMCP7940_LOG_LEVEL);

/* Alarm channels */
#define ALARM0_ID			0
#define ALARM1_ID			1


/* Size of block when writing whole struct */
#define RTC_TIME_REGISTERS_SIZE		sizeof(struct mcp7940n_time_registers)


/* Largest block size */
#define MAX_WRITE_SIZE                  (RTC_TIME_REGISTERS_SIZE)


/* tm struct uses years since 1900 but unix time uses years since
 * 1970. MCP7940N default year is '1' so the offset is 69
 */
#define UNIX_YEAR_OFFSET		69



/* Macro used to decode BCD to UNIX time to avoid potential copy and paste
 * errors.
 */
#define RTC_BCD_DECODE(reg_prefix) (reg_prefix##_one + reg_prefix##_ten * 10)


/* Convert BCD to binary and vice versa */
#define BCD_TO_BIN(val)       (((val) >> 4) * 10 + ((val) & 0x0F))
#define BIN_TO_BCD(val)       ((((val) / 10) << 4) | ((val) % 10))



struct mcp7940n_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec int_gpios;
};



struct mcp7940n_data {
	const struct device *mcp7940n;
	struct k_sem lock;
	struct mcp7940n_time_registers registers;
	struct gpio_callback int_callback;
	bool int_active_high;
};



/** @brief Reads single register from MCP7940N
 *
 * @param dev the MCP7940N device pointer.
 * @param addr register address.
 * @param val pointer to uint8_t that will contain register value if
 * successful.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction.
 */
static int read_register(const struct device *dev, uint8_t addr, uint8_t *val)
{
	const struct mcp7940n_config *cfg = dev->config;

	int rc = i2c_write_read_dt(&cfg->i2c, &addr, sizeof(addr), val, 1);

	return rc;
}



/** @brief Write a single register to MCP7940N
 *
 * @param dev the MCP7940N device pointer.
 * @param addr register address.
 * @param value Value that will be written to the register.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
static int write_register(const struct device *dev, enum mcp7940n_register addr, uint8_t value)
{
	const struct mcp7940n_config *cfg = dev->config;
	int rc = 0;

	uint8_t time_data[2] = {addr, value};

	rc = i2c_write_dt(&cfg->i2c, time_data, sizeof(time_data));

	return rc;
}


/** @brief Write a full time struct to MCP7940N registers.
 *
 * @param dev the MCP7940N device pointer.
 * @param addr first register address to write to, should be REG_RTC_SEC,
 * REG_ALM0_SEC or REG_ALM0_SEC.
 * @param size size of data struct that will be written.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
static int write_data_block(const struct device *dev, enum mcp7940n_register addr, uint8_t size)
{
	struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	int rc = 0;
	uint8_t time_data[MAX_WRITE_SIZE + 1];
	uint8_t *write_block_start;

	if (size > MAX_WRITE_SIZE) {
		return -EINVAL;
	}

	if (addr >= REG_INVAL) {
		return -EINVAL;
	}

	if (addr == REG_RTC_SEC) {
		write_block_start = (uint8_t *)&data->registers;
	} else {
		return -EINVAL;
	}

	/* Load register address into first byte then fill in data values */
	time_data[0] = addr;
	memcpy(&time_data[1], write_block_start, size);

	rc = i2c_write_dt(&cfg->i2c, time_data, size + 1);

	return rc;
}

static int read_time(const struct device *dev, time_t *unix_time)
{
   
	struct mcp7940n_data *data = dev->data;

	const struct mcp7940n_config *cfg = dev->config;
	uint8_t addr = REG_RTC_SEC;
	int rc = i2c_write_read_dt(&cfg->i2c, &addr, sizeof(addr), &data->registers,
				   RTC_TIME_REGISTERS_SIZE);
	if (rc >= 0) {
		*unix_time = decode_rtc(dev);
	}


	return rc;
}

/** @brief Sets the correct weekday.
 *
 * If the time is never set then the device defaults to 1st January 1970
 * but with the wrong weekday set. This function ensures the weekday is
 * correct in this case.
 *
 * @param dev the MCP7940N device pointer.
 * @param unix_time pointer to unix time that will be used to work out the weekday
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
static int set_day_of_week(const struct device *dev, time_t *unix_time)
{
	struct mcp7940n_data *data = dev->data;
	struct tm time_buffer = { 0 };
	int rc = 0;

	if (gmtime_r(unix_time, &time_buffer) != NULL) {
		data->registers.rtc_weekday.weekday = time_buffer.tm_wday;
		rc = write_register(dev, REG_RTC_WDAY,
			*((uint8_t *)(&data->registers.rtc_weekday)));
	} else {
		rc = -EINVAL;
	}

	return rc;
}


static int mcp7940n_counter_start(const struct device *dev)
{
	struct mcp7940n_data *data = dev->data;
	int rc = 0;

//	k_sem_take(&data->lock, K_FOREVER);

	/* Set start oscillator configuration bit */
	data->registers.rtc_sec.start_osc = 1;
	rc = write_register(dev, REG_RTC_SEC,
		*((uint8_t *)(&data->registers.rtc_sec)));

//	k_sem_give(&data->lock);

	return rc;
}



/** @brief Read registers from device and populate mcp7940n_registers struct
 *
 * @param dev the MCP7940N device pointer.
 * @param unix_time pointer to time_t value that will contain unix time if
 * successful.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction.
 */
int RTCmcp7940_get_time(const struct device *dev, time_t *unix_time)
{
   
	struct mcp7940n_data *data = dev->data;

    k_sem_take(&data->lock, K_FOREVER);
	const struct mcp7940n_config *cfg = dev->config;
	uint8_t addr = REG_RTC_SEC;
	int rc = i2c_write_read_dt(&cfg->i2c, &addr, sizeof(addr), &data->registers,
				   RTC_TIME_REGISTERS_SIZE);
	if (rc >= 0) {
		*unix_time = decode_rtc(dev);
	}

    	k_sem_give(&data->lock);
	return rc;
}



int RTCmcp7940_set_time(const struct device *dev,time_t unix_time)
{
    	struct mcp7940n_data *data = dev->data;
	struct tm time_buffer = { 0 };
	int rc = 0;

	if (unix_time > UINT32_MAX) {
		LOG_ERR("Unix time must be 32-bit");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/* Convert unix_time to civil time */
	gmtime_r(&unix_time, &time_buffer);
	LOG_DBG("Desired time is %d-%d-%d %d:%d:%d\n", (time_buffer.tm_year + 1900),
		(time_buffer.tm_mon + 1), time_buffer.tm_mday, time_buffer.tm_hour,
		time_buffer.tm_min, time_buffer.tm_sec);

	/* Encode time */
	rc = encode_rtc(dev, &time_buffer);
	if (rc < 0) {
		goto out;
	}

	/* Write to device */
	rc = write_data_block(dev, REG_RTC_SEC, RTC_TIME_REGISTERS_SIZE);


out:
	k_sem_give(&data->lock);

	return rc;
}






static int mcp7940n_init(const struct device *dev)
{
    struct mcp7940n_data *data = dev->data;
	const struct mcp7940n_config *cfg = dev->config;
	int rc = 0;
	time_t unix_time = 0;

	/* Initialize and take the lock */
	k_sem_init(&data->lock, 0, 1);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
		rc = -ENODEV;
		goto out;
	}

	rc = read_time(dev, &unix_time);
	if (rc < 0) {
		goto out;
	}

	rc = set_day_of_week(dev, &unix_time);
	if (rc < 0) {
		goto out;
	}

	/* Set 24-hour time */
	data->registers.rtc_hours.twelve_hr = false;
	rc = write_register(dev, REG_RTC_HOUR,
		*((uint8_t *)(&data->registers.rtc_hours)));
	if (rc < 0) {
		goto out;
	}

	mcp7940n_counter_start(dev);

out:
	k_sem_give(&data->lock);

	return rc;
}



#define INST_DT_MCP7904N(index)                                                         \
											\
	static struct mcp7940n_data mcp7940n_data_##index;				\
											\
	static const struct mcp7940n_config mcp7940n_config_##index = {			\
		.i2c = I2C_DT_SPEC_INST_GET(index),					\
		.int_gpios = GPIO_DT_SPEC_INST_GET_OR(index, int_gpios, {0}),		\
	};										\
											\
	DEVICE_DT_INST_DEFINE(index, mcp7940n_init, NULL,				\
		    &mcp7940n_data_##index,						\
		    &mcp7940n_config_##index,						\
		    POST_KERNEL,							\
		    CONFIG_CUSTOM_RTCMCP7940_INIT_PRIORITY,					\
		    NULL);



DT_INST_FOREACH_STATUS_OKAY(INST_DT_MCP7904N);