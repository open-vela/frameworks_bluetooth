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
#define LOG_TAG "a2dp_sink"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "a2dp_audio.h"
#include "a2dp_device.h"
#include "a2dp_sink_service.h"
#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "callbacks_list.h"
#include "sal_a2dp_sink_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#ifndef CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#define A2DP_MAX_CONNECTION (1)
#else
#define A2DP_MAX_CONNECTION CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#endif

#define A2DP_SINK_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, a2dp_sink_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node list;
    bool enabled;
    bool offloading;
    callbacks_list_t* callbacks;
    a2dp_peer_t* active_peer;
} a2dp_sink_global_t;

static a2dp_sink_global_t g_a2dp_sink = { 0 };

static void sink_startup(void* data);
static void sink_shutdown(void* data);
static bool a2dp_sink_unregister_callbacks(void** remote, void* cookie);

static void set_active_peer(bt_address_t* bd_addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_sink.list, bd_addr);

    g_a2dp_sink.active_peer = &device->peer;
}

static a2dp_peer_t* get_active_peer(void)
{
    return g_a2dp_sink.active_peer;
}

static a2dp_device_t* find_or_create_device(bt_address_t* bd_addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_sink.list, bd_addr);
    if (device)
        return device;

    device = a2dp_device_new(&g_a2dp_sink, SEP_SRC, bd_addr);
    if (!device) {
        BT_LOGE("A2DP new source device alloc failed");
        return NULL;
    }
    list_add_tail(&g_a2dp_sink.list, &device->node);

    return device;
}

static a2dp_state_machine_t* get_state_machine(bt_address_t* bd_addr)
{
    a2dp_device_t* device = find_or_create_device(bd_addr);

    if (!device)
        return NULL;

    return device->a2dp_sm;
}

void save_a2dp_codec_config(a2dp_peer_t* peer, a2dp_codec_config_t* config)
{
    if (peer == NULL || config == NULL)
        return;

    memcpy(&peer->codec_config, config, sizeof(*config));
    a2dp_codec_set_config(SEP_SRC, &peer->codec_config);
}

