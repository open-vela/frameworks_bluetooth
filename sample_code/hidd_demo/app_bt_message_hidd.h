#ifdef __APP_BT_MESSAGE_CODE__
    APP_BT_HID_DEVICE_MESSAGE_START,
    APP_BT_HID_DEVICE_REGISTER_CALLBACK,
    APP_BT_HID_DEVICE_REGISTER_APP,
    APP_BT_HID_DEVICE_CONNECT,
    APP_BT_HID_DEVICE_MESSAGE_END,
#endif

#ifdef __APP_BT_CALLBACK_CODE__
    APP_BT_HID_DEVICE_CALLBACK_START,
    APP_BT_HID_DEVICE_APP_STATE,
    APP_BT_HID_DEVICE_CONNECTION_STATE,
    APP_BT_HID_DEVICE_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_HIDD_H__
#define _BT_MESSAGE_HIDD_H__

#define BT_NAME_LENGTH 64

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_hid_device.h"

#define MAX_BT_HID_DEVICE_REGISTER_APP_SDP 512

    typedef union {
        struct {
            bt_address_t addr;
        } _bt_hid_device_connect;
    } app_bt_message_hidd_t;

    typedef union {
        struct {
            uint8_t state; /* hid_app_state_t */
        } _app_state;

        struct {
            bt_address_t addr;
            uint8_t le_hid; /* boolean */
            uint8_t state; /* profile_connection_state_t */
        } _connection_state;
    } app_bt_message_hidd_callbacks_t;
#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_HIDD_H__ */