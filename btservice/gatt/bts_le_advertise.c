/****************************************************************************
 * frameworks/bluetooth/btservice/gatt/bts_le_advertise.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bts_le_advertise.h"

#include <errno.h>
#include <string.h>

#include "bts_service.h"
#include "stack_adapter_gatt.h"

#define LOG_TAG "bts_leadv"
#include "log.h"

typedef struct
{
    enum {
        ON_ADV_STARTED = 0,
        ON_ADV_STOPPED,
        ON_ADV_FAILED,
    } event;

    bts_leadv_hdl_t* handle;
    size_t size;
    void* data;
} gatt_lesadv_msg_t;

static void send_msg(gatt_lesadv_msg_t* msg);
static void handle_msg_received(bt_profile_id id, void* data, size_t size);

static struct list_node advertiser_list = LIST_INITIAL_VALUE(advertiser_list);

static bts_leadv_hdl_t* find_advertise_handle(uint8_t advertiser_id)
{
    bts_leadv_hdl_t* client;
    list_for_every_entry(&advertiser_list, client, bts_leadv_hdl_t, node)
    {
        if (client->advertiser_id == advertiser_id) {
            return client;
        }
    }
    return NULL;
}

static void add_advertise_handle(bts_leadv_hdl_t advertiser)
{
    bts_leadv_hdl_t* client = (bts_leadv_hdl_t*)malloc(sizeof(bts_leadv_hdl_t));
    if (!client) {
        BT_LOGE("malloc client fail");
        return;
    }
    memset(client, 0, sizeof(bts_leadv_hdl_t));

    client->advertiser_id = advertiser.advertiser_id;
    client->param = advertiser.param;
    client->callbacks = advertiser.callbacks;
    client->btm_handle = advertiser.btm_handle;
    list_add_tail(&advertiser_list, &client->node);
}

static bool remove_advertise_handle(bts_leadv_hdl_t* advertiser)
{
    list_delete(&advertiser->node);
    free(advertiser);
    return true;
}

static bt_result_code le_start_adv(bts_leadv_hdl_t client)
{
    bts_register_profile_process(BT_PROFILE_LEADV_ID, &handle_msg_received);
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
    bts_leadv_hdl_t* client = find_advertise_handle(advertiser_id);
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
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
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
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
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

static const bts_le_advertise_interface_t ble_advertise_intance = {
    .size = sizeof(ble_advertise_intance),

    .callbacks = &le_callbacks,
    .start_adv = le_start_adv,
    .stop_adv = le_stop_adv,
};

const bts_le_advertise_interface_t* get_bts_bleadv_instance(void)
{
    return &ble_advertise_intance;
}

static void handle_msg_received(bt_profile_id id, void* data, size_t size)
{
    if (id != BT_PROFILE_LEADV_ID) {
        BT_LOGE("error, invalid priofile id:%d", id);
        return;
    }
    BT_LOGD("%s", __func__);
    gatt_lesadv_msg_t* msg = (gatt_lesadv_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }

    switch (msg->event) {
    case ON_ADV_STARTED: {
        bts_leadv_hdl_t* handle = (bts_leadv_hdl_t*)(msg->handle);
        BT_CBACK(handle->callbacks, bts_le_advertise_started_cb, handle->btm_handle, handle->advertiser_id);
        break;
    }
    case ON_ADV_STOPPED: {
        bts_leadv_hdl_t* handle = (bts_leadv_hdl_t*)(msg->handle);
        BT_CBACK(handle->callbacks, bts_le_advertise_stopped_cb, handle->btm_handle, handle->advertiser_id);
        remove_advertise_handle(handle);
        bts_unregister_profile_process(BT_PROFILE_LEADV_ID);
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

static void send_msg(gatt_lesadv_msg_t* msg)
{
    bts_send_uv_msg(BT_PROFILE_LEADV_ID, msg, sizeof(gatt_lesadv_msg_t));
}