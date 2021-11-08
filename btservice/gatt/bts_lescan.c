#include "bts_lescan.h"

#include <errno.h>
#include <string.h>

#include "bts_service.h"
#include "stack_adapter_gatt.h"

#define LOG_TAG "bts_lescan"
#include "log.h"

typedef struct
{
    enum {
        ON_SCAN_RESULT = 0,
        ON_SCAN_STARTED,
        ON_SCAN_STOPPED,
        ON_SCAN_FAILED,
    } event;

    bts_lescan_hdl_t* handle;
    size_t size;
    void* data;
} bts_lescan_msg_t;

static void send_msg(bts_lescan_msg_t* msg);
static void handle_event(void* data, size_t size);

static struct list_node scanner_list = LIST_INITIAL_VALUE(scanner_list);

static uint8_t generate_scanner_id()
{
    uint8_t found = 0;
    bts_lescan_hdl_t* handle;
    for (uint8_t i = 1; i < 256; i++) {
        list_for_every_entry(&scanner_list, handle, bts_lescan_hdl_t, node)
        {
            if (handle->scanner_id == i) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return i;
        }
    }
    BT_LOGE("handle id overflow");
    return 0;
}

static bts_lescan_hdl_t* find_scan_handle(uint8_t scanner_id)
{
    bts_lescan_hdl_t* handle;
    list_for_every_entry(&scanner_list, handle, bts_lescan_hdl_t, node)
    {
        if (handle->scanner_id == scanner_id) {
            return handle;
        }
    }
    return NULL;
}

static uint8_t add_scan_handle(bts_lescan_hdl_t client)
{
    bts_lescan_hdl_t* handle = (bts_lescan_hdl_t*)malloc(sizeof(bts_lescan_hdl_t));
    if (!handle) {
        BT_LOGE("malloc handle fail");
        return 0;
    }

    memcpy(handle, &client, sizeof(bts_lescan_hdl_t));
    handle->scanner_id = generate_scanner_id();
    if (!(handle->scanner_id)) {
        BT_LOGE("fail, generate_scanner_id id:%d", handle->scanner_id);
        return 0;
    }
    list_add_tail(&scanner_list, &handle->node);
    return handle->scanner_id;
}

static bool remove_scan_handle(bts_lescan_hdl_t* handle)
{
    list_delete(&handle->node);
    free(handle);
    return true;
}

static bts_lescan_msg_t* create_adp_msg(uint8_t event, bts_lescan_hdl_t* handle, void* data, size_t size)
{
    bts_lescan_msg_t* msg = (bts_lescan_msg_t*)malloc(sizeof(bts_lescan_msg_t));
    CHECK_PTR_RETURN(msg, NULL);

    if (size < 0) {
        BT_LOGE("fail, invlaid size:%d", size);
        return NULL;
    }
    msg->event = event;
    msg->handle = handle;
    msg->size = size;
    if (size == 0) {
        return msg;
    }

    void* value = malloc(size);
    if (!value) {
        BT_LOGE("fail, malloc data");
        free(msg);
        return NULL;
    }
    memcpy(value, data, size);
    msg->data = value;

    return msg;
}

static bt_result_code start_scan(bts_lescan_hdl_t handle)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_set_ble_scan_filter(&handle.filter);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble scan filter fail, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    ret = service_adapter_gap_set_ble_scan_parameters(&handle.settings);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble scan parameters fail, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    ret = service_adapter_gap_start_ble_scan();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble start scan, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    uint8_t id = add_scan_handle(handle);
    if (!id) {
        BT_LOGE("add_scan_handle err:%d", id);
        return BT_RESULT_FAILED;
    }
    bts_lescan_hdl_t* handle2 = find_scan_handle(id);
    CHECK_PTR_RETURN(handle2, BT_RESULT_FAILED);

    send_msg(create_adp_msg(ON_SCAN_STARTED, handle2, NULL, 0));
    return BT_RESULT_SUCCESS;
}

static bt_result_code stop_scan(uint8_t scanner_id)
{
    bts_lescan_hdl_t* handle = find_scan_handle(scanner_id);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_scan();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        remove_scan_handle(handle);
        BT_LOGE("set ble stop scan, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    send_msg(create_adp_msg(ON_SCAN_STOPPED, handle, NULL, 0));
    return BT_RESULT_SUCCESS;
}

void on_ble_scan_result(const scan_result_t* scan_result_data)
{
    bts_lescan_hdl_t* handle;
    list_for_every_entry(&scanner_list, handle, bts_lescan_hdl_t, node)
    {
        bts_lescan_msg_t* msg = (bts_lescan_msg_t*)malloc(sizeof(bts_lescan_msg_t));
        CHECK_PTR(msg);

        scan_result_t* value = malloc(sizeof(scan_result_t) + scan_result_data->length);
        CHECK_PTR(value);
        memcpy(value->remote_addr, scan_result_data->remote_addr, sizeof(bd_addr_t));
        value->device_type = scan_result_data->device_type;
        value->rssi = scan_result_data->rssi;
        value->addr_type = scan_result_data->addr_type;
        value->evt_type = scan_result_data->evt_type;
        value->length = scan_result_data->length;
        memcpy(value->adv_data, scan_result_data->adv_data, scan_result_data->length);

        msg->event = ON_SCAN_RESULT;
        msg->handle = handle;
        msg->size = sizeof(scan_result_t) + scan_result_data->length;
        msg->data = value;

        send_msg(msg);
    }
}

static const stack_le_scan_callbacks le_scanner_cbs = {
    .ble_scan_result = on_ble_scan_result,
};

static const gatt_scan_interface_t ble_scan_intance = {
    .size = sizeof(gatt_scan_interface_t),

    .callbacks = &le_scanner_cbs,
    .start_scan = start_scan,
    .stop_scan = stop_scan,
};

const gatt_scan_interface_t* get_bts_lescan_instance(void)
{
    return &ble_scan_intance;
}

static void handle_event(void* data, size_t size)
{
    BT_LOGD("%s", __func__);
    bts_lescan_msg_t* msg = (bts_lescan_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }

    bts_lescan_hdl_t* handle = (bts_lescan_hdl_t*)(msg->handle);
    if (!handle) {
        BT_LOGE("%s fail, handle null", __func__);
        return;
    }

    switch (msg->event) {
    case ON_SCAN_RESULT: {
        scan_result_t* scan_result_data = (scan_result_t*)(msg->data);
        BT_CBACK(handle->callbacks, bts_le_scan_result_cb, handle->btm_handle, scan_result_data);
        break;
    }
    case ON_SCAN_STARTED: {
        BT_CBACK(handle->callbacks, bts_ble_scan_started_cb, handle->btm_handle, handle->scanner_id);
        break;
    }
    case ON_SCAN_STOPPED: {
        BT_CBACK(handle->callbacks, bts_ble_scan_stopped_cb, handle->btm_handle);
        remove_scan_handle(handle);
        break;
    }
    case ON_SCAN_FAILED: {
        break;
    }
    default: {
        BT_LOGW("invalid event:%d", msg->event);
        break;
    }
    }
    if (msg->size > 0)
        free(msg->data);
    free(msg);
}

static void send_msg(bts_lescan_msg_t* msg)
{
    excute_service_context_t* context = (excute_service_context_t*)malloc(sizeof(excute_service_context_t));
    context->loop_func = handle_event;
    context->data = (void*)msg;
    context->data_size = sizeof(bts_lescan_msg_t);
    process_in_loop(context);
}