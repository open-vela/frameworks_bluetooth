
#ifndef _SRV_INC_LESCAN_MANAGER_H
#define _SRV_INC_LESCAN_MANAGER_H

#include <nuttx/list.h>
#include <stdbool.h>
#include <stddef.h>

#include "btm_manager.h"
#include "bts_common.h"

typedef void (*bts_le_scan_result_callback)(void* handle, const scan_result_t* scan_result_data);
typedef void (*bts_le_scan_failed_callback)(void* handle, int error);
typedef void (*bts_le_scan_started_callback)(void* handle, uint8_t scanner_id);
typedef void (*bts_le_scan_stopped_callback)(void* handle);

typedef struct {
    bts_le_scan_result_callback bts_le_scan_result_cb;
    bts_le_scan_failed_callback bts_ble_scan_failed_cb;
    bts_le_scan_started_callback bts_ble_scan_started_cb;
    bts_le_scan_stopped_callback bts_ble_scan_stopped_cb;
} bts_ble_scanner_callbacks;

typedef struct {
    struct list_node node;

    uint8_t scanner_id;
    scan_filter_t* filter;
    scan_settings_t* settings;
    bts_ble_scanner_callbacks* callbacks;

    void* btm_handle;
} bts_lescan_hdl_t;

typedef void (*ble_scan_result_callback)(const scan_result_t* result);

typedef struct {
    ble_scan_result_callback ble_scan_result;
} stack_le_scan_callbacks;
typedef struct {
    size_t size;

    stack_le_scan_callbacks* callbacks;
    bt_result_code (*start_scan)(bts_lescan_hdl_t client);
    bt_result_code (*stop_scan)(uint8_t scanner_id);
} gatt_scan_interface_t;

const gatt_scan_interface_t* get_bts_lescan_instance(void);
#endif