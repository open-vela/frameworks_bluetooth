/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#define LOG_TAG "lea_server"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "bt_lea_server.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_audio_sink.h"
#include "lea_audio_source.h"
#include "lea_server_service.h"
#include "lea_server_state_machine.h"
#include "sal_lea_server_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHECK_ENABLED()                    \
    {                                      \
        if (!g_lea_server_service.started) \
            return BT_STATUS_NOT_ENABLED;  \
    }

#define LEAS_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_server_callbacks_t, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct
{
    bool started;
    uint8_t max_connections;
    bt_list_t *leas_devices;
    bt_list_t *leas_stream;
    callbacks_list_t *callbacks;
    pthread_mutex_t device_lock;
    pthread_mutex_t stream_lock;
} lea_server_service_t;

typedef struct
{
    bt_address_t addr;
    lea_server_state_machine_t *leasm;
    profile_connection_state_t state;
} lea_server_device_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static lea_server_state_machine_t *get_state_machine(bt_address_t *addr);
static bt_status_t lea_server_send_message(lea_server_msg_t *msg);

static void on_lea_audio_suspend(uint32_t stream_id);
static void on_lea_audio_resume(uint32_t stream_id);
static void on_lea_meatadata_updated(uint32_t stream_id);
static void on_lea_audio_send(uint32_t stream_id, uint8_t *buffer, uint16_t length);

static void *lea_server_register_callbacks(void *remote, const lea_server_callbacks_t *callbacks);
static bool lea_server_unregister_callbacks(void **remote, void *cookie);
static profile_connection_state_t lea_server_get_connection_state(bt_address_t *addr);
static bt_status_t lea_server_start_announce(int8_t adv_id, uint8_t announce_type,
                                             uint8_t *adv_data, uint16_t adv_size,
                                             uint8_t *md_data, uint16_t md_size);
static bt_status_t lea_server_stop_announce(int8_t adv_id);
static bt_status_t lea_server_disconnect(bt_address_t *addr);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static lea_server_service_t g_lea_server_service = {
    .started = false,
    .leas_devices = NULL,
    .leas_stream = NULL,
    .callbacks = NULL,
};

static lea_sink_callabcks_t lea_sink_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_meatadata_updated,
    .lea_audio_resume_cb = on_lea_audio_resume,
    .lea_audio_suspend_cb = on_lea_audio_suspend,
};

static lea_source_callabcks_t lea_source_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_meatadata_updated,
    .lea_audio_resume_cb = on_lea_audio_resume,
    .lea_audio_suspend_cb = on_lea_audio_suspend,
    .lea_audio_send_cb = on_lea_audio_send,
};

