
#ifndef _MGR_INC_BLUETOOTH_LE_ADVERTISE_H
#define _MGR_INC_BLUETOOTH_LE_ADVERTISE_H

#include <stddef.h>

#include "btm_manager.h"
#include "gatt_common.h"

typedef void (*leadv_started_callback)(void* handle);
typedef void (*leadv_stopped_callback)(void* handle);
typedef void (*leadv_failed_callback)(void* handle, int error);

typedef struct {
    leadv_started_callback le_advertise_started_cb;
    leadv_stopped_callback le_advertise_stopped_cb;
    leadv_failed_callback le_advertise_failed_cb;
} gatt_advertise_callbacks;

typedef struct {
    uint8_t advertiser_id;
    gatt_advertise_callbacks* cb;
} gatt_advertiser_t;

typedef struct {
    size_t size;

    bt_result_code (*start_advertising)(gatt_advertiser_t** handle, advertise_param_t* param,
        gatt_advertise_callbacks* cb);
    bt_result_code (*stop_advertising)(gatt_advertiser_t* handle);
} btm_gatt_advertise_interface_t;

btm_gatt_advertise_interface_t* get_le_advertise_interface(void* bt_mgr_interface);

#endif