#include "app_defines.h"
#include "BLE/BLEapp.h"
#include "crypto/cryptoapp.h"
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include "motorcontrol/mcapp.h"

LOG_MODULE_REGISTER(APPLICATION, LOG_LEVEL_DBG);

uint8_t buffer[50];

GLOBAL struct application_packet app_pack_;

/* Configurations */

LOCAL void button_handler(uint32_t button_state, uint32_t has_changed)
{
	int button_pressed = 0;
	int err;
	if (has_changed & button_state)
	{
		switch (has_changed)
		{
		case DK_BTN1_MSK:
			button_pressed = 1;
			break;
		case DK_BTN2_MSK:
			button_pressed = 2;
			break;
		case DK_BTN3_MSK:
			button_pressed = 3;
			break;
		case DK_BTN4_MSK:
			button_pressed = 4;
			break;
		default:
			break;
		}
		LOG_INF("Button %d pressed.", button_pressed);
		dk_set_led_on(DK_LED1);
		// lockBLE_.set_button_status(button_pressed);
		// err = lockBLE_.send_button_notification(lockBLE_.current_conn, button_pressed, 1);
		// if (err) {
		// 	LOG_ERR("couldn't send notification (err: %d)", err);
		// }

		// err = send_button_notification(button_pressed,1);

		sprintf(buffer, "Hello world: %d", button_pressed);
		err = send_lock_notification_text(buffer, strlen(buffer));
		if (err)
		{
			LOG_ERR("couldn't send notification (err: %d)", err);
		}
	}
}

LOCAL void configure_dk_buttons_leds(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err)
	{
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}
	err = dk_leds_init();
	if (err)
	{
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}
static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
			 h, min, s, ms);
	return buf;
}

static int process_mpu6050(const struct device *dev)
{

	struct sensor_value accel[3];

	int rc = sensor_sample_fetch(dev);

	rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ,
							accel);

	LOG_INF("[%s]:\n"
			"  accel %f %f %f m/s/s\n",
			now_str(),
			sensor_value_to_double(&accel[0]),
			sensor_value_to_double(&accel[1]),
			sensor_value_to_double(&accel[2]));

	return rc;
}

uint8_t msgbuffer[128];
uint8_t encrpt_buffer[128];
uint8_t decrpt_buffer[128];
uint8_t iv_buffer[16] = {
	0xcf, 0x6f, 0x5b, 0x6c, 0x61, 0x3d, 0x9e, 0xac,
	0x46, 0x68, 0x02, 0x83, 0x58, 0x70, 0x6f, 0x8a};
uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28,
				   0xae, 0xd2, 0xa6, 0xab, 0xf7,
				   0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

int main(void)
{
	LOG_INF("SmartLock Started...");
	// configure_dk_buttons_leds();

	BLEapp_init();
	// struct cryptoapp_packet crypto_runtime_packet = {
	// 	.key = key,
	// 	.key_size = sizeof(key),
	// 	.iv_buffer = iv_buffer,
	// 	.iv_size = sizeof(iv_buffer),
	// 	.msgBuf = msgbuffer,
	// 	.msgBuf_size = sizeof(msgbuffer),
	// 	.encryptedMsgBuf = encrpt_buffer,
	// 	.encryptedMsgBuf_size = sizeof(encrpt_buffer),
	// 	.decryptedMsgBuf = decrpt_buffer,
	// 	.decryptedMsgBuf_size = sizeof(decrpt_buffer),
	// };

	// strcpy(msgbuffer, "hello world encrypted now");
	// encrpt_buffer[0] = 0x4C;
	// encrpt_buffer[1] = 0xBC;
	// encrpt_buffer[2] = 0x36;
	// encrpt_buffer[3] = 0x8D;
	// encrpt_buffer[4] = 0x99;
	// encrpt_buffer[5] = 0x1D;
	// encrpt_buffer[6] = 0x46;
	// crypto_runtime_packet.encryptedMsgBuf_size = 7;
	// crypto_runtime_packet.decryptedMsgBuf_size = 7;

	// cryptoapp_run(cryptoapp_impKeyndDecrypt, &crypto_runtime_packet);

	//	const struct device *const adxl345 = DEVICE_DT_GET_ONE(adi_adxl345);

	// if (!device_is_ready(adxl345)) {
	// 	LOG_INF("Device %s is not ready\n", adxl345->name);
	// 	return 0;
	// }

	mcapp_init();

	// mcapp_speedDirection(mcapp_forward,800);
	while (1)
	{
		// int rc = process_mpu6050(adxl345);
		struct BLEapp_event evt = {0};
		BLEapp_event_manager_timed_get(&evt, K_MSEC(10000));

		switch (evt.type)
		{

		case BLE_APP_EVENT_LOCK:
		{
			mcapp_speedDirection(mcapp_forward, 800);
			k_msleep(1000);
			mcapp_speedDirection(mcapp_forward, 0);
			break;
		}

		case BLE_APP_EVENT_UNLOCK:
		{
			mcapp_speedDirection(mcapp_backward, 800);
			k_msleep(1000);
			mcapp_speedDirection(mcapp_backward, 0);
			break;
		}
		}
	}
	return 0;
}
