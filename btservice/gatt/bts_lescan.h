
#ifndef _SRV_INC_LESCAN_MANAGER_H
#define _SRV_INC_LESCAN_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#include "btm_manager.h"
#include "gatt_common.h"
#include "list2.h"

typedef void (*le_scan_result_callback)(void* ctx, const scan_result_t* scan_result_data);
typedef void (*le_scan_failed_callback)(int error);
typedef void (*le_scan_started_callback)(void* ctx, uint8_t scanner_id);

typedef struct {
    le_scan_result_callback _ble_scan_result_cb;
    le_scan_failed_callback _ble_scan_failed_cb;
    le_scan_started_callback _ble_scan_started_cb;
} ble_scanner_callbacks;

typedef struct {
    struct list_node node;

    uint8_t scanner_id;
    scan_filter_t* filter;
    scan_settings_t* settings;
    ble_scanner_callbacks* callbacks;

    void* mgr_ctx;
} scan_hdl_t;

typedef void (*ble_scan_result_callback)(const scan_result_t* result);

typedef struct {
    ble_scan_result_callback ble_scan_result;
} stack_le_scan_callbacks;
typedef struct {
    size_t size;

    stack_le_scan_callbacks* callbacks;
    bt_result_code (*start_scan)(scan_hdl_t client);
    bt_result_code (*stop_scan)(uint8_t scanner_id);
} gatt_scan_interface_t;

const gatt_scan_interface_t* get_ble_scan_instance();
#endif