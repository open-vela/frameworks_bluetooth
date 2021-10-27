
#include "btm_le_scan.h"
#include "btm_manager.h"
#include "bts_lescan.h"
#include "bts_gatt.h"

#include "log.h"

static btm_interface_t* bt_mgr_interface = NULL;

static gatt_scan_interface_t* scanner_interface = NULL;

static void on_le_scan_result(gatt_scanner_t* handle, const scan_result_t* result)
{
    BT_CBACK(handle->cb, le_scan_result_cb, handle, result);
}

static void on_le_scan_failed(gatt_scanner_t* handle, int error)
{
    BT_CBACK(handle->cb, le_scan_failed_cb, handle, error);
}

static void on_le_scan_started(gatt_scanner_t* handle, uint8_t scanner_id)
{
    handle->scanner_id = scanner_id;
    BT_CBACK(handle->cb, le_scan_started_cb, handle);
}

static ble_scanner_callbacks le_scan_callbacks = {
    ._ble_scan_result_cb = on_le_scan_result,
    ._ble_scan_failed_cb = on_le_scan_failed,
    ._ble_scan_started_cb = on_le_scan_started,
};

static bt_result_code start_scan(gatt_scanner_t** handle_ptr, scan_filter_t* filter, scan_settings_t* setttings,
    gatt_scan_callbacks* cb)
{
    if (!scanner_interface) {
        BT_LOGE("fail, scanner_interface nullptr");
        return BT_RESULT_STATE_NOT_ON;
    }

    if (*handle_ptr) {
        BT_LOGW("stop scan, then start scan");
        return BT_RESULT_SUCCESS;
    }

    *handle_ptr = (gatt_scanner_t*)malloc(sizeof(gatt_scanner_t));
    memset(*handle_ptr, 0, sizeof(gatt_scanner_t));
    (*handle_ptr)->cb = cb;

    scan_hdl_t client = {
        .filter = filter,
        .settings = setttings,
        .callbacks = &le_scan_callbacks,
        .mgr_ctx = *handle_ptr,
    };

    bt_result_code ret = scanner_interface->start_scan(client);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,start_scan err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code stop_scan(gatt_scanner_t* handle)
{
    if (!scanner_interface) {
        BT_LOGE("fail, scanner_interface nullptr");
        return BT_RESULT_STATE_NOT_ON;
    }

    if (!handle) {
        BT_LOGE("handle NULL");
        return BT_RESULT_FAILED;
    }

    bt_result_code ret = scanner_interface->stop_scan(handle->scanner_id);
    free(handle);

    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,stop_scan err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static btm_gatt_scan_interface_t le_scan_interface = {
    .size = sizeof(btm_gatt_scan_interface_t),

    .start_scan = start_scan,
    .stop_scan = stop_scan,
};

btm_gatt_scan_interface_t* get_le_scan_interface(
    void* bt_mgr)
{
    if (!bt_mgr) {
        BT_LOGE("fail, bt_mgr NULL");
        return NULL;
    }
    bt_mgr_interface = (btm_interface_t*)(bt_mgr);
    gatt_interface_t* interface = (gatt_interface_t*)bt_mgr_interface->get_profile_interface(BT_PROFILE_GATT);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface gatt");
        return NULL;
    }

    scanner_interface = interface->scanner;
    return &le_scan_interface;
}