/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#define LOG_TAG "pbap_pce"

#include <stdint.h>
#include <stdlib.h>

#include <nuttx/list.h>

#include "bt_addr.h"
#include "bt_device.h"
#include "callbacks_list.h"
#include "pbap_pce_blacklist.h"
#include "pbap_pce_service.h"
#include "pbap_pce_state_machine.h"
#include "pce_parser.h"
#include "sal_pbap_pce_interface.h"
#include "service_loop.h"
#include "service_manager.h"

#include "utils/log.h"

#define PCE_MAX_CONNECTIONS (1)

#define PCE_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, pbap_pce_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    bt_list_t* devices;
    bool enable;
    callbacks_list_t* callbacks;
} pce_global_t;

typedef struct {
    bt_address_t addr;
    pce_state_machine_t* pce_sm;
} pce_device_t;

static pce_global_t g_pce = { 0 };

static void pce_service_event_process(void* data);

bt_status_t do_in_pbap_pce_service(bt_address_t* addr, pbap_pce_event_t event, void* ext_data)
{
    pbap_pce_msg_t* pce_msg;

    pce_msg = pbap_pce_msg_new(event, addr, ext_data);

    if (!pce_msg) {
        BT_LOGE("%s: Create pce msg failed", __func__);
        free(ext_data);
        return BT_STATUS_NOMEM;
    }

    do_in_service_loop(pce_service_event_process, pce_msg);

    return BT_STATUS_SUCCESS;
}

static pce_device_t* pce_device_new(bt_address_t* addr)
{
    pce_device_t* device;
    pce_state_machine_t* pce_sm;

    pce_sm = pce_state_machine_new((void*)&g_pce, addr);
    if (!pce_sm) {
        BT_LOGE("%s: Create state machine failed", __func__);
        return NULL;
    }

    device = malloc(sizeof(pce_device_t));
    if (!device) {
        pce_state_machine_destory(pce_sm);
        BT_LOGE("%s: malloc failed", __func__);
        return NULL;
    }

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->pce_sm = pce_sm;

    return device;
}

static void pce_device_destory(void* data)
{
    pce_device_t* device = (pce_device_t*)data;
    pce_state_machine_destory(device->pce_sm);
    free(device);
}

static bool compare_addr(void* data, void* addr)
{
    pce_device_t* device = (pce_device_t*)data;

    return memcmp(&device->addr, addr, sizeof(bt_address_t)) == 0;
}

static pce_state_machine_t* get_state_machine(bt_address_t* addr)
{
    pce_device_t* device;

    if (!g_pce.enable)
        return NULL;

    device = bt_list_find(g_pce.devices, compare_addr, addr);
    if (device)
        return device->pce_sm;

    device = pce_device_new(addr);
    if (!device) {
        BT_LOGE("%s: New device alloc failed", __func__);
        return NULL;
    }

    bt_list_add_tail(g_pce.devices, device);

    return device->pce_sm;
}

static void* pce_register_callbacks(void* remote, const pbap_pce_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_pce.callbacks, remote, (void*)callbacks);
}

static bool pce_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_pce.callbacks, remote, cookie);
}

static bt_status_t pce_connect(bt_address_t* addr)
{
    return do_in_pbap_pce_service(addr, PCE_CONNECT_REQ, NULL);
}

static bt_status_t pce_disconnect(bt_address_t* addr)
{
    return do_in_pbap_pce_service(addr, PCE_DISCONNECT_REQ, NULL);
}

static bt_status_t pce_get_contact(bt_address_t* addr, bt_pce_get_contact_req_type_t type, void* req_data)
{
    pce_get_contact_req_t* req;

    req = create_query_contact_req(type, req_data);
    if (!req) {
        BT_LOGE("%s: make query contact failed", __func__);
        return BT_STATUS_NOMEM;
    }

    return do_in_pbap_pce_service(addr, PCE_CONNECT_REQ, req);
}

