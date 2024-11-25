#ifndef _BLEAPP_H_
#define _BLEAPP_H_

#include "app_defines.h"





/**
 * @brief Max size of event queue
 *
 */
#define APP_EVENT_QUEUE_SIZE 1

/**
 * @brief Used to interact with different functionality
 * in this application
 */
enum BLEapp_event_type
{

    BLE_APP_EVENT_LOCK = 1,
	BLE_APP_EVENT_UNLOCK,
    BLE_APP_EVENT_LOCK_STATUS,
    BLE_APP_EVENT_BATTERY_LEVEL,
    BLE_APP_EVENT_SETTIMEDATE,
    BLE_APP_EVENT_SETENCRYPTKEY,
    BLE_APP_EVENT_GETTIMEDATE,
    BLE_APP_EVENT_GETENCRYPTKEY,
    BLE_APP_EVENT_ACK,
    BLE_APP_EVENT_NACK,
    BLE_APP_EVENT_MSG_RECV,

};

/**
 * @brief Application event that can be passed back to the main
 * context
 *
 */
struct BLEapp_event
{
    enum BLEapp_event_type type;

};


/**
 * @brief Simplified macro for pushing an app event without data
 *
 */
#define APP_EVENT_MANAGER_PUSH(x)  \
    struct BLEapp_event app_event = { \
        .type = x,                 \
    };                             \
    BLEapp_event_manager_push(&app_event);




/** @brief UUID of the Lock Service. **/
#define BT_UUID_LOCK_SERV_VAL \
	BT_UUID_128_ENCODE(0xe9ea0001, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)

/** @brief UUID of the Button Characteristic. **/
#define BT_UUID_LOCK_BUTTON_CHRC_VAL \
	BT_UUID_128_ENCODE(0xe9ea0002, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)

/** @brief UUID of the Message Characteristic. **/
#define BT_UUID_LOCK_MESSAGE_CHRC_VAL \
	BT_UUID_128_ENCODE(0xe9ea0003, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)


#define BT_UUID_LOCK_SERVICE          BT_UUID_DECLARE_128(BT_UUID_LOCK_SERV_VAL)
#define BT_UUID_LOCK_BUTTON_CHRC 	    BT_UUID_DECLARE_128(BT_UUID_LOCK_BUTTON_CHRC_VAL)
#define BT_UUID_LOCK_MESSAGE_CHRC     BT_UUID_DECLARE_128(BT_UUID_LOCK_MESSAGE_CHRC_VAL)




extern int send_lock_notification(uint8_t value, uint16_t length);
extern int send_lock_notification_text(uint8_t *data, uint16_t length);


extern int BLEapp_event_manager_push(struct BLEapp_event *p_evt);
extern int BLEapp_event_manager_get(struct BLEapp_event *p_evt);
extern int BLEapp_event_manager_timed_get(struct BLEapp_event *p_evt, k_timeout_t timeout);

extern void BLEapp_loop();
extern int BLEapp_init();


#endif