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



enum commands {
    
    NUL = 0x00,
    SOH = 0x01,
    STX = 0x02,
    ETX = 0x03,
    EOT = 0x04,
    EM  = 0x19,

    CMD = 0x01,
    STATUS = 0x02,
    CONFIG = 0x03, 
    ACKNACK = 0x04, 

    LOCK = 0x01,
    UNLOCK = 0x02,

    LOCKSTATUS = 0x01,
    BATTERYLEVEL = 0x02,

    SETTIMEDATE = 0x11,
    SETENCRYPTIONKEY = 0x21,
    GETTIMEDATE = 0x12,
    GETENCRYPTIONKEY = 0x22,

    ACK = 0x06,
    NACK = 0x15,
  

};



struct application_packet {
    uint8_t Key[16];
    uint8_t lockUnlockStatus;
    uint8_t batteryPercentage;


    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t date;
    uint8_t month;
    uint8_t year;
    





    uint8_t BlemsgBuf[255];
    uint16_t BlemsgLength;
    uint8_t BLEcryptoKey[16]; /*Priv Key*/
    uint8_t BLEivBuf[16];     /*Counter buffer used in AES CTR*/
    uint8_t BLEencryptBuf[255];                  /*Encrypted Data Buffer*/
    uint8_t BLEdecryptBuf[255];                  /*Decrypted Data Buffer*/
    uint16_t BLEencryptBufLen;
    uint16_t BLEdecryptBufLen;


     uint8_t uniqueId[8];
     uint8_t datatype;
     uint8_t msgid;
     uint8_t Data[255];
     uint16_t dataLen;

         
    uint8_t msgBuf[255]; /*Main buffer*/
    uint16_t msgLength;
    uint8_t TxivBuf[16];
    uint8_t TxEnCryptMsg[255];
    uint16_t TxEnCryptMsgLen;
    uint8_t TxCpltMsg[255];
    uint16_t TxCpltMsgLen;
    


    /*BLE related*/
    uint8_t mfg_data[20]; /*Manufacturing Data*/




};




extern struct application_packet app_pack_; /*All the stuff related in application will be shared using this*/ 

#endif