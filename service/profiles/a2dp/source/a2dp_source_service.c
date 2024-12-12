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
#define LOG_TAG "a2dp_source"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "a2dp_audio.h"
#include "a2dp_device.h"
#include "a2dp_source_service.h"
#include "adapter_internel.h"
#include "bt_a2dp_source.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "callbacks_list.h"
#include "sal_a2dp_source_interface.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#ifndef CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#define A2DP_MAX_CONNECTION (1)
#else
#define A2DP_MAX_CONNECTION CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#endif

#define A2DP_SOURCE_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, a2dp_source_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node list;
    bool enabled;
    bool offloading;
    callbacks_list_t* callbacks;
    a2dp_peer_t* active_peer;
} a2dp_source_global_t;

static a2dp_source_global_t g_a2dp_source = { 0 };

void do_in_a2dp_service(a2dp_event_t* a2dp_event);

static void source_shutdown(void* data);
static void source_startup(void* data);
static bool a2dp_source_unregister_callbacks(void** remote, void* cookie);

static void set_active_peer(bt_address_t* bd_addr, uint16_t acl_hdl)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_source.list, bd_addr);

    if (!device) {
        BT_LOGE("No A2DP device found with the provided address:%s", bt_addr_str(bd_addr));
        return;
    }

    g_a2dp_source.active_peer = &device->peer;
    device->peer.acl_hdl = acl_hdl;
}

static a2dp_peer_t* get_active_peer(void)
{
    return g_a2dp_source.active_peer;
}

static a2dp_device_t* find_or_create_device(bt_address_t* bd_addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_source.list, bd_addr);
    if (device)
        return device;

    device = a2dp_device_new(&g_a2dp_source, SEP_SNK, bd_addr);
    if (!device) {
        BT_LOGE("A2DP new source device alloc failed");
        return NULL;
    }
    list_add_tail(&g_a2dp_source.list, &device->node);

    return device;
}

static a2dp_state_machine_t* get_state_machine(bt_address_t* bd_addr)
{
    a2dp_device_t* device = find_or_create_device(bd_addr);

    if (!device)
        return NULL;

    return device->a2dp_sm;
}

static void save_a2dp_codec_config(a2dp_peer_t* peer, a2dp_codec_config_t* config)
{
    if (peer == NULL || config == NULL)
        return;

    memcpy(&peer->codec_config, config, sizeof(*config));
    a2dp_codec_set_config(SEP_SNK, &peer->codec_config);
}

static void a2dp_service_prepare_handle(a2dp_state_machine_t* sm,
    a2dp_event_t* event)
{
    switch (event->event) {
    case CONNECTED_EVT: {
        set_active_peer(&event->event_data.bd_addr, bt_sal_get_acl_connection_handle(PRIMARY_ADAPTER, &event->event_data.bd_addr, BT_TRANSPORT_BREDR));
        break;
    }

    case STREAM_STARTED_EVT: {
        a2dp_offload_config_t config = { 0 };
        a2dp_codec_config_t* codec_config;
        a2dp_device_t* device;
        uint8_t param[CONFIG_VSC_MAX_LEN];
        size_t size;
        bool ret;

        if (!g_a2dp_source.offloading) {
            break;
        }

        device = find_a2dp_device_by_addr(&g_a2dp_source.list, &event->event_data.bd_addr);
        if (!device) {
            BT_LOGE("A2DP find_device_by_addr:%s failed", bt_addr_str(&event->event_data.bd_addr));
            break;
        }

        codec_config = &device->peer.codec_config;
        codec_config->l2c_rcid = event->event_data.l2c_rcid;
        codec_config->acl_hdl = device->peer.acl_hdl;
        save_a2dp_codec_config(&device->peer, codec_config);

        ret = a2dp_codec_get_offload_config(&config);
        if (!ret) {
            BT_LOGE("A2DP codec_get_offload_config failed");
            break;
        }

        ret = a2dp_offload_start_builder(&config, param, &size);
        if (!ret) {
            BT_LOGE("A2DP codec_offload_start_builder failed");
            break;
        }

        event->event = OFFLOAD_START_REQ;
        free(event->event_data.data);
        event->event_data.data = malloc(size);
        event->event_data.size = size;
        memcpy(event->event_data.data, param, size);
        break;
    }

    case STREAM_CLOSED_EVT:
    case STREAM_SUSPENDED_EVT: {
        a2dp_offload_config_t config = { 0 };
        uint8_t param[OFFLOAD_START_REQ];
        bool ret;
        size_t size;

        if (!g_a2dp_source.offloading) {
            break;
        }

        ret = a2dp_codec_get_offload_config(&config);
        if (!ret) {
            BT_LOGE("A2DP codec_get_offload_config failed");
            break;
        }

        ret = a2dp_offload_stop_builder(&config, param, &size);
        if (!ret) {
            BT_LOGE("A2DP codec_offload_stop_builder failed");
            break;
        }

        do_in_a2dp_service(a2dp_event_new_ext(OFFLOAD_STOP_REQ, &event->event_data.bd_addr, param, size));
        break;
    }
    default:
        break;
    }
}

