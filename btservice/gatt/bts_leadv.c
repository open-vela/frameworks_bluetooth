
#include "bts_leadv.h"

#include <errno.h>
#include <string.h>

#include "bts_service.h"
#include "stack_adapter_gatt.h"

#define LOG_TAG "bts_leadv"
typedef struct
{
    enum {
        ON_ADV_STARTED = 0,
        ON_ADV_STOPPED,
        ON_ADV_FAILED,
    } event;

    advertise_hdl* handle;
    size_t size;
    void* data;
} gatt_lesadv_msg_t;

static void send_msg(gatt_lesadv_msg_t* msg);
static void handle_event(void* data, size_t size);

static struct list_node advertiser_list = LIST_INITIAL_VALUE(advertiser_list);

static advertise_hdl* find_advertise_handle(uint8_t advertiser_id)
{
    advertise_hdl* client;
    list_for_every_entry(&advertiser_list, client, advertise_hdl, node)
    {
        if (client->advertiser_id == advertiser_id) {
            return client;
        }
    }
    return NULL;
}

static void add_advertise_handle(advertise_hdl advertiser)
{
    advertise_hdl* client = (advertise_hdl*)malloc(sizeof(advertise_hdl));
    if (!client) {
        BT_LOGE("malloc client fail");
        return;
    }

    client->advertiser_id = advertiser.advertiser_id;
    client->param = advertiser.param;
    client->callbacks = advertiser.callbacks;
    client->mgr_ctx = advertiser.mgr_ctx;
    list_add_tail(&advertiser_list, &client->node);
}

static bool remove_advertise_handle(advertise_hdl* advertiser)
{
    list_delete(&advertiser->node);
    free(advertiser);
    return true;
}

static bt_result_code le_start_adv(advertise_hdl client)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_start_ble_adv(client.param);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble start adv fail, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    add_advertise_handle(client);
    return BT_RESULT_SUCCESS;
}

static bt_result_code le_stop_adv(uint8_t advertiser_id)
{
    advertise_hdl* client = find_advertise_handle(advertiser_id);
    if (!client) {
        BT_LOGE("fail, invalid advertiser_id:%d", advertiser_id);
        return BT_RESULT_FAILED;
    }

    SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_adv(advertiser_id);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble start adv fail, err:%d", ret);
         remove_advertise_handle(client);
        return BT_RESULT_FAILED;
    }

    return BT_RESULT_SUCCESS;
}

static void on_ble_advtise_started_cb(uint8_t adv_id)
{
    advertise_hdl* client = find_advertise_handle(adv_id);
    if (!client) {
        BT_LOGE("fail, invalid adv id:%d", adv_id);
        return;
    }

    gatt_lesadv_msg_t* msg = (gatt_lesadv_msg_t*)malloc(sizeof(gatt_lesadv_msg_t));
    memset(msg, 0, sizeof(gatt_lesadv_msg_t));
    msg->event = ON_ADV_STARTED;
    msg->handle = client;
    send_msg(msg);
}

static void on_ble_advtise_stopped_cb(uint8_t adv_id)
{
    advertise_hdl* client = find_advertise_handle(adv_id);
    if (!client) {
        BT_LOGE("fail, invalid adv id:%d", adv_id);
        return;
    }

    gatt_lesadv_msg_t* msg = (gatt_lesadv_msg_t*)malloc(sizeof(gatt_lesadv_msg_t));
    memset(msg, 0, sizeof(gatt_lesadv_msg_t));
    msg->event = ON_ADV_STOPPED;
    msg->handle = client;
    send_msg(msg);
}

static const stack_le_advertise_callbacks le_callbacks = {
    .ble_advtise_started_cb = on_ble_advtise_started_cb,
    .ble_advtise_stopped_cb = on_ble_advtise_stopped_cb,
};

static const gatt_advertise_interface_t ble_advertise_intance = {
    .size = sizeof(ble_advertise_intance),

    .callbacks = &le_callbacks,
    .start_adv = le_start_adv,
    .stop_adv = le_stop_adv,
};

const gatt_advertise_interface_t* get_ble_advertise_instance(void)
{
    return &ble_advertise_intance;
}

static void handle_event(void* data, size_t size)
{
    BT_LOGD("%s", __func__);
    gatt_lesadv_msg_t* msg = (gatt_lesadv_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }

    switch (msg->event) {
    case ON_ADV_STARTED: {
        advertise_hdl* handle = (advertise_hdl*)(msg->handle);
        BT_CBACK(handle->callbacks, _ble_advertise_started_cb, handle->mgr_ctx, handle->advertiser_id);
        free(data);
        break;
    }
    case ON_ADV_STOPPED: {
        advertise_hdl* handle = (advertise_hdl*)(msg->handle);
        BT_CBACK(handle->callbacks, _ble_advertise_stopped_callback, handle->mgr_ctx, handle->advertiser_id);
        remove_advertise_handle(handle);
        free(data);
        break;
    }
    default:
        break;
    }
}

static void send_msg(gatt_lesadv_msg_t* msg)
{
    excute_service_context_t* context = (excute_service_context_t*)malloc(sizeof(excute_service_context_t));
    context->loop_func = handle_event;
    context->data = (void*)msg;
    context->data_size = sizeof(gatt_lesadv_msg_t);
    process_in_loop(context);
}