static void a2dp_snk_service_handle_event(void* data)
{
    a2dp_event_t* event = data;

    if (!g_a2dp_sink.enabled && event->event != A2DP_STARTUP) {
        a2dp_event_destory(event);
        return;
    }

    switch (event->event) {
    case A2DP_STARTUP:
        sink_startup(event->event_data.cb);
        break;
    case A2DP_SHUTDOWN:
        sink_shutdown(event->event_data.cb);
        break;
    case CODEC_CONFIG_EVT: {
        a2dp_codec_config_t* config;
        a2dp_device_t* device;
        device = find_or_create_device(&event->event_data.bd_addr);
        if (!device) {
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
    case PEER_STREAM_START_REQ:
        bt_sal_a2dp_sink_start_stream(PRIMARY_ADAPTER, &event->event_data.bd_addr);
        break;
    default: {
        a2dp_state_machine_t* a2dp_sm;
        a2dp_sm = get_state_machine(&event->event_data.bd_addr);
        if (!a2dp_sm) {
            break;
        }

        if (event->event == CONNECTED_EVT)
            set_active_peer(&event->event_data.bd_addr);

        a2dp_state_machine_handle_event(a2dp_sm, event);
        break;
    }
    }

    a2dp_event_destory(event);
}

static void do_in_a2dp_snk_service(a2dp_event_t* a2dp_event)
{
    if (a2dp_event == NULL)
        return;

    do_in_service_loop(a2dp_snk_service_handle_event, a2dp_event);
}

a2dp_peer_t* a2dp_sink_find_peer(bt_address_t* addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_sink.list, addr);

    if (!device)
        return NULL;

    return &device->peer;
}

bool a2dp_sink_stream_ready(void)
{
    a2dp_state_machine_t* a2dp_sm;
    a2dp_state_t state;
    a2dp_peer_t* peer = get_active_peer();
    if (!peer)
        return false;

    a2dp_sm = get_state_machine(peer->bd_addr);
    if (!a2dp_sm)
        return false;

    state = a2dp_state_machine_get_state(a2dp_sm);
    return (state == A2DP_STATE_OPENED);
}

bool a2dp_sink_stream_started(void)
{
    a2dp_state_machine_t* a2dp_sm;
    a2dp_state_t state;
    a2dp_peer_t* peer = get_active_peer();
    if (!peer)
        return false;

    a2dp_sm = get_state_machine(peer->bd_addr);
    if (!a2dp_sm)
        return false;

    state = a2dp_state_machine_get_state(a2dp_sm);
    return (state == A2DP_STATE_STARTED);
}

void a2dp_sink_codec_state_change(void)
{
    a2dp_peer_t* peer = get_active_peer();
    if (!peer)
        return;

    do_in_a2dp_snk_service(a2dp_event_new(DEVICE_CODEC_STATE_CHANGE_EVT, peer->bd_addr));
}

static bt_status_t a2dp_sink_init(void)
{
    g_a2dp_sink.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    return BT_STATUS_SUCCESS;
}

static void a2dp_sink_cleanup(void)
{
    g_a2dp_sink.active_peer = NULL;
    bt_callbacks_list_free(g_a2dp_sink.callbacks);
    g_a2dp_sink.callbacks = NULL;
}

static void sink_startup(void* data)
{
    profile_on_startup_t on_startup = (profile_on_startup_t)data;

    list_initialize(&g_a2dp_sink.list);
    if (bt_sal_a2dp_sink_init(A2DP_MAX_CONNECTION) != BT_STATUS_SUCCESS) {
        list_delete(&g_a2dp_sink.list);
        /* callback notify startup failed */
        on_startup(PROFILE_A2DP_SINK, false);
        return;
    }

    a2dp_audio_init(SVR_SINK, g_a2dp_sink.offloading);

    g_a2dp_sink.enabled = true;
    on_startup(PROFILE_A2DP_SINK, true);
}

static bt_status_t a2dp_sink_startup(profile_on_startup_t cb)
{
    if (g_a2dp_sink.enabled) {
        return BT_STATUS_NOT_ENABLED;
    }

    a2dp_event_t* evt = a2dp_event_new(A2DP_STARTUP, NULL);
    evt->event_data.cb = cb;
    do_in_a2dp_snk_service(evt);
    return BT_STATUS_SUCCESS;
}

static void sink_shutdown(void* data)
{
    a2dp_device_t* device;
    struct list_node* node;
    struct list_node* tmp;
    profile_on_shutdown_t on_shutdown = (profile_on_shutdown_t)data;

    g_a2dp_sink.enabled = false;
    a2dp_audio_cleanup(SVR_SINK);

    list_for_every_safe(&g_a2dp_sink.list, node, tmp)
    {
        device = (a2dp_device_t*)node;
        a2dp_device_delete(device);
    }
    list_delete(&g_a2dp_sink.list);
    bt_sal_a2dp_sink_cleanup();
    g_a2dp_sink.active_peer = NULL;
    on_shutdown(PROFILE_A2DP_SINK, true);
}

static bt_status_t a2dp_sink_shutdown(profile_on_shutdown_t cb)
{
    if (!g_a2dp_sink.enabled) {
        return BT_STATUS_SUCCESS;
    }

    a2dp_event_t* evt = a2dp_event_new(A2DP_SHUTDOWN, NULL);
    evt->event_data.cb = cb;
    do_in_a2dp_snk_service(evt);

    return BT_STATUS_SUCCESS;
}

static void a2dp_sink_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_A2DP_OFFLOADING:
        g_a2dp_sink.offloading = msg->data.valuebool;
        break;
    case PROFILE_EVT_REMOTE_DETACH: {
        bt_instance_t* ins = msg->data.data;

        if (ins->a2dp_sink_cookie) {
            BT_LOGD("%s PROFILE_EVT_REMOTE_DETACH", __func__);
            a2dp_sink_unregister_callbacks(NULL, ins->a2dp_sink_cookie);
            ins->a2dp_sink_cookie = NULL;
        }
        break;
    }
    default:
        break;
    }
}

static void* a2dp_sink_register_callbacks(void* remote, const a2dp_sink_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_a2dp_sink.callbacks, remote, (void*)callbacks);
}

static bool a2dp_sink_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_a2dp_sink.callbacks, remote, cookie);
}