static void a2dp_service_handle_event(void* data)
{
    a2dp_event_t* event = data;

    /* msg cleanup ? */
    if (!g_a2dp_source.enabled && event->event != A2DP_STARTUP) {
        a2dp_event_destory(event);
        return;
    }

    switch (event->event) {
    case A2DP_STARTUP:
        source_startup(event->event_data.cb);
        break;
    case A2DP_SHUTDOWN:
        source_shutdown(event->event_data.cb);
        break;
    case CODEC_CONFIG_EVT: {
        a2dp_codec_config_t* config;
        a2dp_device_t* device;

        device = find_or_create_device(&event->event_data.bd_addr);
        if (device == NULL) {
            break;
        }

        config = event->event_data.data;
        BT_LOGD("CODEC_CONFIG_EVT : codec_type: %d, sample_rate: %" PRIu32 ", bits_per_sample: %d, channel_mode: %d",
            config->codec_type,
            config->sample_rate,
            config->bits_per_sample,
            config->channel_mode);
        save_a2dp_codec_config(&device->peer, config);
        break;
    }
    case STREAM_MTU_CONFIG_EVT: {
        a2dp_device_t* device = find_or_create_device(&event->event_data.bd_addr);
        if (device == NULL) {
            break;
        }

        device->peer.mtu = event->event_data.mtu;
        BT_LOGD("STREAM_MTU_CONFIG_EVT :%d", device->peer.mtu);
        a2dp_codec_update_config(SEP_SNK, &device->peer.codec_config, device->peer.mtu);
        break;
    }
    default: {
        a2dp_state_machine_t* a2dp_sm;

        a2dp_sm = get_state_machine(&event->event_data.bd_addr);
        if (!a2dp_sm) {
            break;
        }

        a2dp_service_prepare_handle(a2dp_sm, event);
        a2dp_state_machine_handle_event(a2dp_sm, event);
        break;
    }
    }

    a2dp_event_destory(event);
}

void do_in_a2dp_service(a2dp_event_t* a2dp_event)
{
    if (a2dp_event == NULL)
        return;

    do_in_service_loop(a2dp_service_handle_event, a2dp_event);
}

void bt_sal_a2dp_source_event_callback(a2dp_event_t* event)
{
    do_in_a2dp_service(event);
}

a2dp_peer_t* a2dp_source_active_peer(void)
{
    return get_active_peer();
}

a2dp_peer_t* a2dp_source_find_peer(bt_address_t* addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_source.list, addr);

    if (!device)
        return NULL;

    return &device->peer;
}

void a2dp_source_stream_start(void)
{
    a2dp_peer_t* peer = a2dp_source_active_peer();
    if (!peer)
        return;

    do_in_a2dp_service(a2dp_event_new(STREAM_START_REQ, peer->bd_addr));
}

void a2dp_source_stream_prepare_suspend(void)
{
    a2dp_audio_prepare_suspend(SEP_SNK);
}

void a2dp_source_stream_stop(void)
{
    a2dp_peer_t* peer = a2dp_source_active_peer();
    if (!peer)
        return;

    do_in_a2dp_service(a2dp_event_new(STREAM_SUSPEND_REQ, peer->bd_addr));
}

bool a2dp_source_stream_ready(void)
{
    a2dp_state_machine_t* a2dp_sm;
    a2dp_state_t state;
    a2dp_peer_t* peer = a2dp_source_active_peer();
    if (!peer)
        return false;

    a2dp_sm = get_state_machine(peer->bd_addr);
    if (!a2dp_sm)
        return false;

    state = a2dp_state_machine_get_state(a2dp_sm);
    if (state == A2DP_STATE_OPENED || state == A2DP_STATE_STARTED)
        return true;

    return false;
}

