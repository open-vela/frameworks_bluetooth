/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#include "pbap_pce_service.h"
#include "sal_pbap_pce_interface.h"
#include "service_loop.h"
#include "service_manager.h"

#include "utils/log.h"

#define PCE_MAX_CONNECTIONS (1)

#define PCE_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, pbap_pce_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node conn_list;
    bool enable;
    pthread_mutex_t pce_lock;
    callbacks_list_t* callbacks;
} pce_global_t;

typedef struct {
    struct list_node node;
    bt_address_t addr;
    uint8_t state;
} pce_conn_t;

typedef struct {
    profile_connection_state_t state;
} pce_conn_evt_t;

typedef struct {
    uint16_t status;
} pce_status_evt_t;

typedef struct {
    uint16_t len;
    char* data;
} pce_data_evt_t;

typedef struct {
    bt_address_t addr;
    enum {
        PCE_CONNECTION_EVT,
        PCE_DIR_EVT,
        PCE_VCARD_LISTING_DATA_EVT,
        PCE_VCARD_LISTING_END_EVT,
        PCE_VCARD_DATA_EVT,
        PCE_VCARD_END_EVT,
    } evt_id;
    union {
        pce_conn_evt_t conn_evt;
        pce_status_evt_t dir_evt;
        pce_data_evt_t vcard_listing_evt;
        pce_status_evt_t vcard_listing_end_evt;
        pce_data_evt_t vcard_evt;
        pce_status_evt_t vcard_end_evt;
    };
} pce_msg_t;

static pce_global_t g_pce = { 0 };

static pce_conn_t* pce_find_conn(bt_address_t* addr);
static void pce_conn_close(pce_conn_t* conn);

static pce_conn_t* pce_find_new_conn(bt_address_t* addr)
{
    pce_conn_t* conn;

    if (list_length(&g_pce.conn_list) == PCE_MAX_CONNECTIONS) {
        BT_LOGD("%s, PCE_MAX_CONNECTIONS", __func__);
        return NULL;
    }

    conn = pce_find_conn(addr);
    if (conn)
        return conn;

    conn = malloc(sizeof(pce_conn_t));
    if (!conn) {
        BT_LOGE("%s, memory full", __func__);
        return NULL;
    }

    memcpy(&conn->addr, addr, sizeof(bt_address_t));
    list_add_tail(&g_pce.conn_list, &conn->node);

    return conn;
}

static void pce_free_conn(pce_conn_t* conn)
{
    list_delete(&conn->node);
    free(conn);
}

static pce_conn_t* pce_find_conn(bt_address_t* addr)
{
    pce_conn_t* conn;
    struct list_node* node;

    list_for_every(&g_pce.conn_list, node)
    {
        conn = (pce_conn_t*)node;
        if (!memcmp(addr, &conn->addr, sizeof(bt_address_t)))
            return conn;
    }

    return NULL;
}

static void pce_close_all_conn(void)
{
    pce_conn_t* conn;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_pce.conn_list, node, tmp)
    {
        conn = (pce_conn_t*)node;
        pce_conn_close(conn);
    }
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
    bt_status_t status;

    pthread_mutex_lock(&g_pce.pce_lock);
    if (!g_pce.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    status = bt_sal_pce_connect(addr);

exit:
    pthread_mutex_unlock(&g_pce.pce_lock);
    return status;
}

