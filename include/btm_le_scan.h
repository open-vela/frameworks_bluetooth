
#ifndef _SDK_INC_BLUETOOTH_LE_SCAN_H
#define _SDK_INC_BLUETOOTH_LE_SCAN_H

#include <stddef.h>

#include "btm_manager.h"
#include "gatt_common.h"

typedef void (*lescan_result_callback)(void* handle, const scan_result_t* result);
typedef void (*lescan_failed_callback)(void* handle, int error);
typedef void (*lescan_started_callback)(void* handle);
typedef struct {
    lescan_result_callback le_scan_result_cb;
    lescan_failed_callback le_scan_failed_cb;
    lescan_started_callback le_scan_started_cb;
} gatt_scan_callbacks;

typedef struct {
    uint8_t scanner_id;
    gatt_scan_callbacks* cb;
} gatt_scanner_t;

typedef struct {
    size_t size;

    bt_result_code (*start_scan)(gatt_scanner_t** handle, scan_filter_t* filter, scan_settings_t* setttings,
        gatt_scan_callbacks* cb);
    bt_result_code (*stop_scan)(gatt_scanner_t* handle);
} btm_gatt_scan_interface_t;

btm_gatt_scan_interface_t* get_le_scan_interface(void* bt_mgr_interface);

#endif