static bool a2dp_sink_is_connected(bt_address_t* addr)
{
    if (!g_a2dp_sink.enabled) {
        return false;
    }

    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_sink.list, addr);
    if (!device) {
        return false;
    }

    profile_connection_state_t state = a2dp_state_machine_get_connection_state(device->a2dp_sm);

    return state == PROFILE_STATE_CONNECTED;
}

static bool a2dp_sink_is_playing(bt_address_t* addr)
{
    if (!g_a2dp_sink.enabled) {
        return false;
    }

    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_sink.list, addr);
    if (!device) {
        return false;
    }

    a2dp_state_t state = a2dp_state_machine_get_state(device->a2dp_sm);

    return state == A2DP_STATE_STARTED;
}

static profile_connection_state_t a2dp_sink_get_connection_state(bt_address_t* addr)
{
    if (!g_a2dp_sink.enabled) {
        return PROFILE_STATE_DISCONNECTED;
    }

    a2dp_device_t* device = find_a2dp_device_by_addr(&g_a2dp_sink.list, addr);
    if (!device) {
        return PROFILE_STATE_DISCONNECTED;
    }

    profile_connection_state_t state = a2dp_state_machine_get_connection_state(device->a2dp_sm);

    return state;
}

static bt_status_t a2dp_sink_connect(bt_address_t* addr)
{
    if (!g_a2dp_sink.enabled) {
        return BT_STATUS_FAIL;
    }

    do_in_a2dp_snk_service(a2dp_event_new(CONNECT_REQ, addr));

    return BT_STATUS_SUCCESS;
}

static bt_status_t a2dp_sink_disconnect(bt_address_t* addr)
{
    if (!g_a2dp_sink.enabled) {
        return BT_STATUS_FAIL;
    }

    do_in_a2dp_snk_service(a2dp_event_new(DISCONNECT_REQ, addr));

    return BT_STATUS_SUCCESS;
}

static bt_status_t a2dp_sink_set_active_device(bt_address_t* addr)
{
    return BT_STATUS_SUCCESS;
}

static int a2dp_sink_get_state(void)
{
    return 1;
}

static const a2dp_sink_interface_t a2dp_sinkInterface = {
    .size = sizeof(a2dp_sinkInterface),
    .register_callbacks = a2dp_sink_register_callbacks,
    .unregister_callbacks = a2dp_sink_unregister_callbacks,
    .is_connected = a2dp_sink_is_connected,
    .is_playing = a2dp_sink_is_playing,
    .get_connection_state = a2dp_sink_get_connection_state,
    .connect = a2dp_sink_connect,
    .disconnect = a2dp_sink_disconnect,
    .set_active_device = a2dp_sink_set_active_device,
};

static const void* get_a2dp_sink_profile_interface(void)
{
    return (void*)&a2dp_sinkInterface;
}

static int a2dp_sink_dump(void)
{
    return 0;
}

static const profile_service_t a2dp_sink_service = {
    .auto_start = true,
    .name = PROFILE_A2DP_SINK_NAME,
    .id = PROFILE_A2DP_SINK,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = a2dp_sink_init,
    .startup = a2dp_sink_startup,
    .shutdown = a2dp_sink_shutdown,
    .process_msg = a2dp_sink_process_msg,
    .get_state = a2dp_sink_get_state,
    .get_profile_interface = get_a2dp_sink_profile_interface,
    .cleanup = a2dp_sink_cleanup,
    .dump = a2dp_sink_dump,
};

void bt_sal_a2dp_sink_event_callback(a2dp_event_t* event)
{
    do_in_a2dp_snk_service(event);
}

void a2dp_sink_service_notify_connection_state_changed(
    bt_address_t* addr, profile_connection_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    A2DP_SINK_CALLBACK_FOREACH(g_a2dp_sink.callbacks, connection_state_cb, addr, state);
}

void a2dp_sink_service_notify_audio_state_changed(
    bt_address_t* addr, a2dp_audio_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    A2DP_SINK_CALLBACK_FOREACH(g_a2dp_sink.callbacks, audio_state_cb, addr, state);
}

void a2dp_sink_service_notify_audio_sink_config_changed(
    bt_address_t* addr)
{
    BT_LOGD("%s", __FUNCTION__);
    A2DP_SINK_CALLBACK_FOREACH(g_a2dp_sink.callbacks, audio_sink_config_cb, addr);
}

void register_a2dp_sink_service(void)
{
    register_service(&a2dp_sink_service);
}