static const lea_server_interface_t LEAServerInterface = {
    sizeof(LEAServerInterface),
    .register_callbacks = lea_server_register_callbacks,
    .unregister_callbacks = lea_server_unregister_callbacks,
    .start_announce = lea_server_start_announce,
    .stop_announce = lea_server_stop_announce,
    .get_connection_state = lea_server_get_connection_state,
    .disconnect = lea_server_disconnect,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool lea_server_device_cmp(void *device, void *addr)
{
    return bt_addr_compare(&((lea_server_device_t *)device)->addr, addr) == 0;
}

static lea_server_device_t *find_lea_server_device_by_addr(bt_address_t *addr)
{
    return bt_list_find(g_lea_server_service.leas_devices,
                        lea_server_device_cmp, addr);
}

static lea_server_device_t *lea_server_device_new(bt_address_t *addr,
                                                  lea_server_state_machine_t *leasm)
{
    lea_server_device_t *device = malloc(sizeof(lea_server_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->leasm = leasm;

    return device;
}

static void lea_server_device_delete(lea_server_device_t *device)
{
    if (!device)
        return;

    lea_server_msg_t *msg = lea_server_msg_new(DISCONNECT, &device->addr);
    if (msg == NULL)
        return;

    lea_server_state_machine_dispatch(device->leasm, msg);
    lea_server_msg_destory(msg);
    lea_server_state_machine_destory(device->leasm);
    free(device);
}

static lea_server_state_machine_t *get_state_machine(bt_address_t *addr)
{
    lea_server_state_machine_t *leasm;
    lea_server_device_t *device;

    if (!g_lea_server_service.started)
        return NULL;

    device = find_lea_server_device_by_addr(addr);
    if (device)
        return device->leasm;

    leasm = lea_server_state_machine_new(addr, (void *)&g_lea_server_service);
    if (!leasm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    device = lea_server_device_new(addr, leasm);
    if (!device) {
        BT_LOGE("New device alloc failed");
        lea_server_state_machine_destory(leasm);
        return NULL;
    }

    bt_list_add_tail(g_lea_server_service.leas_devices, device);

    return leasm;
}

static void lea_server_do_shutdown(void)
{
    if (!g_lea_server_service.started)
        return;

    pthread_mutex_lock(&g_lea_server_service.device_lock);
    g_lea_server_service.started = false;
    bt_list_free(g_lea_server_service.leas_devices);
    bt_list_free(g_lea_server_service.leas_stream);
    g_lea_server_service.leas_devices = NULL;
    g_lea_server_service.leas_stream = NULL;
    pthread_mutex_unlock(&g_lea_server_service.device_lock);
    pthread_mutex_destroy(&g_lea_server_service.device_lock);
    bt_callbacks_list_free(g_lea_server_service.callbacks);
    g_lea_server_service.callbacks = NULL;
    lea_audio_sink_cleanup();
    lea_audio_source_cleanup();
    bt_sal_lea_server_cleanup();
}

static void lea_server_process_message(void *data)
{
    lea_server_msg_t *msg = (lea_server_msg_t *)data;

    switch (msg->event) {
    case SHUTDOWN:
        lea_server_do_shutdown();
        break;
    default: {
        pthread_mutex_lock(&g_lea_server_service.device_lock);
        lea_server_state_machine_t *leasm = get_state_machine(&msg->data.addr);
        if (leasm)
            lea_server_state_machine_dispatch(leasm, msg);
        pthread_mutex_unlock(&g_lea_server_service.device_lock);
        break;
    }
    }

    lea_server_msg_destory(msg);
}

static bt_status_t lea_server_send_message(lea_server_msg_t *msg)
{
    assert(msg);

    do_in_service_loop(lea_server_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_server_send_event(bt_address_t *addr, lea_server_event_t evt)
{
    lea_server_msg_t *msg = lea_server_msg_new(evt, addr);

    if (!msg)
        return BT_STATUS_NOMEM;

    return lea_server_send_message(msg);
}

static void on_lea_audio_suspend(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGW("%s, failed stream id(0x%08x) invalid", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_STREAM_SUSPEND, &stream->addr);
    if (!msg)
        return;

    BT_LOGD("%s, Stream ID:0x%08x", __func__, stream_id);
    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

static void on_lea_audio_resume(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGW("%s, failed stream id(0x%08x) invalid", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_STREAM_RESUME, &stream->addr);
    if (!msg)
        return;

    BT_LOGD("%s, Stream ID:0x%08x", __func__, stream_id);
    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

static void on_lea_meatadata_updated(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGW("%s, failed stream id(0x%08x) invalid", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_METADATA_UPDATED, &stream->addr);
    if (!msg)
        return;

    BT_LOGD("%s, Stream ID:0x%08x", __func__, stream_id);
    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

static void on_lea_audio_send(uint32_t stream_id, uint8_t *buffer, uint16_t length)
{
    lea_audio_stream_t *stream;
    lea_send_iso_data_t *iso_pkt;
    int size;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        return;
    }

    iso_pkt = bt_sal_lea_alloc_send_buffer(stream->sdu_size, stream->iso_handle);
    iso_pkt->sdu_length = stream->sdu_size;

    size = lea_audio_source_read(stream->stream_id, iso_pkt->sdu, iso_pkt->sdu_length);
    if (size != iso_pkt->sdu_length) {
        BT_LOGE("%s, size:%d frame_size:%d", __func__, size, iso_pkt->sdu_length);
        return;
    }

    bt_sal_lea_send_iso_data(iso_pkt);
}

static bt_status_t lea_server_init(void)
{
    bt_status_t ret;

    BT_LOGD("%s", __func__);
    ret = lea_audio_sink_init(&lea_sink_callbacks);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    ret = lea_audio_source_init(&lea_source_callbacks);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    return BT_STATUS_SUCCESS;
}

static void lea_server_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static bt_status_t lea_server_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    pthread_mutexattr_t attr;
    lea_server_service_t *service = &g_lea_server_service;

    BT_LOGD("%s", __func__);
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->leas_devices = bt_list_new((bt_list_free_cb_t)
                                            lea_server_device_delete);
    service->leas_stream = bt_list_new(NULL);
    service->callbacks = bt_callbacks_list_new(2);
    if (!service->leas_devices || !service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->device_lock, &attr);
    pthread_mutex_init(&service->stream_lock, &attr);

    status = bt_sal_lea_server_init();
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    service->started = true;

    return BT_STATUS_SUCCESS;

fail:
    bt_list_free(service->leas_devices);
    bt_callbacks_list_free(service->callbacks);
    pthread_mutex_destroy(&service->device_lock);
    pthread_mutex_destroy(&service->stream_lock);
    return status;
}

static bt_status_t lea_server_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);

    return lea_server_send_event(NULL, SHUTDOWN);
}

static void *lea_server_register_callbacks(void *remote, const lea_server_callbacks_t *callbacks)
{
    if (!g_lea_server_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_lea_server_service.callbacks,
                                        remote, (void *)callbacks);
}

static bool lea_server_unregister_callbacks(void **remote, void *cookie)
{
    if (!g_lea_server_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_lea_server_service.callbacks,
                                          remote, cookie);
}

static profile_connection_state_t lea_server_get_connection_state(bt_address_t *addr)
{
    lea_server_device_t *device = find_lea_server_device_by_addr(addr);
    profile_connection_state_t conn_state;

    if (!device)
        return PROFILE_STATE_DISCONNECTED;

    pthread_mutex_lock(&g_lea_server_service.device_lock);
    conn_state = device->state;
    pthread_mutex_unlock(&g_lea_server_service.device_lock);

    return conn_state;
}

static bt_status_t lea_server_start_announce(int8_t adv_id, uint8_t announce_type,
                                             uint8_t *adv_data, uint16_t adv_size,
                                             uint8_t *md_data, uint16_t md_size)
{
    return bt_sal_lea_server_start_announce(adv_id, announce_type, adv_data,
                                            adv_size, md_data, md_size);
}

static bt_status_t lea_server_stop_announce(int8_t adv_id)
{
    return bt_sal_lea_server_stop_announce(adv_id);
}

static bt_status_t lea_server_disconnect(bt_address_t *addr)
{
    CHECK_ENABLED();
    profile_connection_state_t state = lea_server_get_connection_state(addr);
    if (state == PROFILE_STATE_DISCONNECTED || state == PROFILE_STATE_DISCONNECTING)
        return BT_STATUS_FAIL;

    return bt_sal_lea_server_disconnect(addr);
}

static const void *get_leas_profile_interface(void)
{
    return &LEAServerInterface;
}

static int lea_server_dump(void)
{
    printf("impl hfp hf dump");
    return 0;
}

static bool lea_server_stream_cmp(void *audio_stream, void *stream_id)
{
    return ((lea_audio_stream_t *)audio_stream)->stream_id == *((uint32_t *)stream_id);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lea_audio_stream_t *lea_server_add_stream(
    uint32_t stream_id, bt_address_t *remote_addr)
{
    lea_audio_stream_t *audio_stream;

    audio_stream = malloc(sizeof(lea_audio_stream_t));
    if (!audio_stream) {
        BT_LOGE("error, malloc %s", __func__);
        return NULL;
    }

    audio_stream->stream_id = stream_id;
    audio_stream->is_source = bt_sal_lea_is_source_stream(stream_id);
    memcpy(&audio_stream->addr, remote_addr, sizeof(bt_address_t));
    pthread_mutex_lock(&g_lea_server_service.stream_lock);
    bt_list_add_tail(g_lea_server_service.leas_stream, audio_stream);
    pthread_mutex_unlock(&g_lea_server_service.stream_lock);

    return audio_stream;
}

lea_audio_stream_t *lea_server_find_stream(uint32_t stream_id)
{
    lea_audio_stream_t *stream;

    pthread_mutex_lock(&g_lea_server_service.stream_lock);
    stream = bt_list_find(g_lea_server_service.leas_stream, lea_server_stream_cmp, &stream_id);
    pthread_mutex_unlock(&g_lea_server_service.stream_lock);

    return stream;
}

lea_audio_stream_t *lea_server_find_update_stream(lea_audio_stream_t *stream)
{
    lea_audio_stream_t *local_stream = NULL;

    pthread_mutex_lock(&g_lea_server_service.stream_lock);
    local_stream = bt_list_find(g_lea_server_service.leas_stream, lea_server_stream_cmp, &stream->stream_id);
    if (local_stream) {
        memcpy(local_stream, stream, sizeof(lea_audio_stream_t));
    }
    pthread_mutex_unlock(&g_lea_server_service.stream_lock);

    return local_stream;
}

void lea_server_remove_stream(uint32_t stream_id)
{
    lea_audio_stream_t *audio_stream;

    pthread_mutex_lock(&g_lea_server_service.stream_lock);
    audio_stream = bt_list_find(g_lea_server_service.leas_stream, lea_server_stream_cmp, &stream_id);
    bt_list_remove(g_lea_server_service.leas_stream, audio_stream);
    pthread_mutex_unlock(&g_lea_server_service.stream_lock);
}

void lea_server_remove_streams()
{
    pthread_mutex_lock(&g_lea_server_service.stream_lock);
    bt_list_clear(g_lea_server_service.leas_stream);
    pthread_mutex_unlock(&g_lea_server_service.stream_lock);
}

void lea_server_notify_stack_state_changed(lea_server_stack_state_t
                                               enabled)
{
    BT_LOGD("%s", __func__);
    LEAS_CALLBACK_FOREACH(g_lea_server_service.callbacks,
                          server_stack_state_cb, enabled);
}

void lea_server_notify_connection_state_changed(bt_address_t *addr,
                                                profile_connection_state_t state)
{
    BT_LOGD("%s", __func__);
    LEAS_CALLBACK_FOREACH(g_lea_server_service.callbacks,
                          server_connection_state_cb, state, addr);
}

void lea_server_on_stack_state_changed(lea_server_stack_state_t enabled)
{
    lea_server_msg_t *msg = lea_server_msg_new(STACK_EVENT_STACK_STATE,
                                               NULL);
    if (!msg)
        return;

    msg->data.valueint1 = enabled;
    lea_server_send_message(msg);
}

void lea_server_on_connection_state_changed(bt_address_t *addr,
                                            profile_connection_state_t state)
{
    lea_server_msg_t *msg = lea_server_msg_new(STACK_EVENT_CONNECTION_STATE,
                                               addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    lea_server_send_message(msg);
}

void lea_server_on_storage_changed(void *value, uint32_t size)
{
    lea_server_msg_t *msg = lea_server_msg_new_ext(STACK_EVENT_STORAGE, NULL,
                                                   size);
    if (!msg)
        return;

    memcpy(msg->data.dataarry, value, size);
    lea_server_send_message(msg);
}

void lea_server_on_stream_added(bt_address_t *addr, uint32_t stream_id)
{
    lea_server_msg_t *msg = lea_server_msg_new(STACK_EVENT_STREAM_ADDED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_removed(bt_address_t *addr, uint32_t stream_id)
{
    lea_server_msg_t *msg = lea_server_msg_new(STACK_EVENT_STREAM_REMOVED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_started(lea_audio_stream_t *audio)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(audio->stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:%d", __func__, audio->stream_id);
        return;
    }

    msg = lea_server_msg_new_ext(STACK_EVENT_STREAM_STARTED,
                                 &stream->addr, sizeof(lea_audio_stream_t));
    if (!msg)
        return;

    memcpy(msg->data.dataarry, audio, sizeof(lea_audio_stream_t));
    lea_server_send_message(msg);
}

void lea_server_on_stream_stopped(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:%d", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_STREAM_STOPPED, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_suspend(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:%d", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_STREAM_SUSPEND, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_resume(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:%d", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_STREAM_RESUME, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_metedata_updated(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:%d", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_METADATA_UPDATED, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_recv(uint32_t stream_id, uint32_t time_stamp,
                               uint16_t seq_number, uint8_t *sdu, uint16_t size)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;
    lea_recv_iso_data_t *packet;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:%d", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_STREAN_RECV, &stream->addr);
    if (!msg)
        return;

    packet = lea_audio_sink_packet_alloc(time_stamp, seq_number, sdu, size);
    if (!packet)
        return;

    msg->data.valueint1 = stream_id;
    msg->data.datapointer = packet;
    lea_server_send_message(msg);
}

void lea_server_on_stream_send(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_server_msg_t *msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:%d", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_STREAN_SENT, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_ascs_event(bt_address_t *addr, lea_adpt_ase_state_t evt, void *data)
{
    lea_server_event_t event;

    switch (evt) {
    case ADPT_LEA_ASE_STATE_IDLE: {
        event = STACK_EVENT_ASE_IDLE;
    } break;
    case ADPT_LEA_ASE_STATE_CODEC_CONFIG: {
        event = STACK_EVENT_ASE_CODEC_CONFIG;
    } break;
    case ADPT_LEA_ASE_STATE_QOS_CONFIG: {
        event = STACK_EVENT_ASE_QOS_CONFIG;
    } break;
    case ADPT_LEA_ASE_STATE_ENABLING: {
        event = STACK_EVENT_ASE_ENABLING;
    } break;
    case ADPT_LEA_ASE_STATE_STREAMING: {
        event = STACK_EVENT_ASE_STREAMING;
    } break;
    case ADPT_LEA_ASE_STATE_DISABLING: {
        event = STACK_EVENT_ASE_DISABLING;
    } break;
    case ADPT_LEA_ASE_STATE_RELEASING: {
        event = STACK_EVENT_ASE_RELEASING;
    } break;
    default: {
        BT_LOGE("%s, unexpect event:%d", __func__, evt);
        return;
    };
    }

    lea_server_send_event(addr, event);
}

static const profile_service_t lea_server_service = {
    .auto_start = true,
    .name = "lea_server",
    .id = PROFILE_LEAUDIO_SERVER,
    .transport = BT_TRANSPORT_BLE,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = lea_server_init,
    .startup = lea_server_startup,
    .shutdown = lea_server_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_leas_profile_interface,
    .cleanup = lea_server_cleanup,
    .dump = lea_server_dump,
};

void register_lea_server_service(void)
{
    register_service(&lea_server_service);
}