static bt_status_t pce_disconnect(bt_address_t* addr)
{
    bt_status_t status;

    pthread_mutex_lock(&g_pce.pce_lock);
    if (!g_pce.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    status = bt_sal_pce_disconnect(addr);

exit:
    pthread_mutex_unlock(&g_pce.pce_lock);
    return status;
}

static bt_status_t pce_change_directory(bt_address_t* addr, const char* dir)
{
    bt_status_t status;

    pthread_mutex_lock(&g_pce.pce_lock);
    if (!g_pce.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (!dir || (strlen(dir) > 0xFF)) {
        status = BT_STATUS_PARM_INVALID;
        goto exit;
    }

    status = bt_sal_pce_change_directory(addr, dir);

exit:
    pthread_mutex_unlock(&g_pce.pce_lock);
    return status;
}

static bt_status_t pce_pull_vcard_listing(bt_address_t* addr, pbap_search_property_t property, const char* value)
{
    bt_status_t status;

    pthread_mutex_lock(&g_pce.pce_lock);
    if (!g_pce.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (value && (strlen(value) > 0xFF)) {
        status = BT_STATUS_PARM_INVALID;
        goto exit;
    }

    status = bt_sal_pce_pull_vcard_listing(addr, property, value);

exit:
    pthread_mutex_unlock(&g_pce.pce_lock);
    return status;
}

static bt_status_t pce_pull_vcard(bt_address_t* addr, const char* object, uint64_t filter)
{
    bt_status_t status;

    pthread_mutex_lock(&g_pce.pce_lock);
    if (!g_pce.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (!object || (strlen(object) > 0xFF)) {
        status = BT_STATUS_PARM_INVALID;
        goto exit;
    }

    status = bt_sal_pce_pull_vcard(addr, object, filter);

exit:
    pthread_mutex_unlock(&g_pce.pce_lock);
    return status;
}

static const pbap_pce_interface_t pceInterface = {
    .size = sizeof(pceInterface),
    .register_callbacks = pce_register_callbacks,
    .unregister_callbacks = pce_unregister_callbacks,
    .connect = pce_connect,
    .disconnect = pce_disconnect,
    .change_directory = pce_change_directory,
    .pull_vcard_listing = pce_pull_vcard_listing,
    .pull_vcard = pce_pull_vcard,
};

static pce_conn_t* pce_conn_open(bt_address_t* addr)
{
    pce_conn_t* conn;

    conn = pce_find_new_conn(addr);
    if (!conn)
        goto error;

    conn->state = PROFILE_STATE_CONNECTED;

    return conn;

error:
    if (conn)
        pce_conn_close(conn);
    else
        bt_sal_pce_disconnect(addr);
    return NULL;
}

static void pce_conn_close(pce_conn_t* conn)
{
    if (conn == NULL)
        return;

    if (conn->state != PROFILE_STATE_DISCONNECTED && conn->state != PROFILE_STATE_DISCONNECTING)
        bt_sal_pce_disconnect(&conn->addr);

    pce_free_conn(conn);
}

static void on_pce_connection_state_changed(bt_address_t* addr, pce_conn_evt_t* evt)
{
    pce_conn_t* conn;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s, addr: %s, state: %d", __func__, addr_str, evt->state);

    switch (evt->state) {
    case PROFILE_STATE_DISCONNECTED: {
        conn = pce_find_conn(addr);
        pce_conn_close(conn);
        break;
    }
    case PROFILE_STATE_CONNECTED:
        conn = pce_conn_open(addr);
        break;
    case PROFILE_STATE_CONNECTING:
    case PROFILE_STATE_DISCONNECTING:
    default:
        break;
    }

    PCE_CALLBACK_FOREACH(g_pce.callbacks, connection_state_cb, addr, evt->state);
}

static void on_pce_dir_changed(bt_address_t* addr, pce_status_evt_t* evt)
{
    PCE_CALLBACK_FOREACH(g_pce.callbacks, dir_changed_cb, addr, evt->status);
}

static void on_pce_vcard_listing_data_received(bt_address_t* addr, pce_data_evt_t* evt)
{
    PCE_CALLBACK_FOREACH(g_pce.callbacks, vcard_listing_data_cb, addr, evt->len, evt->data);
}

static void on_pce_vcard_listing_end(bt_address_t* addr, pce_status_evt_t* evt)
{
    PCE_CALLBACK_FOREACH(g_pce.callbacks, vcard_listing_end_cb, addr, evt->status);
}

static void on_pce_vcard_data_received(bt_address_t* addr, pce_data_evt_t* evt)
{
    PCE_CALLBACK_FOREACH(g_pce.callbacks, vcard_data_cb, addr, evt->len, evt->data);
}

static void on_pce_vcard_end(bt_address_t* addr, pce_status_evt_t* evt)
{
    PCE_CALLBACK_FOREACH(g_pce.callbacks, vcard_end_cb, addr, evt->status);
}

static void pce_service_event_process(void* data)
{
    pce_msg_t* msg = data;

    pthread_mutex_lock(&g_pce.pce_lock);

    switch (msg->evt_id) {
    case PCE_CONNECTION_EVT:
        on_pce_connection_state_changed(&msg->addr, &msg->conn_evt);
        break;
    case PCE_DIR_EVT:
        on_pce_dir_changed(&msg->addr, &msg->dir_evt);
        break;
    case PCE_VCARD_LISTING_DATA_EVT:
        on_pce_vcard_listing_data_received(&msg->addr, &msg->vcard_listing_evt);
        break;
    case PCE_VCARD_LISTING_END_EVT:
        on_pce_vcard_listing_end(&msg->addr, &msg->vcard_listing_end_evt);
        break;
    case PCE_VCARD_DATA_EVT:
        on_pce_vcard_data_received(&msg->addr, &msg->vcard_evt);
        break;
    case PCE_VCARD_END_EVT:
        on_pce_vcard_end(&msg->addr, &msg->vcard_end_evt);
        break;
    default:
        break;
    }

    pthread_mutex_unlock(&g_pce.pce_lock);
    free(data);
    return;
}

void pce_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state)
{
    pce_msg_t* pce_msg = (pce_msg_t*)malloc(sizeof(pce_msg_t));
    if (pce_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    memcpy(&pce_msg->addr, addr, sizeof(bt_address_t));
    pce_msg->evt_id = PCE_CONNECTION_EVT;
    pce_msg->conn_evt.state = state;

    do_in_service_loop(pce_service_event_process, pce_msg);
}

void pce_on_dir_changed(bt_address_t* addr, uint16_t status)
{
    pce_msg_t* pce_msg = (pce_msg_t*)malloc(sizeof(pce_msg_t));
    if (pce_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    memcpy(&pce_msg->addr, addr, sizeof(bt_address_t));
    pce_msg->evt_id = PCE_DIR_EVT;
    pce_msg->dir_evt.status = status;

    do_in_service_loop(pce_service_event_process, pce_msg);
}

void pce_on_vcard_listing_data_received(bt_address_t* addr, char* obj, uint16_t len)
{
    pce_msg_t* pce_msg = (pce_msg_t*)malloc(sizeof(pce_msg_t));
    if (pce_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    memcpy(&pce_msg->addr, addr, sizeof(bt_address_t));
    pce_msg->evt_id = PCE_VCARD_LISTING_DATA_EVT;
    if (obj != NULL && len > 0) {
        pce_msg->vcard_listing_evt.data = malloc(len);
        if (pce_msg->vcard_listing_evt.data == NULL) {
            BT_LOGE("%s malloc failed", __func__);
            free(pce_msg);
            return;
        }
        memcpy(pce_msg->vcard_listing_evt.data, obj, len);
        pce_msg->vcard_listing_evt.len = len;
    } else {
        pce_msg->vcard_listing_evt.data = NULL;
        pce_msg->vcard_listing_evt.len = 0;
    }

    do_in_service_loop(pce_service_event_process, pce_msg);
}

void pce_on_vcard_listing_end(bt_address_t* addr, uint16_t status)
{
    pce_msg_t* pce_msg = (pce_msg_t*)malloc(sizeof(pce_msg_t));
    if (pce_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    memcpy(&pce_msg->addr, addr, sizeof(bt_address_t));
    pce_msg->evt_id = PCE_VCARD_LISTING_END_EVT;
    pce_msg->vcard_listing_end_evt.status = status;

    do_in_service_loop(pce_service_event_process, pce_msg);
}

void pce_on_vcard_data_received(bt_address_t* addr, char* obj, uint16_t len)
{
    pce_msg_t* pce_msg = (pce_msg_t*)malloc(sizeof(pce_msg_t));
    if (pce_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    memcpy(&pce_msg->addr, addr, sizeof(bt_address_t));
    pce_msg->evt_id = PCE_VCARD_DATA_EVT;
    if (obj != NULL && len > 0) {
        pce_msg->vcard_evt.data = malloc(len);
        if (pce_msg->vcard_evt.data == NULL) {
            BT_LOGE("%s malloc failed", __func__);
            free(pce_msg);
            return;
        }
        memcpy(pce_msg->vcard_evt.data, obj, len);
        pce_msg->vcard_evt.len = len;
    } else {
        pce_msg->vcard_evt.data = NULL;
        pce_msg->vcard_evt.len = 0;
    }

    do_in_service_loop(pce_service_event_process, pce_msg);
}

void pce_on_vcard_end(bt_address_t* addr, uint16_t status)
{
    pce_msg_t* pce_msg = (pce_msg_t*)malloc(sizeof(pce_msg_t));
    if (pce_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    memcpy(&pce_msg->addr, addr, sizeof(bt_address_t));
    pce_msg->evt_id = PCE_VCARD_END_EVT;
    pce_msg->vcard_end_evt.status = status;

    do_in_service_loop(pce_service_event_process, pce_msg);
}

static bt_status_t pce_init(void)
{
    BT_LOGD("%s", __func__);

    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_pce.pce_lock, &attr) < 0)
        return BT_STATUS_FAIL;

    g_pce.callbacks = bt_callbacks_list_new(2);

    return BT_STATUS_SUCCESS;
}

static bt_status_t pce_startup(profile_on_startup_t cb)
{
    BT_LOGD("%s", __func__);
    pthread_mutex_lock(&g_pce.pce_lock);
    if (g_pce.enable)
        goto exit;

    list_initialize(&g_pce.conn_list);
    if (bt_sal_pce_init() != BT_STATUS_SUCCESS)
        goto error;

    g_pce.enable = true;

exit:
    cb(PROFILE_PBAP_PCE, true);
    pthread_mutex_unlock(&g_pce.pce_lock);
    return BT_STATUS_SUCCESS;

error:
    pthread_mutex_unlock(&g_pce.pce_lock);
    list_delete(&g_pce.conn_list);
    cb(PROFILE_PBAP_PCE, false);
    return BT_STATUS_FAIL;
}

static bt_status_t pce_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);
    pthread_mutex_lock(&g_pce.pce_lock);
    if (!g_pce.enable)
        goto exit;

    g_pce.enable = false;
    pce_close_all_conn();

exit:
    cb(PROFILE_PBAP_PCE, true);
    pthread_mutex_unlock(&g_pce.pce_lock);
    return BT_STATUS_SUCCESS;
}

static int pce_get_state(void)
{
    return 1;
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
    pthread_mutex_destroy(&g_pce.pce_lock);

    return;
}

static int pce_dump(void)
{
    BT_LOGD("%s", __func__);

    /* Do someting here */

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
