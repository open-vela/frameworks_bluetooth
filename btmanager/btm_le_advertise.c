
#include "btm_le_advertise.h"
#include "btm_manager.h"
#include "bts_leadv.h"
#include "gatt_service.h"

#include "log.h"

static btm_interface_t* bt_mgr_interface = NULL;

static gatt_advertise_interface_t* advertiser_interface = NULL;

static void on_le_advertise_started(gatt_advertiser_t* handle, uint8_t adv_id)
{
    if (!handle) {
        BT_LOGE("handle NULL");
    }
    handle->advertiser_id = adv_id;
    BT_CBACK(handle->cb, le_advertise_started_cb, handle);
}

static void on_le_advertise_stopped(gatt_advertiser_t* handle, uint8_t adv_id)
{
    if (!handle) {
        BT_LOGE("handle NULL");
    }
    BT_CBACK(handle->cb, le_advertise_stopped_cb, handle);
}

static void on_le_advertise_failed(gatt_advertiser_t* handle, int error)
{
    if (!handle) {
        BT_LOGE("handle NULL");
    }
    BT_CBACK(handle->cb, le_advertise_failed_cb, handle, error);
}

static ble_advertiser_callbacks le_advertise_callbacks = {
    ._ble_advertise_started_cb = on_le_advertise_started,
    ._ble_advertise_stopped_callback = on_le_advertise_stopped,
    ._ble_advertise_failed_callback = on_le_advertise_failed,
};

static bt_result_code start_advertising(gatt_advertiser_t** handle_ptr, advertise_param_t* param,
    gatt_advertise_callbacks* cb)
{
    if (!advertiser_interface) {
        BT_LOGE("fail, advertiser_interface nullptr");
        return BT_RESULT_STATE_NOT_ON;
    }

    if (*handle_ptr) {
        BT_LOGW("fail, start_advertising");
        return BT_RESULT_SUCCESS;
    }

    *handle_ptr = (gatt_advertiser_t*)malloc(sizeof(gatt_advertiser_t));
    memset(*handle_ptr, 0, sizeof(gatt_advertiser_t));
    (*handle_ptr)->cb = cb;

    advertise_hdl client = {
        .param = param,
        .callbacks = &le_advertise_callbacks,
        .mgr_ctx = *handle_ptr,
    };
    bt_result_code ret = advertiser_interface->start_adv(client);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, start_adv err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code stop_advertising(gatt_advertiser_t* handle)
{
    if (!advertiser_interface) {
        BT_LOGE("fail, advertiser_interface nullptr");
        return BT_RESULT_STATE_NOT_ON;
    }
    if (!handle) {
        BT_LOGE("handle NULL");
        return BT_RESULT_FAILED;
    }

    bt_result_code ret = advertiser_interface->stop_adv(handle->advertiser_id);
    free(handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,stop_scan err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static btm_gatt_advertise_interface_t le_advertise_interface = {
    .size = sizeof(le_advertise_interface),

    .start_advertising = start_advertising,
    .stop_advertising = stop_advertising,
};

btm_gatt_advertise_interface_t* get_le_advertise_interface(
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

    advertiser_interface = interface->advertiser;
    return &le_advertise_interface;
}