bool a2dp_source_stream_started(void)
{
    a2dp_state_machine_t* a2dp_sm;
    a2dp_peer_t* peer = a2dp_source_active_peer();
    if (!peer)
        return false;

    a2dp_sm = get_state_machine(peer->bd_addr);
    if (!a2dp_sm)
        return false;

    if (a2dp_state_machine_is_pending_stop(a2dp_sm))
        return false;

    return a2dp_state_machine_get_state(a2dp_sm) == A2DP_STATE_STARTED;
}

void a2dp_source_codec_state_change(void)
{
    a2dp_peer_t* peer = a2dp_source_active_peer();
    if (!peer)
        return;

    do_in_a2dp_service(a2dp_event_new(DEVICE_CODEC_STATE_CHANGE_EVT, peer->bd_addr));
}

// show Device[1]: Addr: 04:7F:0E:00:00:1B, State: Opened, Active: true
static int a2dp_source_dump(void)
{
    a2dp_device_t* device;
    struct list_node* node;
    int i = 0;
    uint8_t is_active;
    const char* state;
    list_for_every(&g_a2dp_source.list, node)
    {
        i++;
        device = (a2dp_device_t*)node;
        if (memcmp(&device->bd_addr, g_a2dp_source.active_peer, 6) == 0)
            is_active = 1;
        else
            is_active = 0;
        state = a2dp_state_machine_current_state(device->a2dp_sm);
        BT_LOGD("\tDevice[%d]: Addr: %s, State: %s, Active: %s\n", i,
            bt_addr_str(&device->bd_addr), state, is_active ? "true" : "false");
    }
    if (i == 0)
        BT_LOGE("\tNo A2dp Sink device found\n");

    return 0;
}

static bt_status_t a2dp_source_init(void)
{
    g_a2dp_source.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    return BT_STATUS_SUCCESS;
}

static void a2dp_source_cleanup(void)
{
    g_a2dp_source.active_peer = NULL;

    bt_callbacks_list_free(g_a2dp_source.callbacks);
    g_a2dp_source.callbacks = NULL;
}

static void source_startup(void* data)
{
    profile_on_startup_t on_startup = (profile_on_startup_t)data;

    list_initialize(&g_a2dp_source.list);
    if (bt_sal_a2dp_source_init(A2DP_MAX_CONNECTION) != BT_STATUS_SUCCESS) {
        list_delete(&g_a2dp_source.list);
        on_startup(PROFILE_A2DP, false);
        return;
    }

    a2dp_audio_init(SVR_SOURCE, g_a2dp_source.offloading);
    g_a2dp_source.enabled = true;
    on_startup(PROFILE_A2DP, true);
}

static bt_status_t a2dp_source_startup(profile_on_startup_t cb)
{
    if (g_a2dp_source.enabled) {
        return BT_STATUS_BUSY;
    }

    a2dp_event_t* evt = a2dp_event_new(A2DP_STARTUP, NULL);
    evt->event_data.cb = cb;
    do_in_a2dp_service(evt);

    return BT_STATUS_SUCCESS;
}

static void source_shutdown(void* data)
{
    a2dp_device_t* device;
    struct list_node* node;
    struct list_node* tmp;
    profile_on_shutdown_t on_shutdown = (profile_on_shutdown_t)data;

    g_a2dp_source.enabled = false;
    a2dp_audio_cleanup(SVR_SOURCE);
    list_for_every_safe(&g_a2dp_source.list, node, tmp)
    {
        device = (a2dp_device_t*)node;
        a2dp_device_delete(device);
    }
    list_delete(&g_a2dp_source.list);
    bt_sal_a2dp_source_cleanup();
    g_a2dp_source.active_peer = NULL;
    on_shutdown(PROFILE_A2DP, true);
}

static bt_status_t a2dp_source_shutdown(profile_on_shutdown_t cb)
{
    if (!g_a2dp_source.enabled) {
        return BT_STATUS_SUCCESS;
    }

    a2dp_event_t* evt = a2dp_event_new(A2DP_SHUTDOWN, NULL);
    evt->event_data.cb = cb;
    do_in_a2dp_service(evt);

    return BT_STATUS_SUCCESS;
}

static void a2dp_source_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_A2DP_OFFLOADING:
        g_a2dp_source.offloading = msg->data.valuebool;
        break;
    case PROFILE_EVT_REMOTE_DETACH: {
        bt_instance_t* ins = msg->data.data;

        if (ins->a2dp_source_cookie) {
            BT_LOGD("%s PROFILE_EVT_REMOTE_DETACH", __func__);
            a2dp_source_unregister_callbacks(NULL, ins->a2dp_source_cookie);
            ins->a2dp_source_cookie = NULL;
        }
        break;
    }
    default:
        break;
    }
}