static bt_status_t pce_get_contact_by_name(bt_address_t* addr, char* name)
{
    return pce_get_contact(addr, PCE_GET_CONTACT_BY_NAME, name);
}

static bt_status_t pce_get_contact_by_number(bt_address_t* addr, char* number)
{
    return pce_get_contact(addr, PCE_GET_CONTACT_BY_NUMBER, number);
}

static bt_status_t pce_add_to_blacklist(bt_address_t* set_addr)
{
    if (set_addr == NULL)
        return BT_STATUS_PARM_INVALID;

    return pbap_pce_add_to_blacklist(set_addr);
}

static bt_status_t pce_remove_from_blacklist(bt_address_t* remove_addr)
{
    if (remove_addr == NULL)
        return BT_STATUS_PARM_INVALID;

    return pbap_pce_remove_from_blacklist(remove_addr);
}

static bool pce_is_in_blacklist(bt_address_t* query_addr)
{
    if (query_addr == NULL)
        return BT_STATUS_PARM_INVALID;

    return pbap_pce_is_in_blacklist(query_addr);
}

static const pbap_pce_interface_t pceInterface = {
    .size = sizeof(pceInterface),
    .register_callbacks = pce_register_callbacks,
    .unregister_callbacks = pce_unregister_callbacks,
    .connect = pce_connect,
    .disconnect = pce_disconnect,
    .get_contact_by_number = pce_get_contact_by_number,
    .get_contact_by_name = pce_get_contact_by_name,
    .add_to_blacklist = pce_add_to_blacklist,
    .remove_from_blacklist = pce_remove_from_blacklist,
    .is_in_blacklist = pce_is_in_blacklist,
};

void notify_pce_connection_state_changed(bt_address_t* addr, profile_connection_state_t state)
{
    BT_LOGD("addr: %s: state: %d", bt_addr_str(addr), state);
    PCE_CALLBACK_FOREACH(g_pce.callbacks, connection_state_cb, addr, state);
}

void notify_get_contact_end(bt_address_t* addr, pce_get_contact_end_evt_t* evt)
{
    BT_LOGD("addr: %s: status: %d", bt_addr_str(addr), evt->status);
    PCE_CALLBACK_FOREACH(g_pce.callbacks, get_contact_end_cb, evt->status, evt->type, evt->req_data, evt->contact);
}

static void pce_service_event_process(void* data)
{
    pbap_pce_msg_t* msg = data;
    pce_state_machine_t* pce_sm;

    pce_sm = get_state_machine(&msg->data.addr);

    if (pce_sm == NULL) {
        BT_LOGE("%s: get pce_sm fail.", __func__);
        pbap_pce_msg_destroy(data);
        return;
    }

    pce_state_machine_handle_event(pce_sm, msg);
    pbap_pce_msg_destroy(data);
    return;
}

void pce_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state)
{
    pbap_pce_event_t event;

    switch (state) {
    case PROFILE_STATE_CONNECTED:
        event = PCE_CONNECTED_EVT;
        break;
    case PROFILE_STATE_DISCONNECTED:
        event = PCE_DISCONNECTED_EVT;
        break;
    default:
        return;
    }

    do_in_pbap_pce_service(addr, event, NULL);
}

void pce_on_dir_changed(bt_address_t* addr, uint16_t status)
{
    BT_LOGD("dir changed, %s, addr: %s, status: %d", __func__, bt_addr_str(addr), status);
}

void pce_on_vcard_listing_data_received(bt_address_t* addr, char* obj, uint16_t len)
{
    bt_status_t status;
    char* data = NULL;

    if (obj != NULL && len > 0) {
        data = malloc(len + 1);
        if (data == NULL) {
            BT_LOGE("%s: malloc failed", __func__);
            return;
        }
        strlcpy(data, obj, len + 1);
    }

    status = do_in_pbap_pce_service(addr, PCE_PULL_VCARD_LIST_DATA_EVT, data);

    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: do in pbap service failed, status: %d", __func__, status);
        free(data);
        return;
    }
}

