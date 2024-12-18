#include "app_defines.h"
#include "BLE/BLEapp.h"
#include "crypto/cryptoapp.h"
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include "motorcontrol/mcapp.h"
#include "System/bib.h"
#include "System/sys_flash.h"
#include <nrfx.h>

LOG_MODULE_REGISTER(APPLICATION, LOG_LEVEL_DBG);

uint8_t buffer[50];

union byte_2byte
{
	uint16_t L;
	uint8_t B[2];
};
union byte_2byte com_conv_;

GLOBAL struct application_packet app_pack_ = {NULL};

uint8_t key[16] = 
{
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

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

int main(void)
{
	LOG_INF("SmartLock Started...");

	const struct device *const adxl345 = DEVICE_DT_GET_ONE(adi_adxl345);

	if (!device_is_ready(adxl345))
	{
		LOG_INF("Device %s is not ready\n", adxl345->name);
		// return 0;
	}

	 const struct device *const aht20 = DEVICE_DT_GET_ONE(zephyr_aht20);
    if (!device_is_ready(aht20)) {
        LOG_ERR("AHT20 device not ready or not found");
        //return;
    }
	sys_flash_init();


	 sys_flash_erase(Priv_key,16);
	 sys_flash_write(Priv_key, (uint8_t *)key, Priv_Key_Size);

	sys_flash_read(Priv_key, (uint8_t *)app_pack_.Key, Priv_Key_Size);

    if(app_pack_.Key[0] == 0xff && app_pack_.Key[1] == 0xff && app_pack_.Key[2] == 0xff 
	&& app_pack_.Key[3] == 0xff && app_pack_.Key[4] == 0xff && app_pack_.Key[5] == 0xff 
	&& app_pack_.Key[6] == 0xff && app_pack_.Key[7] == 0xff && app_pack_.Key[8] == 0xff 
	&& app_pack_.Key[9] == 0xff && app_pack_.Key[10] == 0xff && app_pack_.Key[11] == 0xff 
	&& app_pack_.Key[12] == 0xff && app_pack_.Key[13] == 0xff && app_pack_.Key[14] == 0xff 
	&& app_pack_.Key[15] == 0xff )
	{
		sys_flash_write(Priv_key, (uint8_t *)key, Priv_Key_Size);
		sys_flash_read(Priv_key, (uint8_t *)app_pack_.Key, Priv_Key_Size);

	}

	sys_flash_read(lock_status_location, &app_pack_.lockUnlockStatus, lock_status_Size);

	if (app_pack_.lockUnlockStatus == 0xff || app_pack_.lockUnlockStatus == 0x00)
	{
		app_pack_.lockUnlockStatus = 0x01;
	}

	app_pack_.batteryPercentage = 90;

	LOG_HEXDUMP_INF(app_pack_.Key, 16, "Flash Key:");

	 uint32_t device_id_low = NRF_FICR->DEVICEID[0];
    uint32_t device_id_high = NRF_FICR->DEVICEID[1];

    // Combine into an 8-byte ID
    memcpy(&app_pack_.uniqueId[0], &device_id_low, 4);  // Copy low 4 bytes
    memcpy(&app_pack_.uniqueId[4], &device_id_high, 4); // Copy high 4 bytes


     LOG_HEXDUMP_INF(app_pack_.uniqueId, 8, "Unique 8-byte ID:");
	 
    app_pack_.mfg_data[0] = app_pack_.uniqueId[0];
	app_pack_.mfg_data[1] = app_pack_.uniqueId[1];
	app_pack_.mfg_data[2] = app_pack_.uniqueId[2];
	app_pack_.mfg_data[3] = app_pack_.uniqueId[3];
	app_pack_.mfg_data[4] = app_pack_.uniqueId[4];
	app_pack_.mfg_data[5] = app_pack_.uniqueId[5];
	app_pack_.mfg_data[6] = app_pack_.uniqueId[6];
	app_pack_.mfg_data[7] = app_pack_.uniqueId[7];
	app_pack_.mfg_data[8] = app_pack_.lockUnlockStatus;
	app_pack_.mfg_data[9] = 99;

	BLEapp_init();
	mcapp_init();

	while (1)
	{

		int rc = process_mpu6050(adxl345);

		 struct sensor_value temp, hum;

		     sensor_sample_fetch(aht20);

            sensor_channel_get(aht20, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(aht20, SENSOR_CHAN_HUMIDITY, &hum);

            LOG_INF("Temperature: %d.%06d C, Humidity: %d.%06d %%", 
                    temp.val1, temp.val2, hum.val1, hum.val2);


		struct BLEapp_event evt = {0};
		BLEapp_event_manager_timed_get(&evt, K_MSEC(1000));

		BLEapp_loop(app_pack_.lockUnlockStatus,11);

		switch (evt.type)
		{

		case BLE_APP_EVENT_LOCK:
		{
			LOG_INF("*******LOCK SYSTEM*******");
			mcapp_speedDirection(mcapp_forward, 800);
			k_msleep(1000);
			mcapp_speedDirection(mcapp_forward, 0);
			app_pack_.lockUnlockStatus = 0x01;
			sys_flash_write(lock_status_location, &app_pack_.lockUnlockStatus, lock_status_Size);
			APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_ACK);
			break;
		}

		case BLE_APP_EVENT_UNLOCK:
		{
			LOG_INF("*******UNLOCK SYSTEM*******");
			mcapp_speedDirection(mcapp_backward, 800);
			k_msleep(1000);
			mcapp_speedDirection(mcapp_backward, 0);
			app_pack_.lockUnlockStatus = 0x02;
			sys_flash_write(lock_status_location, &app_pack_.lockUnlockStatus, lock_status_Size);
			APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_ACK);
			break;
		}

		case BLE_APP_EVENT_LOCK_STATUS:
		{
			LOG_INF("*******LOCK STATUS*******");
			uint16_t len = 0;
			app_pack_.msgBuf[0] = STX;
			len++;

			for (int i = 0; i < 8; i++)
			{
				app_pack_.msgBuf[i + 1] = app_pack_.uniqueId[i];
				len++;
			}

			app_pack_.msgBuf[9] = EM;
			len++;
			app_pack_.msgBuf[10] = STATUS;
			len++;
			app_pack_.msgBuf[11] = EM;
			len++;
			app_pack_.msgBuf[12] = LOCKSTATUS;
			len++;
			app_pack_.msgBuf[13] = EM;
			len++;
			app_pack_.msgBuf[14] = app_pack_.lockUnlockStatus;
			len++;
			app_pack_.msgBuf[15] = EM;
			len++;
			app_pack_.msgBuf[16] = 0x00;
			len++;
			app_pack_.msgBuf[17] = 0x01;
			len++;
			app_pack_.msgBuf[18] = ETX;
			len++;
			app_pack_.msgBuf[19] = EOT;
			len++;

			struct cryptoapp_packet TxmsgCrypto = {
				.key = app_pack_.Key,
				.key_size = sizeof(app_pack_.Key),
				.iv_buffer = app_pack_.TxivBuf,
				.iv_size = 16,
				.msgBuf = app_pack_.msgBuf,
				.msgBuf_size = len,
				.encryptedMsgBuf = app_pack_.TxEnCryptMsg,
				.encryptedMsgBuf_size = len,
			};

			cryptoapp_run(cryptoapp_impKeyndEncrypt, &TxmsgCrypto);

			app_pack_.TxCpltMsgLen = 0;
			app_pack_.TxCpltMsg[0] = SOH;
			app_pack_.TxCpltMsgLen++;
			for (int i = 0; i < 16; i++)
			{
				app_pack_.TxCpltMsg[i + 1] = app_pack_.TxivBuf[i];
				app_pack_.TxCpltMsgLen++;
			}
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			for (int i = 0; i < len; i++)
			{
				app_pack_.TxCpltMsg[i + 1 + 17] = app_pack_.TxEnCryptMsg[i];
				app_pack_.TxCpltMsgLen++;
			}

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			com_conv_.L = len;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[1];
			app_pack_.TxCpltMsgLen++;
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[0];
			app_pack_.TxCpltMsgLen++;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EOT;
			app_pack_.TxCpltMsgLen++;

			LOG_HEXDUMP_INF(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen, "Tx Data content:");

			send_lock_notification_text(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen);

			break;
		}

		case BLE_APP_EVENT_BATTERY_LEVEL:
		{
			LOG_INF("*******BATTERY LEVEL*******");

			uint16_t len = 0;
			app_pack_.msgBuf[0] = STX;
			len++;

			for (int i = 0; i < 8; i++)
			{
				app_pack_.msgBuf[i + 1] = app_pack_.uniqueId[i];
				len++;
			}

			app_pack_.msgBuf[9] = EM;
			len++;
			app_pack_.msgBuf[10] = STATUS;
			len++;
			app_pack_.msgBuf[11] = EM;
			len++;
			app_pack_.msgBuf[12] = BATTERYLEVEL;
			len++;
			app_pack_.msgBuf[13] = EM;
			len++;
			app_pack_.msgBuf[14] = app_pack_.batteryPercentage;
			len++;
			app_pack_.msgBuf[15] = EM;
			len++;
			app_pack_.msgBuf[16] = 0x00;
			len++;
			app_pack_.msgBuf[17] = 0x01;
			len++;
			app_pack_.msgBuf[18] = ETX;
			len++;
			app_pack_.msgBuf[19] = EOT;
			len++;

			struct cryptoapp_packet TxmsgCrypto = {
				.key = app_pack_.Key,
				.key_size = sizeof(app_pack_.Key),
				.iv_buffer = app_pack_.TxivBuf,
				.iv_size = 16,
				.msgBuf = app_pack_.msgBuf,
				.msgBuf_size = len,
				.encryptedMsgBuf = app_pack_.TxEnCryptMsg,
				.encryptedMsgBuf_size = len,
			};

			cryptoapp_run(cryptoapp_impKeyndEncrypt, &TxmsgCrypto);

			app_pack_.TxCpltMsgLen = 0;
			app_pack_.TxCpltMsg[0] = SOH;
			app_pack_.TxCpltMsgLen++;
			for (int i = 0; i < 16; i++)
			{
				app_pack_.TxCpltMsg[i + 1] = app_pack_.TxivBuf[i];
				app_pack_.TxCpltMsgLen++;
			}
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			for (int i = 0; i < len; i++)
			{
				app_pack_.TxCpltMsg[i + 1 + 17] = app_pack_.TxEnCryptMsg[i];
				app_pack_.TxCpltMsgLen++;
			}

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			com_conv_.L = len;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[1];
			app_pack_.TxCpltMsgLen++;
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[0];
			app_pack_.TxCpltMsgLen++;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EOT;
			app_pack_.TxCpltMsgLen++;

			LOG_HEXDUMP_INF(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen, "Tx Data content:");

			send_lock_notification_text(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen);

			break;
		}

		case BLE_APP_EVENT_SETTIMEDATE:
		{
			LOG_INF("*******SET TIME DATE*******");

			if (bib_parseTimeDateAndValidate(app_pack_.Data, &app_pack_.hour, &app_pack_.min, &app_pack_.sec, &app_pack_.date, &app_pack_.month, &app_pack_.year) == 0)
			{
				APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_ACK);
			}
			else
			{
				APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
			}

			break;
		}

		case BLE_APP_EVENT_SETENCRYPTKEY:
		{
			LOG_INF("*******SET ENCRYPTION KEY*******");

			cryptoapp_init();
			if (cryptoapp_importKey(app_pack_.Data, app_pack_.dataLen) == cryptoapp_ok)
			{
				APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_ACK);
				sys_flash_write(Priv_key, (uint8_t *)app_pack_.Data, Priv_Key_Size);

				sys_flash_read(Priv_key, (uint8_t *)app_pack_.Key, Priv_Key_Size);
			}
			else
			{
				APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
			}
			cryptoapp_finish();
			break;
		}

		case BLE_APP_EVENT_GETTIMEDATE:
		{
			LOG_INF("*******GET TIME DATE*******");

			uint16_t len = 0;

			char timebuf[50];

			sprintf(timebuf,"%02d:%02d:%02d|%02d/%02d/%02d",app_pack_.hour,app_pack_.min,app_pack_.sec
			                                               ,app_pack_.date,app_pack_.month,app_pack_.year);
			app_pack_.msgBuf[0] = STX;
			len++;

			for (int i = 0; i < 8; i++)
			{
				app_pack_.msgBuf[i + 1] = app_pack_.uniqueId[i];
				len++;
			}

			app_pack_.msgBuf[9] = EM;
			len++;
			app_pack_.msgBuf[10] = CONFIG;
			len++;
			app_pack_.msgBuf[11] = EM;
			len++;
			app_pack_.msgBuf[12] = GETTIMEDATE;
			len++;
			app_pack_.msgBuf[13] = EM;
			len++;
	         for (int i = 14; i < 31; i++)
			{
				app_pack_.msgBuf[i] = timebuf[i - 14];
				len++;
			}
			app_pack_.msgBuf[31] = EM;
			len++;
			app_pack_.msgBuf[32] = 0x00;
			len++;
			app_pack_.msgBuf[33] = 0x11;
			len++;
			app_pack_.msgBuf[34] = ETX;
			len++;
			app_pack_.msgBuf[35] = EOT;
			len++;

			struct cryptoapp_packet TxmsgCrypto = {
				.key = app_pack_.Key,
				.key_size = sizeof(app_pack_.Key),
				.iv_buffer = app_pack_.TxivBuf,
				.iv_size = 16,
				.msgBuf = app_pack_.msgBuf,
				.msgBuf_size = len,
				.encryptedMsgBuf = app_pack_.TxEnCryptMsg,
				.encryptedMsgBuf_size = len,
			};

			cryptoapp_run(cryptoapp_impKeyndEncrypt, &TxmsgCrypto);

			app_pack_.TxCpltMsgLen = 0;
			app_pack_.TxCpltMsg[0] = SOH;
			app_pack_.TxCpltMsgLen++;
			for (int i = 0; i < 16; i++)
			{
				app_pack_.TxCpltMsg[i + 1] = app_pack_.TxivBuf[i];
				app_pack_.TxCpltMsgLen++;
			}
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			for (int i = 0; i < len; i++)
			{
				app_pack_.TxCpltMsg[i + 1 + 17] = app_pack_.TxEnCryptMsg[i];
				app_pack_.TxCpltMsgLen++;
			}

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			com_conv_.L = len;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[1];
			app_pack_.TxCpltMsgLen++;
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[0];
			app_pack_.TxCpltMsgLen++;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EOT;
			app_pack_.TxCpltMsgLen++;

			LOG_HEXDUMP_INF(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen, "Tx Data content:");

			send_lock_notification_text(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen);

			break;
		}

		case BLE_APP_EVENT_GETENCRYPTKEY:
		{
			LOG_INF("*******GET ENCRYPTION KEY*******");

			uint16_t len = 0;
			app_pack_.msgBuf[0] = STX;
			len++;

			for (int i = 0; i < 8; i++)
			{
				app_pack_.msgBuf[i + 1] = app_pack_.uniqueId[i];
				len++;
			}

			app_pack_.msgBuf[9] = EM;
			len++;
			app_pack_.msgBuf[10] = CONFIG;
			len++;
			app_pack_.msgBuf[11] = EM;
			len++;
			app_pack_.msgBuf[12] = GETENCRYPTIONKEY;
			len++;
			app_pack_.msgBuf[13] = EM;
			len++;
	         for (int i = 14; i < 30; i++)
			{
				app_pack_.msgBuf[i] = app_pack_.Key[i - 14];
				len++;
			}
			app_pack_.msgBuf[30] = EM;
			len++;
			app_pack_.msgBuf[31] = 0x00;
			len++;
			app_pack_.msgBuf[32] = 0x11;
			len++;
			app_pack_.msgBuf[33] = ETX;
			len++;
			app_pack_.msgBuf[34] = EOT;
			len++;

			struct cryptoapp_packet TxmsgCrypto = {
				.key = app_pack_.Key,
				.key_size = sizeof(app_pack_.Key),
				.iv_buffer = app_pack_.TxivBuf,
				.iv_size = 16,
				.msgBuf = app_pack_.msgBuf,
				.msgBuf_size = len,
				.encryptedMsgBuf = app_pack_.TxEnCryptMsg,
				.encryptedMsgBuf_size = len,
			};

			cryptoapp_run(cryptoapp_impKeyndEncrypt, &TxmsgCrypto);

			app_pack_.TxCpltMsgLen = 0;
			app_pack_.TxCpltMsg[0] = SOH;
			app_pack_.TxCpltMsgLen++;
			for (int i = 0; i < 16; i++)
			{
				app_pack_.TxCpltMsg[i + 1] = app_pack_.TxivBuf[i];
				app_pack_.TxCpltMsgLen++;
			}
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			for (int i = 0; i < len; i++)
			{
				app_pack_.TxCpltMsg[i + 1 + 17] = app_pack_.TxEnCryptMsg[i];
				app_pack_.TxCpltMsgLen++;
			}

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			com_conv_.L = len;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[1];
			app_pack_.TxCpltMsgLen++;
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[0];
			app_pack_.TxCpltMsgLen++;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EOT;
			app_pack_.TxCpltMsgLen++;

			LOG_HEXDUMP_INF(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen, "Tx Data content:");

			send_lock_notification_text(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen);

			break;
		}

		case BLE_APP_EVENT_ACK:
		{
			LOG_INF("*******GENERATE ACK*******");
			/*Sending Ack*/
			uint16_t len = 0;
			app_pack_.msgBuf[0] = STX;
			len++;

			for (int i = 0; i < 8; i++)
			{
				app_pack_.msgBuf[i + 1] = app_pack_.uniqueId[i];
				len++;
			}

			app_pack_.msgBuf[9] = EM;
			len++;
			app_pack_.msgBuf[10] = ACKNACK;
			len++;
			app_pack_.msgBuf[11] = EM;
			len++;
			app_pack_.msgBuf[12] = ACK;
			len++;
			app_pack_.msgBuf[13] = EM;
			len++;
			app_pack_.msgBuf[14] = NUL;
			len++;
			app_pack_.msgBuf[15] = EM;
			len++;
			app_pack_.msgBuf[16] = 0x00;
			len++;
			app_pack_.msgBuf[17] = 0x00;
			len++;
			app_pack_.msgBuf[18] = ETX;
			len++;
			app_pack_.msgBuf[19] = EOT;
			len++;

			struct cryptoapp_packet TxmsgCrypto = {
				.key = app_pack_.Key,
				.key_size = sizeof(app_pack_.Key),
				.iv_buffer = app_pack_.TxivBuf,
				.iv_size = 16,
				.msgBuf = app_pack_.msgBuf,
				.msgBuf_size = len,
				.encryptedMsgBuf = app_pack_.TxEnCryptMsg,
				.encryptedMsgBuf_size = len,
			};

			cryptoapp_run(cryptoapp_impKeyndEncrypt, &TxmsgCrypto);

			app_pack_.TxCpltMsgLen = 0;
			app_pack_.TxCpltMsg[0] = SOH;
			app_pack_.TxCpltMsgLen++;
			for (int i = 0; i < 16; i++)
			{
				app_pack_.TxCpltMsg[i + 1] = app_pack_.TxivBuf[i];
				app_pack_.TxCpltMsgLen++;
			}
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			for (int i = 0; i < len; i++)
			{
				app_pack_.TxCpltMsg[i + 1 + 17] = app_pack_.TxEnCryptMsg[i];
				app_pack_.TxCpltMsgLen++;
			}

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			com_conv_.L = len;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[1];
			app_pack_.TxCpltMsgLen++;
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[0];
			app_pack_.TxCpltMsgLen++;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EOT;
			app_pack_.TxCpltMsgLen++;

			LOG_HEXDUMP_INF(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen, "Tx Data content:");

			send_lock_notification_text(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen);
			break;
		}

		case BLE_APP_EVENT_NACK:
		{
			LOG_INF("*******GENERATE NACK*******");
			/*Sending NAck*/
			uint16_t len = 0;
			app_pack_.msgBuf[0] = STX;
			len++;

			for (int i = 0; i < 8; i++)
			{
				app_pack_.msgBuf[i + 1] = app_pack_.uniqueId[i];
				len++;
			}

			app_pack_.msgBuf[9] = EM;
			len++;
			app_pack_.msgBuf[10] = ACKNACK;
			len++;
			app_pack_.msgBuf[11] = EM;
			len++;
			app_pack_.msgBuf[12] = NACK;
			len++;
			app_pack_.msgBuf[13] = EM;
			len++;
			app_pack_.msgBuf[14] = NUL;
			len++;
			app_pack_.msgBuf[15] = EM;
			len++;
			app_pack_.msgBuf[16] = 0x00;
			len++;
			app_pack_.msgBuf[17] = 0x00;
			len++;
			app_pack_.msgBuf[18] = ETX;
			len++;
			app_pack_.msgBuf[19] = EOT;
			len++;

			struct cryptoapp_packet TxmsgCrypto = {
				.key = app_pack_.Key,
				.key_size = sizeof(app_pack_.Key),
				.iv_buffer = app_pack_.TxivBuf,
				.iv_size = 16,
				.msgBuf = app_pack_.msgBuf,
				.msgBuf_size = len,
				.encryptedMsgBuf = app_pack_.TxEnCryptMsg,
				.encryptedMsgBuf_size = len,
			};

			cryptoapp_run(cryptoapp_impKeyndEncrypt, &TxmsgCrypto);

			app_pack_.TxCpltMsgLen = 0;
			app_pack_.TxCpltMsg[0] = SOH;
			app_pack_.TxCpltMsgLen++;
			for (int i = 0; i < 16; i++)
			{
				app_pack_.TxCpltMsg[i + 1] = app_pack_.TxivBuf[i];
				app_pack_.TxCpltMsgLen++;
			}
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			for (int i = 0; i < len; i++)
			{
				app_pack_.TxCpltMsg[i + 1 + 17] = app_pack_.TxEnCryptMsg[i];
				app_pack_.TxCpltMsgLen++;
			}

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
			app_pack_.TxCpltMsgLen++;

			com_conv_.L = len;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[1];
			app_pack_.TxCpltMsgLen++;
			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = com_conv_.B[0];
			app_pack_.TxCpltMsgLen++;

			app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EOT;
			app_pack_.TxCpltMsgLen++;

			LOG_HEXDUMP_INF(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen, "Tx Data content:");

			send_lock_notification_text(app_pack_.TxCpltMsg, app_pack_.TxCpltMsgLen);
			break;
		}

		case BLE_APP_EVENT_MSG_RECV:
		{
			LOG_INF("BLE MEssage Received");

			if (!bib_parseMainMsg(app_pack_.BlemsgBuf, app_pack_.BlemsgLength, app_pack_.BLEivBuf, app_pack_.BLEencryptBuf, &app_pack_.BLEencryptBufLen))
			{

              if(memcmp(app_pack_.BLEivBuf,app_pack_.BLEPrevivBuf,16) != 0)
				{
					memcpy(app_pack_.BLEPrevivBuf,app_pack_.BLEivBuf,16);
				LOG_HEXDUMP_INF(app_pack_.BLEivBuf, 16, "IV counter:");
				
				LOG_INF("CipherLength: %d", app_pack_.BLEencryptBufLen);
				LOG_HEXDUMP_INF(app_pack_.BLEencryptBuf, app_pack_.BLEencryptBufLen, "Encryptbuf:");

				struct cryptoapp_packet BlemsgCrypto = {
					.key = app_pack_.Key,
					.key_size = sizeof(app_pack_.Key),
					.iv_buffer = app_pack_.BLEivBuf,
					.iv_size = 16,
					.encryptedMsgBuf = app_pack_.BLEencryptBuf,
					.encryptedMsgBuf_size = app_pack_.BLEencryptBufLen,
					.decryptedMsgBuf = app_pack_.BLEdecryptBuf,
					.decryptedMsgBuf_size = app_pack_.BLEencryptBufLen,
				};

				cryptoapp_run(cryptoapp_impKeyndDecrypt, &BlemsgCrypto);

				if (!bib_parseInnerMsg(app_pack_.BLEdecryptBuf, app_pack_.BLEencryptBufLen, app_pack_.BLEuniqueId, &app_pack_.datatype, &app_pack_.msgid, app_pack_.Data, &app_pack_.dataLen))
				{

					LOG_HEXDUMP_INF(app_pack_.BLEuniqueId, 8, "UniqueId Recved:");
					if(strcmp(app_pack_.BLEuniqueId, app_pack_.uniqueId) == 0 )
					{
					LOG_INF("DataType : %d", app_pack_.datatype);
					LOG_INF("MessageId: %d", app_pack_.msgid);
					LOG_INF("Data len : %d", app_pack_.dataLen);
					LOG_HEXDUMP_INF(app_pack_.Data, app_pack_.dataLen, "Data:");

					switch (app_pack_.datatype)
					{
					case CMD:
					{
						switch (app_pack_.msgid)
						{
						case LOCK:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_LOCK);
							break;
						}

						case UNLOCK:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_UNLOCK);
							break;
						}
						default:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
							break;
						}
						}

						break;
					}

					case STATUS:
					{
						switch (app_pack_.msgid)
						{
						case LOCKSTATUS:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_LOCK_STATUS);
							break;
						}

						case BATTERYLEVEL:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_BATTERY_LEVEL);
							break;
						}

						default:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
							break;
						}
						}

						break;
					}

					case CONFIG:
					{
						switch (app_pack_.msgid)
						{
						case SETTIMEDATE:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_SETTIMEDATE);
							break;
						}

						case SETENCRYPTIONKEY:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_SETENCRYPTKEY);
							break;
						}

						case GETTIMEDATE:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_GETTIMEDATE);
							break;
						}

						case GETENCRYPTIONKEY:
						{

							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_GETENCRYPTKEY);
							break;
						}
						default:
						{
							APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
							break;
						}
						}

						break;
					}

					default:
					{
						APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
						break;
					}
					}
					}
					else
					{
	                      APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
					}
				
				
				}
				else
				{
					APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_NACK);
				}
			
				}
				else
				{
                     
                    LOG_ERR("*****INTRUDER*****");
				}
			
			}
			else
			{

				/*To do*/

				 LOG_ERR("Wrong Encryption, Therefore Discarding it");
			}
			break;
		}

		

		default:
		{

			break;
		}
		}
	}
	return 0;
}
