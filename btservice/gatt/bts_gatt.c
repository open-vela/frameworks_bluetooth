
#include <stddef.h>

#include "bts_gatt.h"
#include "log.h"

// static const stack_bt_gatt_interface* get_gatt_instance()
// {
//     stack_bt_gatt_interface* bluetooth_adapter = get_stack_bluetooth_adapter();
//     if (!bluetooth_adapter) {
//         BT_LOGE("fail, get_stack_bluetooth_adapter NULL");
//         return NULL;
//     }

//     stack_bt_gatt_interface* bt_gatt = (stack_bt_gatt_interface*)bluetooth_adapter->get_profile_interface(BT_PROFILE_GATT_ID);
//     if (!bt_gatt) {
//         BT_LOGE("fail, get_profile_interface NULL");
//         return NULL;
//     }
//     return bt_gatt;
// }

// bt_result_code gatt_init(const btgatt_callbacks* callbacks)
// {
//     stack_bt_gatt_interface* gatt = get_gatt_instance();
//     if (!gatt) {
//         BT_LOGE("fail, gatt null");
//         return BT_RESULT_SUCCESS;
//     }

//     gatt->gatt_init();
//     gatt->register_callback(gatt_if.scanner->callbacks, gatt_if.advertiser->callbacks,
//         gatt_if.client->callbacks, gatt_if.server->callbacks);
//     return BT_STATUS_SUCCESS;
// }

// static void gatt_cleanup()
// {
//     stack_bt_gatt_interface* gatt = get_gatt_instance();
//     if (!gatt) {
//         BT_LOGE("fail, gatt null");
//         return BT_RESULT_SUCCESS;
//     }

//     gatt->unregister_callback();
//     gatt->gatt_cleanup();
//     return BT_STATUS_SUCCESS;
// }

// static btgatt_callbacks* gatt_callbacks = NULL;

// static stack_le_scan_callbacks gatt_scan_callback = {
//     .ble_scan_result
// };

// static stack_bt_gatt_callbacks stack_gatt_callbacks = {
//     .gatt_scan_callbacks = &gatt_scan_callback,
//     .gatt_advertise_callbacks = NULL,
//     .client_callbacks = NULL,
//     .server_callbacks = NULL,
// };

static gatt_interface_t gatt_if = {
    .size = sizeof(gatt_if),

    .init = NULL,
    .cleanup = NULL,

    .client = NULL,
    .server = NULL,
    .scanner = NULL,
    .advertiser = NULL,
};

const gatt_interface_t* gatt_get_interface(void)
{
    // gatt_if.client = get_ble_client_instance();
    // gatt_if.server = get_ble_server_instance();
    // gatt_if.scanner = get_ble_scan_instance();
    // gatt_if.advertiser = get_ble_advertise_instance();
    gatt_if.client = NULL;
    gatt_if.server = NULL;
    gatt_if.scanner = get_ble_scan_instance();
    gatt_if.advertiser = get_ble_advertise_instance();
    return &gatt_if;
}