void pce_on_vcard_listing_end(bt_address_t* addr, uint16_t status)
{
    uint16_t* data;

    data = malloc(sizeof(uint16_t));
    *data = status;

    status = do_in_pbap_pce_service(addr, PCE_PULL_VCARD_LIST_END_EVT, data);

    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: do in pbap service failed, status: %d", __func__, status);
        free(data);
        return;
    }
}

void pce_on_vcard_data_received(bt_address_t* addr, char* obj, uint16_t len)
{
    int status;
    char* data = NULL;

    if (obj != NULL && len > 0) {
        data = malloc(len + 1);
        if (data == NULL) {
            BT_LOGE("%s: malloc failed", __func__);
            return;
        }
        strlcpy(data, obj, len + 1);
    }

    status = do_in_pbap_pce_service(addr, PCE_PULL_VCARD_DATA_EVT, data);

    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: do in pbap service failed, status: %d", __func__, status);
        free(data);
        return;
    }
}

void pce_on_vcard_end(bt_address_t* addr, uint16_t status)
{
    uint16_t* data;

    data = malloc(sizeof(uint16_t));
    *data = status;

    status = do_in_pbap_pce_service(addr, PCE_PULL_VCARD_END_EVT, data);

    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: do in pbap service failed, status: %d", __func__, status);
        free(data);
        return;
    }
}

static bt_status_t pce_init(void)
{
    BT_LOGD("%s", __func__);

    g_pce.callbacks = bt_callbacks_list_new(2);

    return BT_STATUS_SUCCESS;
}

static bt_status_t pce_startup(profile_on_startup_t cb)
{
    bt_status_t status = BT_STATUS_FAIL;

    BT_LOGD("%s", __func__);
    if (g_pce.enable)
        goto exit;

    g_pce.devices = bt_list_new(pce_device_destory);
    if (g_pce.devices == NULL) {
        BT_LOGE("%s: devices init failed", __func__);
        goto exit;
    }

    status = bt_sal_pbap_pce_init();
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: sal init failed", __func__);
        goto exit;
    }

    g_pce.enable = true;
    status = BT_STATUS_SUCCESS;

exit:
    if (status != BT_STATUS_SUCCESS) {
        bt_list_free(g_pce.devices);
        g_pce.devices = NULL;
    }

    cb(PROFILE_PBAP_PCE, g_pce.enable);
    return status;
}

static bt_status_t pce_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);
    if (!g_pce.enable)
        goto exit;

    g_pce.enable = false;

    bt_list_free(g_pce.devices);
    g_pce.devices = NULL;

exit:
    return BT_STATUS_SUCCESS;
}

static int pce_get_state(void)
{
    return g_pce.enable;
}

static const void* get_pce_profile_interface(void)
{
    return (void*)&pceInterface;
}

static void pce_cleanup(void)
{
    BT_LOGD("%s", __func__);

    bt_callbacks_list_free(g_pce.callbacks);
    g_pce.callbacks = NULL;
    if (g_pce.devices)
        bt_list_free(g_pce.devices);
    g_pce.devices = NULL;

    memset(&g_pce, 0, sizeof(g_pce));
    return;
}

static int pce_dump(void)
{
    BT_LOGD("%s", __func__);
    return 0;
}

static const profile_service_t pce_service = {
    .auto_start = true,
    .name = PROFILE_PBAP_PCE_NAME,
    .id = PROFILE_PBAP_PCE,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = pce_init,
    .startup = pce_startup,
    .shutdown = pce_shutdown,
    .process_msg = NULL,
    .get_state = pce_get_state,
    .get_profile_interface = get_pce_profile_interface,
    .cleanup = pce_cleanup,
    .dump = pce_dump,
};

void register_pce_service(void)
{
    register_service(&pce_service);
}
