#ifndef _APP_DEFINES_H_
#define _APP_DEFINES_H_


#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <stddef.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>




#define GLOBAL
#define LOCAL static

#define RUN_STATUS_LED DK_LED1
#define CONN_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

struct application_packet {
    uint8_t msgBuf[128]; /*Main buffer*/
    

    /*BLE related*/
    uint8_t mfg_data[20]; /*Manufacturing Data*/

    /*Crypto Related*/
    uint8_t cryptoKey[16]; /*Priv Key*/
    uint8_t ivBuf[16];     /*Counter buffer used in AES CTR*/
    uint8_t encryptBuf[128];                  /*Encrypted Data Buffer*/
    uint8_t decryptBuf[128];                  /*Decrypted Data Buffer*/
};

extern struct application_packet app_pack_; /*All the stuff related in application will be shared using this*/ 

#endif