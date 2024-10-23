#include "app_defines.h"
#include "BLE/BLEapp.h"
#include "crypto/cryptoapp.h"
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include "motorcontrol/mcapp.h"
#include "System/bib.h"

LOG_MODULE_REGISTER(APPLICATION, LOG_LEVEL_DBG);

uint8_t buffer[50];

union byte_2byte {
	uint16_t L;
	uint8_t B[2];
};
union byte_2byte com_conv_;

GLOBAL struct application_packet app_pack_ = {NULL};

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

uint8_t msgbuffer[500];
uint8_t encrpt_buffer[500];
uint8_t decrpt_buffer[500];

uint8_t iv_buffer[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28,
				   0xae, 0xd2, 0xa6, 0xab, 0xf7,
				   0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

int main(void)
{
	LOG_INF("SmartLock Started...");




		const struct device *const adxl345 = DEVICE_DT_GET_ONE(adi_adxl345);

	if (!device_is_ready(adxl345)) {
		LOG_INF("Device %s is not ready\n", adxl345->name);
		//return 0;
	}

	BLEapp_init();
	mcapp_init();

	while (1)
	{
		// int rc = process_mpu6050(adxl345);
		struct BLEapp_event evt = {0};
		BLEapp_event_manager_timed_get(&evt, K_MSEC(10000));

		switch (evt.type)
		{

		case BLE_APP_EVENT_LOCK:
		{
				LOG_INF("Lock system");
			mcapp_speedDirection(mcapp_forward, 800);
			k_msleep(1000);
			mcapp_speedDirection(mcapp_forward, 0);
			break;
		}

		case BLE_APP_EVENT_UNLOCK:
		{
			LOG_INF("unLock system");
			mcapp_speedDirection(mcapp_backward, 800);
			k_msleep(1000);
			mcapp_speedDirection(mcapp_backward, 0);
			break;
		}

		case BLE_APP_EVENT_MSG_RECV:
		{
			LOG_INF("BLE MEssage Received");

			if (!bib_parseMainMsg(app_pack_.BlemsgBuf, app_pack_.BlemsgLength, app_pack_.BLEivBuf, app_pack_.BLEencryptBuf, &app_pack_.BLEencryptBufLen))
			{
				LOG_HEXDUMP_INF(app_pack_.BLEivBuf, 16, "IV counter:");

				LOG_INF("CipherLength: %d", app_pack_.BLEencryptBufLen);
				LOG_HEXDUMP_INF(app_pack_.BLEencryptBuf, app_pack_.BLEencryptBufLen, "Encryptbuf:");

				struct cryptoapp_packet BlemsgCrypto = {
					.key = key,
					.key_size = sizeof(key),
					.iv_buffer = app_pack_.BLEivBuf,
					.iv_size = 16,
					.encryptedMsgBuf = app_pack_.BLEencryptBuf,
					.encryptedMsgBuf_size = app_pack_.BLEencryptBufLen,
					.decryptedMsgBuf = app_pack_.BLEdecryptBuf,
					.decryptedMsgBuf_size = app_pack_.BLEencryptBufLen,
				};

				cryptoapp_run(cryptoapp_impKeyndDecrypt, &BlemsgCrypto);
				
				if(!bib_parseInnerMsg(app_pack_.BLEdecryptBuf,app_pack_.BLEencryptBufLen
				    ,app_pack_.uniqueId,&app_pack_.datatype,&app_pack_.msgid
					,app_pack_.Data,&app_pack_.dataLen))
				{

					LOG_HEXDUMP_INF(app_pack_.uniqueId, 8, "UniqueId:");
					LOG_INF("DataType : %d" , app_pack_.datatype);
					LOG_INF("MessageId: %d", app_pack_.msgid);
					LOG_INF("Data len : %d", app_pack_.dataLen);
					LOG_HEXDUMP_INF(app_pack_.Data, app_pack_.dataLen, "Data:");


					if(app_pack_.msgid == 0x01)
					{
                      APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_LOCK);
					}
					else if(app_pack_.msgid == 0x02)
					{
                      APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_UNLOCK);
					}


                      /*Sending Ack/Nack*/
					  uint16_t len = 0;
					  app_pack_.msgBuf[0] = STX;
					  len++;

					  for(int i = 0; i< 8 ; i++)
					  {
						app_pack_.msgBuf[i+1] = app_pack_.uniqueId[i];
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
					.key = key,
					.key_size = sizeof(key),
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
				for(int i = 0; i < 16 ; i++ )
				{
					app_pack_.TxCpltMsg[i+1] = app_pack_.TxivBuf[i];
					app_pack_.TxCpltMsgLen++;
				}
				app_pack_.TxCpltMsg[app_pack_.TxCpltMsgLen] = EM;
				app_pack_.TxCpltMsgLen++;

				for(int i = 0; i < len ; i++ )
				{
					app_pack_.TxCpltMsg[i+1+17] = app_pack_.TxEnCryptMsg[i];
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


					 

				}

				else
				{
                    /*To Do */

				}

			}
			else
			{

				/*To do*/
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