void a2dp_source_service_notify_connection_state_changed(
    bt_address_t* addr, profile_connection_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    A2DP_SOURCE_CALLBACK_FOREACH(g_a2dp_source.callbacks, connection_state_cb, addr, state);
}

void a2dp_source_service_notify_audio_state_changed(
    bt_address_t* addr, a2dp_audio_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    A2DP_SOURCE_CALLBACK_FOREACH(g_a2dp_source.callbacks, audio_state_cb, addr, state);
}

void a2dp_source_service_notify_audio_source_config_changed(
    bt_address_t* addr)
{
    BT_LOGD("%s", __FUNCTION__);
    A2DP_SOURCE_CALLBACK_FOREACH(g_a2dp_source.callbacks, audio_source_config_cb, addr);
}

static void* a2dp_source_register_callbacks(void* remote, const a2dp_source_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_a2dp_source.callbacks, remote, (void*)callbacks);
}

static bool a2dp_source_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_a2dp_source.callbacks, remote, cookie);
}

static bool a2dp_source_is_connected(bt_address_t* addr)
{
    if (!g_a2dp_source.enabled) {
        return false;
    }

    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_source.list, addr);
    if (!device) {
        return false;
    }

    profile_connection_state_t state = a2dp_state_machine_get_connection_state(device->a2dp_sm);

    return state == PROFILE_STATE_CONNECTED;
}

static bool a2dp_source_is_playing(bt_address_t* addr)
{
    if (!g_a2dp_source.enabled) {
        return false;
    }

    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_source.list, addr);
    if (!device) {
        return false;
    }

    a2dp_state_t state = a2dp_state_machine_get_state(device->a2dp_sm);
    return state == A2DP_STATE_STARTED;
}

static profile_connection_state_t a2dp_source_get_connection_state(bt_address_t* addr)
{
    if (!g_a2dp_source.enabled) {
        return PROFILE_STATE_DISCONNECTED;
    }

    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_source.list, addr);
    if (!device) {
        return PROFILE_STATE_DISCONNECTED;
    }

    profile_connection_state_t state = a2dp_state_machine_get_connection_state(device->a2dp_sm);
    return state;
}

static bt_status_t a2dp_source_connect(bt_address_t* addr)
{
    if (!g_a2dp_source.enabled) {
        return PROFILE_STATE_DISCONNECTED;
    }

    do_in_a2dp_service(a2dp_event_new(CONNECT_REQ, addr));

    return BT_STATUS_SUCCESS;
}

static bt_status_t a2dp_source_disconnect(bt_address_t* addr)
{
    if (!g_a2dp_source.enabled) {
        return PROFILE_STATE_DISCONNECTED;
    }

    do_in_a2dp_service(a2dp_event_new(DISCONNECT_REQ, addr));

    return BT_STATUS_SUCCESS;
}

static bt_status_t a2dp_source_set_silence_device(bt_address_t* addr, bool silence)
{
    return BT_STATUS_SUCCESS;
}

static bt_status_t a2dp_source_set_active_device(bt_address_t* addr)
{
    return BT_STATUS_SUCCESS;
}

static int a2dp_src_get_state(void)
{
    return 1;
}

static const a2dp_source_interface_t a2dp_sourceInterface = {
    .size = sizeof(a2dp_sourceInterface),
    .register_callbacks = a2dp_source_register_callbacks,
    .unregister_callbacks = a2dp_source_unregister_callbacks,
    .is_connected = a2dp_source_is_connected,
    .is_playing = a2dp_source_is_playing,
    .get_connection_state = a2dp_source_get_connection_state,
    .connect = a2dp_source_connect,
    .disconnect = a2dp_source_disconnect,
    .set_silence_device = a2dp_source_set_silence_device,
    .set_active_device = a2dp_source_set_active_device,
};

static const void* get_a2dp_source_profile_interface(void)
{
    return (void*)&a2dp_sourceInterface;
}

static const profile_service_t a2dp_source_service = {
    .auto_start = true,
    .name = PROFILE_A2DP_NAME,
    .id = PROFILE_A2DP,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = a2dp_source_init,
    .startup = a2dp_source_startup,
    .shutdown = a2dp_source_shutdown,
    .process_msg = a2dp_source_process_msg,
    .get_state = a2dp_src_get_state,
    .get_profile_interface = get_a2dp_source_profile_interface,
    .cleanup = a2dp_source_cleanup,
    .dump = a2dp_source_dump,
};

void register_a2dp_source_service(void)
{
    register_service(&a2dp_source_service);
}
