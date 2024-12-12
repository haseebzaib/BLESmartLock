#include "BLEapp.h"
#include "crypto/cryptoapp.h"

LOG_MODULE_REGISTER(BLEapp,LOG_LEVEL_DBG);

/* Define message queue */
K_MSGQ_DEFINE(app_event_msq, sizeof(struct BLEapp_event), APP_EVENT_QUEUE_SIZE, 4);






#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME)-1)


LOCAL struct bt_conn * current_conn;

/* Declarations */
LOCAL void on_connected(struct bt_conn *conn, uint8_t err);
LOCAL void on_disconnected(struct bt_conn *conn, uint8_t reason);
//LOCAL void on_notif_changed(enum bt_button_notifications_enabled status);
LOCAL void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
LOCAL ssize_t read_lock_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
LOCAL void lock_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
LOCAL ssize_t on_write(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags);


LOCAL struct bt_lock_service_cb {
    //void (*notif_changed)(enum bt_button_notifications_enabled status);
    void (*data_received)(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
};


static uint8_t mfg_data[10] = { 0xDE, 0xAD, 0xBE, 0xEF,0xAA,0x1A,0x2A,0xDE, 0x01, 0x16 };
LOCAL const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA(BT_DATA_BIG_INFO, mfg_data, 10), //14 is maximum
};

LOCAL const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LOCK_SERV_VAL),
};

LOCAL struct bt_conn_cb bluetooth_callbacks = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};


void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_INF("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}


static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

LOCAL struct bt_lock_service_cb lock_callbacks = {
	//.notif_changed = on_notif_changed,
    .data_received = on_data_received,
};


BT_GATT_SERVICE_DEFINE(lock_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_LOCK_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_LOCK_BUTTON_CHRC,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_lock_characteristic_cb, NULL, NULL),
     BT_GATT_CCC(lock_chrc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_LOCK_MESSAGE_CHRC,
                    BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                    NULL, on_write, NULL),
);


/* Callbacks */

void on_connected(struct bt_conn *conn, uint8_t err)
{
	if(err) {
		LOG_ERR("connection err: %d", err);
		return;
	}
	LOG_INF("Connected.");
	current_conn = bt_conn_ref(conn);
	dk_set_led_on(CONN_STATUS_LED);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason: %d)", reason);
	dk_set_led_off(CONN_STATUS_LED);
	if(current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}


void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{

	memcpy(app_pack_.BlemsgBuf, data, len);
    app_pack_.BlemsgBuf[len] = 0x00;
    app_pack_.BlemsgLength = len;

	LOG_INF("Received data on conn %p. Len: %d", (void *)conn, len);
    LOG_HEXDUMP_INF(app_pack_.BlemsgBuf,len,"Content:");

    
     if(app_pack_.BlemsgBuf[0] == SOH)
     {
           APP_EVENT_MANAGER_PUSH(BLE_APP_EVENT_MSG_RECV);
     }


}


/* File local functions */
LOCAL void bt_ready(int err)
{
    bt_gatt_cb_register(&gatt_callbacks);
   err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Couldn't start advertising (err = %d)", err);
        return ;
    }

}


static ssize_t on_write(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
	LOG_INF("Received data, handle %d, conn %p",
		attr->handle, (void *)conn);

	if (lock_callbacks.data_received) {
		lock_callbacks.data_received(conn, buf, len);
    }
	return len;
}

void on_sent(struct bt_conn *conn, void *user_data)
{
    ARG_UNUSED(user_data);
    LOG_INF("Notification sent on connection %p", (void *)conn);
}

static ssize_t read_lock_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	// return bt_gatt_attr_read(conn, attr, buf, len, offset, &button_value,
	// 			 sizeof(button_value));

       LOG_DBG("Button value is read");
}

void lock_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Notifications %s", notif_enabled? "enabled":"disabled");

}

/*Global functions*/



 GLOBAL int send_lock_notification(uint8_t value, uint16_t length)
 {
 int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &lock_srv.attrs[2];

    params.attr = attr;
    params.data = &value;
    params.len = length;
    params.func = on_sent;

    err = bt_gatt_notify_cb(current_conn, &params);

    return err;
 }

 GLOBAL int send_lock_notification_text(uint8_t *data, uint16_t length)
 {
     int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &lock_srv.attrs[2];

    params.attr = attr;
    params.data = &data[0];
    params.len = length;
    params.func = on_sent;

    err = bt_gatt_notify_cb(current_conn, &params);

    return err;
 }


GLOBAL int BLEapp_event_manager_push(struct BLEapp_event *p_evt)
{
return k_msgq_put(&app_event_msq, p_evt, K_NO_WAIT);
}

GLOBAL int BLEapp_event_manager_get(struct BLEapp_event *p_evt)
{
return k_msgq_get(&app_event_msq, p_evt, K_FOREVER);
}

GLOBAL int BLEapp_event_manager_timed_get(struct BLEapp_event *p_evt, k_timeout_t timeout)
{
    return k_msgq_get(&app_event_msq, p_evt, timeout);
}

GLOBAL void BLEapp_loop(uint8_t lockstatus,uint8_t batteryLevel)
{

     if (!current_conn) { // Update only when not connected

	        mfg_data[8] = lockstatus;
	        mfg_data[9] = batteryLevel;
            bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
        }


}



GLOBAL int BLEapp_init()
{
    //mfg_data[3] = 0x00;
    int err;
   LOG_INF("Initializing bluetooth...");

    strncpy(mfg_data,app_pack_.mfg_data,10);

    bt_conn_cb_register(&bluetooth_callbacks);

   err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("bt_enable returned %d", err);
        return err;
    }
    return err;
}