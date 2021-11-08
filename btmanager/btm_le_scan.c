
#include "btm_le_scan.h"
#include "btm_manager.h"
#include "bts_gatt.h"
#include "bts_lescan.h"

#include "log.h"

#define LOG_TAG "btm_lescan"
typedef struct {
    uint8_t scanner_id;
    gatt_scan_callbacks* cb;
} btm_lescan_hdl_t;

static btm_interface_t* bt_mgr_interface = NULL;
static gatt_scan_interface_t* scanner_interface = NULL;

static void on_le_scan_result(btm_lescan_hdl_t* handle, const scan_result_t* result)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_scan_result_cb, handle, result);
}

static void on_le_scan_failed(btm_lescan_hdl_t* handle, int error)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_scan_failed_cb, handle, error);
}

static void on_le_scan_started(btm_lescan_hdl_t* handle, uint8_t scanner_id)
{
    CHECK_PTR(handle);
    handle->scanner_id = scanner_id;
    BT_CBACK(handle->cb, le_scan_started_cb, handle);
}

static void on_le_scan_stopped(btm_lescan_hdl_t* handle)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_scan_stopped_cb, handle);
    free(handle);
}

static bts_ble_scanner_callbacks le_scan_callbacks = {
    .bts_le_scan_result_cb = on_le_scan_result,
    .bts_ble_scan_failed_cb = on_le_scan_failed,
    .bts_ble_scan_started_cb = on_le_scan_started,
    .bts_ble_scan_stopped_cb = on_le_scan_stopped,
};

static bt_result_code start_scan(btm_lescan_hdl_t** handle_ptr, scan_filter_t* filter, scan_settings_t* setttings,
    gatt_scan_callbacks* cb)
{
    CHECK_PTR_RETURN(scanner_interface, BT_RESULT_STATE_NOT_ON);
    *handle_ptr = (btm_lescan_hdl_t*)malloc(sizeof(btm_lescan_hdl_t));
    memset(*handle_ptr, 0, sizeof(btm_lescan_hdl_t));
    (*handle_ptr)->cb = cb;

    bts_lescan_hdl_t client = {
        .filter = filter,
        .settings = setttings,
        .callbacks = &le_scan_callbacks,
        .btm_handle = *handle_ptr,
    };

    bt_result_code ret = scanner_interface->start_scan(client);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,start_scan err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code stop_scan(btm_lescan_hdl_t* handle)
{
    CHECK_PTR_RETURN(scanner_interface, BT_RESULT_STATE_NOT_ON);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    bt_result_code ret = scanner_interface->stop_scan(handle->scanner_id);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,stop_scan err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static btm_le_scan_interface_t le_scan_interface = {
    .size = sizeof(le_scan_interface),

    .start_scan = start_scan,
    .stop_scan = stop_scan,
};

btm_le_scan_interface_t* get_btm_lescan_interface(void* bt_mgr)
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