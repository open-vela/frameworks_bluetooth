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
#include "bt_vendor.h"
#include "callbacks_list.h"
#include "lea_audio_sink.h"
#include "lea_audio_source.h"
#include "lea_codec.h"
#include "lea_server_service.h"
#include "lea_server_state_machine.h"
#include "sal_lea_common.h"
#include "sal_lea_server_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LEAS_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_server_callbacks_t, _cback, ##__VA_ARGS__)

#define LEAS_CONTEXT_TYPE_ALL (ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL | ADPT_LEA_CONTEXT_TYPE_MEDIA | ADPT_LEA_CONTEXT_TYPE_GAME | ADPT_LEA_CONTEXT_TYPE_INSTRUCTIONAL | ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | ADPT_LEA_CONTEXT_TYPE_LIVE | ADPT_LEA_CONTEXT_TYPE_SOUND_EFFECTS | ADPT_LEA_CONTEXT_TYPE_NOTIFICATIONS | ADPT_LEA_CONTEXT_TYPE_RINGTONE | ADPT_LEA_CONTEXT_TYPE_ALERTS | ADPT_LEA_CONTEXT_TYPE_EMERGENCY_ALARM)

#ifndef CONFIG_LEAS_CALL_SINK_SUPPORTED_SAMPLE_FREQUENCY
#define CONFIG_LEAS_CALL_SINK_SUPPORTED_SAMPLE_FREQUENCY (ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_8000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_16000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_24000)
#endif

#ifndef CONFIG_LEAS_CALL_SOURCE_SUPPORTED_SAMPLE_FREQUENCY
#define CONFIG_LEAS_CALL_SOURCE_SUPPORTED_SAMPLE_FREQUENCY (ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_8000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_16000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_24000)
#endif

#ifndef CONFIG_LEAS_MEDIA_SINK_SUPPORTED_SAMPLE_FREQUENCY
#define CONFIG_LEAS_MEDIA_SINK_SUPPORTED_SAMPLE_FREQUENCY (ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_16000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_32000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_48000)
#endif

#ifndef CONFIG_LEAS_CALL_SINK_METADATA_PREFER_CONTEX
#define CONFIG_LEAS_CALL_SINK_METADATA_PREFER_CONTEX (ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL | ADPT_LEA_CONTEXT_TYPE_INSTRUCTIONAL | ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | ADPT_LEA_CONTEXT_TYPE_SOUND_EFFECTS | ADPT_LEA_CONTEXT_TYPE_NOTIFICATIONS | ADPT_LEA_CONTEXT_TYPE_RINGTONE | ADPT_LEA_CONTEXT_TYPE_ALERTS | ADPT_LEA_CONTEXT_TYPE_EMERGENCY_ALARM)
#endif

#ifndef CONFIG_LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX
#define CONFIG_LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX (ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL | ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | ADPT_LEA_CONTEXT_TYPE_LIVE)
#endif

#ifndef CONFIG_LEAS_MEDIA_SINK_METADATA_PREFER_CONTEX
#define CONFIG_LEAS_MEDIA_SINK_METADATA_PREFER_CONTEX (ADPT_LEA_CONTEXT_TYPE_MEDIA | ADPT_LEA_CONTEXT_TYPE_GAME | ADPT_LEA_CONTEXT_TYPE_LIVE)
#endif

#ifndef CONFIG_LEAS_PACS_FRAME_DURATION
#define CONFIG_LEAS_PACS_FRAME_DURATION (ADPT_LEA_SUPPORTED_FRAME_DURATION_10 | ADPT_LEA_PREFERRED_FRAME_DURATION_10)
#endif

#define CHECK_ENABLED()                    \
    {                                      \
        if (!g_lea_server_service.started) \
            return BT_STATUS_NOT_ENABLED;  \
    }

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct
{
    bool started;
    bool offloading;
    uint8_t max_connections;
    uint32_t sink_location;
    uint32_t source_location;
    bt_list_t* leas_devices;
    bt_list_t* leas_stream;
    callbacks_list_t* callbacks;
    pthread_mutex_t device_lock;
    pthread_mutex_t stream_lock;
} lea_server_service_t;

typedef struct {
    uint8_t ase_id;
    uint8_t ase_state;
    uint16_t type;
} lea_server_endpoint_t;

typedef struct
{
    bt_address_t addr;

    uint8_t ase_number;
    lea_server_endpoint_t ase[2]; // CONFIG_BLUETOOTH_LEAUDIO_SERVER_SINK_ASE_NUMBER + CONFIG_BLUETOOTH_LEAUDIO_SERVER_SOURCE_ASE_NUMBER
    lea_server_state_machine_t* leasm;
    profile_connection_state_t state;
} lea_server_device_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static lea_server_state_machine_t* get_state_machine(bt_address_t* addr);

static void on_lea_sink_audio_suspend();
static void on_lea_sink_audio_resume();
static void on_lea_sink_meatadata_updated();

static void on_lea_source_audio_suspend();
static void on_lea_source_audio_resume();
static void on_lea_source_meatadata_updated();
static void on_lea_source_audio_send(uint8_t* buffer, uint16_t length);

static void* lea_server_register_callbacks(void* remote, const lea_server_callbacks_t* callbacks);
static bool lea_server_unregister_callbacks(void** remote, void* cookie);
static profile_connection_state_t lea_server_get_connection_state(bt_address_t* addr);
static bt_status_t lea_server_start_announce(int8_t adv_id, uint8_t announce_type,
    uint8_t* adv_data, uint16_t adv_size,
    uint8_t* md_data, uint16_t md_size);
static bt_status_t lea_server_stop_announce(int8_t adv_id);
static bt_status_t lea_server_disconnect_device(bt_address_t* addr);
static bt_status_t lea_server_disconnect_audio(bt_address_t* addr);
static bool lea_server_streams_are_started(bt_address_t* addr);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
bt_status_t lea_server_send_message(lea_server_msg_t* msg);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static lea_server_service_t g_lea_server_service = {
    .started = false,
    .leas_devices = NULL,
    .leas_stream = NULL,
    .callbacks = NULL,
};

static lea_sink_callabcks_t g_lea_sink_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_sink_meatadata_updated,
    .lea_audio_resume_cb = on_lea_sink_audio_resume,
    .lea_audio_suspend_cb = on_lea_sink_audio_suspend,
};

static lea_source_callabcks_t g_lea_source_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_source_meatadata_updated,
    .lea_audio_resume_cb = on_lea_source_audio_resume,
    .lea_audio_suspend_cb = on_lea_source_audio_suspend,
    .lea_audio_send_cb = on_lea_source_audio_send,
};

static const lea_server_interface_t LEAServerInterface = {
    sizeof(LEAServerInterface),
    .register_callbacks = lea_server_register_callbacks,
    .unregister_callbacks = lea_server_unregister_callbacks,
    .start_announce = lea_server_start_announce,
    .stop_announce = lea_server_stop_announce,
    .get_connection_state = lea_server_get_connection_state,
    .disconnect = lea_server_disconnect_device,
    .disconnect_audio = lea_server_disconnect_audio,
};

static lea_metadata_t g_metadata_info[] = {
    { .type = ADPT_LEA_METADATA_PREFERRED_AUDIO_CONTEXTS,
        .preferred_contexts = CONFIG_LEAS_CALL_SINK_METADATA_PREFER_CONTEX },
    { .type = ADPT_LEA_METADATA_PREFERRED_AUDIO_CONTEXTS,
        .preferred_contexts = CONFIG_LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX },
    { .type = ADPT_LEA_METADATA_PREFERRED_AUDIO_CONTEXTS,
        .preferred_contexts = CONFIG_LEAS_MEDIA_SINK_METADATA_PREFER_CONTEX }
};

static lea_pac_info_t g_pacs_info[] = {
    { .pac_type = ADPT_LEA_PAC_TYPE_SINK_PAC, .pac_id = 1, .codec_id.format = ADPT_LEA_FORMAT_LC3, .codec_pac = {
                                                                                                       .mask = 0x1F,
                                                                                                       .frequencies = CONFIG_LEAS_CALL_SINK_SUPPORTED_SAMPLE_FREQUENCY,
                                                                                                       .durations = CONFIG_LEAS_PACS_FRAME_DURATION,
                                                                                                       .channels = ADPT_LEA_SUPPORTED_CHANNEL_COUNT_1,
                                                                                                       .frame_octets_min = 26,
                                                                                                       .frame_octets_max = 80,
                                                                                                       .max_frames = 1,
                                                                                                   },
        .md_number = sizeof(g_metadata_info[0]) / sizeof(lea_metadata_t),
        .md_value = &g_metadata_info[0] },
    { .pac_type = ADPT_LEA_PAC_TYPE_SOURCE_PAC, .pac_id = 2, .codec_id.format = ADPT_LEA_FORMAT_LC3, .codec_pac = {
                                                                                                         .mask = 0x1F,
                                                                                                         .frequencies = CONFIG_LEAS_CALL_SOURCE_SUPPORTED_SAMPLE_FREQUENCY,
                                                                                                         .durations = CONFIG_LEAS_PACS_FRAME_DURATION,
                                                                                                         .channels = ADPT_LEA_SUPPORTED_CHANNEL_COUNT_1,
                                                                                                         .frame_octets_min = 26,
                                                                                                         .frame_octets_max = 80,
                                                                                                         .max_frames = 1,
                                                                                                     },
        .md_number = sizeof(g_metadata_info[1]) / sizeof(lea_metadata_t),
        .md_value = &g_metadata_info[1] },
    { .pac_type = ADPT_LEA_PAC_TYPE_SINK_PAC, .pac_id = 3, .codec_id.format = ADPT_LEA_FORMAT_LC3, .codec_pac = {
                                                                                                       .mask = 0x1F,
                                                                                                       .frequencies = CONFIG_LEAS_MEDIA_SINK_SUPPORTED_SAMPLE_FREQUENCY,
                                                                                                       .durations = CONFIG_LEAS_PACS_FRAME_DURATION,
                                                                                                       .channels = ADPT_LEA_SUPPORTED_CHANNEL_COUNT_1,
                                                                                                       .frame_octets_min = 60,
                                                                                                       .frame_octets_max = 155,
                                                                                                       .max_frames = 1,
                                                                                                   },
        .md_number = sizeof(g_metadata_info[2]) / sizeof(lea_metadata_t),
        .md_value = &g_metadata_info[2] },
};

static lea_csis_info_t g_csis_info[] = {
    {
        .csis_id = ADPT_LEA_CSIS1_ID,
        .set_size = CONFIG_BLUETOOTH_LEAUDIO_SERVER_CSIS_SIZE,
        .rank = CONFIG_BLUETOOTH_LEAUDIO_SERVER_CSIS_RANK,
        .sirk_type = ADPT_LEA_SIRK_TYPE_ENCRYPTED,
        .sirk = { 0xB8, 0x03, 0xEA, 0xC6, 0xAF, 0xBB, 0x65, 0xA2, 0x5A, 0x41, 0xF1, 0x53, 0x05, 0x68, 0x8E, 0x83 },
    },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool
lea_server_device_cmp(void* device, void* addr)
{
    return bt_addr_compare(&((lea_server_device_t*)device)->addr, addr) == 0;
}

static lea_server_device_t* find_lea_server_device_by_addr(bt_address_t* addr)
{
    lea_server_service_t* service = &g_lea_server_service;

    return bt_list_find(service->leas_devices, lea_server_device_cmp, addr);
}

static lea_server_device_t* lea_server_device_new(bt_address_t* addr,
    lea_server_state_machine_t* leasm)
{
    lea_server_device_t* device = calloc(1, sizeof(lea_server_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->leasm = leasm;

    return device;
}

static void lea_server_device_delete(lea_server_device_t* device)
{
    if (!device)
        return;

    lea_server_msg_t* msg = lea_server_msg_new(DISCONNECT, &device->addr);
    if (msg == NULL)
        return;

    lea_server_state_machine_dispatch(device->leasm, msg);
    lea_server_msg_destory(msg);
    lea_server_state_machine_destory(device->leasm);
    free(device);
}

static lea_server_state_machine_t* get_state_machine(bt_address_t* addr)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_server_state_machine_t* leasm;
    lea_server_device_t* device;

    if (!service->started)
        return NULL;

    device = find_lea_server_device_by_addr(addr);
    if (device)
        return device->leasm;

    leasm = lea_server_state_machine_new(addr, (void*)service);
    if (!leasm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    lea_server_state_machine_set_offloading(leasm, service->offloading);
    device = lea_server_device_new(addr, leasm);
    if (!device) {
        BT_LOGE("New device alloc failed");
        lea_server_state_machine_destory(leasm);
        return NULL;
    }

    bt_list_add_tail(service->leas_devices, device);

    return leasm;
}

static void lea_server_do_shutdown(void)
{
    lea_server_service_t* service = &g_lea_server_service;

    if (!service->started)
        return;

    pthread_mutex_lock(&service->device_lock);
    service->started = false;
    bt_list_free(service->leas_devices);
    bt_list_free(service->leas_stream);
    service->leas_devices = NULL;
    service->leas_stream = NULL;
    pthread_mutex_unlock(&service->device_lock);
    pthread_mutex_destroy(&service->device_lock);
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    lea_audio_sink_cleanup();
    lea_audio_source_cleanup();
    bt_sal_lea_cleanup();
}

static bool lea_server_message_prehandle(lea_server_state_machine_t* leasm,
    lea_server_msg_t* event)
{
    lea_server_service_t* service = &g_lea_server_service;

    switch (event->event) {
    case STACK_EVENT_STREAM_STARTED: {
        lea_audio_stream_t* audio_stream = (lea_audio_stream_t*)event->data.data;
        lea_offload_config_t offload = { 0 };
        uint8_t param[sizeof(lea_offload_config_t)];
        size_t size;
        bool ret;

        BT_LOGD("%s addr:%s, started:%d, stream_id:0x%08x", __func__, bt_addr_str(&audio_stream->addr),
            audio_stream->started, audio_stream->stream_id);
        memcpy(&audio_stream->addr, &event->data.addr, sizeof(bt_address_t));
        audio_stream->started = true;
        audio_stream = lea_server_update_stream(audio_stream);
        if (!audio_stream) {
            BT_LOGE("fail, %s audio_stream not exist", __func__);
            return false;
        }

        lea_codec_set_config(audio_stream);
        if (!service->offloading) {
            break;
        }

        ret = lea_server_streams_are_started(&event->data.addr);
        if (!ret) {
            BT_LOGW("device(%s) streams streamming not completed", bt_addr_str(&event->data.addr));
            return false;
        }

        lea_codec_get_offload_config(&offload);
        offload.initiator = false;
        ret = lea_offload_start_builder(&offload, param, &size);
        if (!ret) {
            BT_LOGE("failed,  lea_offload_start_builder failed");
            return false;
        }

        event->event = OFFLOAD_START_REQ;
        free(event->data.data);
        event->data.data = malloc(size);
        memcpy(event->data.data, param, size);
        event->data.size = size;
        break;
    }
    case STACK_EVENT_STREAM_STOPPED: {
        lea_offload_config_t offload = { 0 };
        lea_audio_stream_t* stream;
        uint8_t param[sizeof(lea_offload_config_t)];
        lea_server_msg_t* msg;
        size_t size;
        bool ret;

        if (!service->offloading) {
            break;
        }

        lea_codec_get_offload_config(&offload);
        ret = lea_offload_stop_builder(&offload, param, &size);
        if (!ret) {
            BT_LOGE("failed, lea_offload_stop_builder");
            break;
        }

        stream = lea_server_find_stream(event->data.valueint1);
        if (stream) {
            lea_codec_unset_config(stream->is_source);
        }

        msg = lea_server_msg_new_ext(OFFLOAD_STOP_REQ, &event->data.addr, param, size);
        if (!msg) {
            BT_LOGE("failed, %s lea_server_msg_new_ext", __func__);
            break;
        }
        lea_server_send_message(msg);
        break;
    }
    default:
        break;
    }

    return true;
}

static void lea_server_process_message(void* data)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_server_msg_t* msg = (lea_server_msg_t*)data;

    switch (msg->event) {
    case SHUTDOWN:
        lea_server_do_shutdown();
        break;
    case STACK_EVENT_STACK_STATE:
        lea_server_notify_stack_state_changed(msg->data.valueint1);
        break;
    default: {
        bool dispatch;

        pthread_mutex_lock(&service->device_lock);
        lea_server_state_machine_t* leasm = get_state_machine(&msg->data.addr);
        if (!leasm) {
            pthread_mutex_unlock(&service->device_lock);
            BT_LOGE("%s, event:%d drop, leasm null", __func__, msg->event);
            break;
        }

        dispatch = lea_server_message_prehandle(leasm, msg);
        if (!dispatch) {
            pthread_mutex_unlock(&service->device_lock);
            BT_LOGE("%s, event:%d not dispatch", __func__, msg->event);
            break;
        }

        lea_server_state_machine_dispatch(leasm, msg);
        pthread_mutex_unlock(&service->device_lock);
        break;
    }
    }

    lea_server_msg_destory(msg);
}

bt_status_t lea_server_send_message(lea_server_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(lea_server_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_server_send_event(bt_address_t* addr, lea_server_event_t evt)
{
    lea_server_msg_t* msg = lea_server_msg_new(evt, addr);

    if (!msg)
        return BT_STATUS_NOMEM;

    return lea_server_send_message(msg);
}

static void streams_send_message(bool is_source, lea_server_event_t event)
{
    lea_server_service_t* service = &g_lea_server_service;
    bt_list_t* list = service->leas_stream;
    lea_audio_stream_t* stream;
    bt_list_node_t* node;
    lea_server_msg_t* msg;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        stream = bt_list_node(node);
        if (stream->started && (stream->is_source == is_source)) {
            msg = lea_server_msg_new(event, &stream->addr);
            if (!msg)
                return;

            msg->data.valueint1 = stream->stream_id;
            lea_server_send_message(msg);
        }
    }
}

static void on_lea_sink_audio_suspend()
{
    // todo suspend
    BT_LOGD("%s", __func__);
}

static void on_lea_sink_audio_resume()
{
    // todo resume
    BT_LOGD("%s", __func__);
}

static void on_lea_sink_meatadata_updated()
{
    streams_send_message(false, STACK_EVENT_METADATA_UPDATED);
}

static void on_lea_source_audio_suspend()
{
    // todo suspend
    BT_LOGD("%s", __func__);
}

static void on_lea_source_audio_resume()
{
    // todo resume
    BT_LOGD("%s", __func__);
}

static void on_lea_source_meatadata_updated()
{
    streams_send_message(true, STACK_EVENT_METADATA_UPDATED);
}

static void lea_audio_send_data(lea_audio_stream_t* stream, uint8_t* buffer, uint16_t length)
{
    lea_send_iso_data_t* iso_pkt;

    iso_pkt = bt_sal_lea_alloc_send_buffer(stream->sdu_size, stream->iso_handle);
    memcpy(iso_pkt->sdu, buffer, length);
    iso_pkt->sdu_length = length;

    bt_sal_lea_send_iso_data(iso_pkt);
}

static void on_lea_source_audio_send(uint8_t* buffer, uint16_t length)
{
    lea_server_service_t* service = &g_lea_server_service;
    bt_list_t* list = service->leas_stream;
    lea_audio_stream_t* stream;
    bt_list_node_t* node;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        stream = bt_list_node(node);
        if (stream->started && stream->is_source) {
            lea_audio_send_data(stream, buffer, length);
        }
    }
}

static bt_status_t lea_server_init(void)
{
    lea_server_service_t* service = &g_lea_server_service;
    bt_status_t ret;

    BT_LOGD("%s", __func__);
    ret = lea_audio_sink_init(service->offloading);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    ret = lea_audio_source_init(service->offloading);
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
    lea_server_service_t* service = &g_lea_server_service;

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

#if defined(CONFIG_KVDB) && defined(__NuttX__)
    service->sink_location = property_get_int32("persist.bluetooth.lea.sinkloc", CONFIG_BLUETOOTH_LEAUDIO_SERVER_SINK_LOCATION);
    service->source_location = property_get_int32("persist.bluetooth.lea.srcloc", CONFIG_BLUETOOTH_LEAUDIO_SERVER_SOURCE_LOCATION);
#else
    service->sink_location = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SINK_LOCATION;
    service->source_location = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SOURCE_LOCATION;
#endif

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->device_lock, &attr);
    pthread_mutex_init(&service->stream_lock, &attr);

    status = bt_sal_lea_init();
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    service->started = true;

    return BT_STATUS_SUCCESS;

fail:
    bt_list_free(service->leas_devices);
    service->leas_devices = NULL;
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    pthread_mutex_destroy(&service->device_lock);
    pthread_mutex_destroy(&service->stream_lock);
    return status;
}

static bt_status_t lea_server_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);

    return lea_server_send_event(NULL, SHUTDOWN);
}

static void lea_server_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_LEA_OFFLOADING:
        g_lea_server_service.offloading = msg->data.valuebool;
        break;

    default:
        break;
    }
}

static void* lea_server_register_callbacks(void* remote, const lea_server_callbacks_t* callbacks)
{
    lea_server_service_t* service = &g_lea_server_service;

    if (!service->started)
        return NULL;

    return bt_remote_callbacks_register(service->callbacks, remote, (void*)callbacks);
}

static bool lea_server_unregister_callbacks(void** remote, void* cookie)
{
    lea_server_service_t* service = &g_lea_server_service;

    if (!service->started)
        return false;

    return bt_remote_callbacks_unregister(service->callbacks, remote, cookie);
}

static void lea_server_update_connection_state(bt_address_t* addr, profile_connection_state_t state)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_server_device_t* device;

    device = find_lea_server_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device(%s) not found", __func__, bt_addr_str(addr));
        return;
    }

    pthread_mutex_lock(&service->device_lock);
    device->state = state;
    pthread_mutex_unlock(&service->device_lock);
}

static profile_connection_state_t lea_server_get_connection_state(bt_address_t* addr)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_server_device_t* device;
    profile_connection_state_t conn_state;

    device = find_lea_server_device_by_addr(addr);
    if (!device)
        return PROFILE_STATE_DISCONNECTED;

    pthread_mutex_lock(&service->device_lock);
    conn_state = device->state;
    pthread_mutex_unlock(&service->device_lock);

    return conn_state;
}

static bt_status_t lea_server_start_announce(int8_t adv_id, uint8_t announce_type,
    uint8_t* adv_data, uint16_t adv_size,
    uint8_t* md_data, uint16_t md_size)
{
    return bt_sal_lea_server_start_announce(adv_id, announce_type, adv_data,
        adv_size, md_data, md_size);
}

static bt_status_t lea_server_stop_announce(int8_t adv_id)
{
    return bt_sal_lea_server_stop_announce(adv_id);
}

static bt_status_t lea_server_disconnect_device(bt_address_t* addr)
{
    profile_connection_state_t state;

    CHECK_ENABLED();
    state = lea_server_get_connection_state(addr);
    if (state == PROFILE_STATE_DISCONNECTED || state == PROFILE_STATE_DISCONNECTING)
        return BT_STATUS_FAIL;

    return bt_sal_lea_disconnect(addr);
}

static bt_status_t lea_server_disconnect_audio(bt_address_t* addr)
{
    lea_server_service_t* service = &g_lea_server_service;
    profile_connection_state_t state;
    lea_server_device_t* device;
    int index;
    lea_server_endpoint_t* ase;

    CHECK_ENABLED();
    state = lea_server_get_connection_state(addr);
    if (state == PROFILE_STATE_DISCONNECTED || state == PROFILE_STATE_DISCONNECTING)
        return BT_STATUS_FAIL;

    device = find_lea_server_device_by_addr(addr);
    if (!device) {
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    pthread_mutex_lock(&service->device_lock);
    for (index = 0; index < device->ase_number; index++) {
        ase = &device->ase[index];
        bt_sal_lea_server_request_disable(addr, ase->ase_id);
    }
    pthread_mutex_unlock(&service->device_lock);

    return BT_STATUS_SUCCESS;
}

static const void* get_leas_profile_interface(void)
{
    return &LEAServerInterface;
}

static int lea_server_dump(void)
{
    printf("impl hfp hf dump");
    return 0;
}

static bool lea_server_stream_cmp(void* audio_stream, void* stream_id)
{
    return ((lea_audio_stream_t*)audio_stream)->stream_id == *((uint32_t*)stream_id);
}

static void update_server_ase(lea_server_device_t* device, uint8_t id, uint8_t state, uint16_t type)
{
    static lea_server_service_t* service = &g_lea_server_service;
    int index;
    bool found = false;

    pthread_mutex_lock(&service->device_lock);
    for (index = 0; index < device->ase_number; index++) {
        if (device->ase[index].ase_id == id) {
            device->ase[index].ase_state = state;
            found = true;
            break;
        }
    }

    if (!found) {
        device->ase[device->ase_number].type = type;
        device->ase[device->ase_number].ase_id = id;
        device->ase_number++;
    }

    pthread_mutex_unlock(&service->device_lock);
}

static bool lea_server_streams_are_started(bt_address_t* addr)
{
    lea_server_service_t* service = &g_lea_server_service;
    bt_list_t* list = service->leas_stream;
    lea_audio_stream_t* stream;
    bt_list_node_t* node;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        stream = bt_list_node(node);
        if (bt_addr_compare(addr, &stream->addr) == 0) {
            if (!stream->started) {
                return false;
            }
        }
    }
    return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lea_audio_stream_t* lea_server_add_stream(
    uint32_t stream_id, bt_address_t* remote_addr)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_audio_stream_t* audio_stream;

    audio_stream = lea_server_find_stream(stream_id);
    if (audio_stream) {
        memcpy(&audio_stream->addr, remote_addr, sizeof(bt_address_t));
        return audio_stream;
    }

    audio_stream = calloc(1, sizeof(lea_audio_stream_t));
    if (!audio_stream) {
        BT_LOGE("error, malloc %s", __func__);
        return NULL;
    }

    audio_stream->stream_id = stream_id;
    audio_stream->is_source = bt_sal_lea_is_source_stream(stream_id);
    memcpy(&audio_stream->addr, remote_addr, sizeof(bt_address_t));
    pthread_mutex_lock(&service->stream_lock);
    bt_list_add_tail(service->leas_stream, audio_stream);
    pthread_mutex_unlock(&service->stream_lock);

    return audio_stream;
}

lea_audio_stream_t* lea_server_find_stream(uint32_t stream_id)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_audio_stream_t* stream;

    pthread_mutex_lock(&service->stream_lock);
    stream = bt_list_find(service->leas_stream, lea_server_stream_cmp, &stream_id);
    pthread_mutex_unlock(&service->stream_lock);

    return stream;
}

lea_audio_stream_t* lea_server_update_stream(lea_audio_stream_t* stream)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_audio_stream_t* local_stream = NULL;

    pthread_mutex_lock(&service->stream_lock);
    local_stream = bt_list_find(service->leas_stream, lea_server_stream_cmp, &stream->stream_id);
    if (!local_stream) {
        BT_LOGE("fail, %s addr:%s,  stream_id:0x%08x not exist", __func__,
            bt_addr_str(&stream->addr), stream->stream_id);
        pthread_mutex_unlock(&service->stream_lock);
        return NULL;
    }

    memcpy(local_stream, stream, sizeof(lea_audio_stream_t));
    pthread_mutex_unlock(&service->stream_lock);

    return local_stream;
}

void lea_server_remove_stream(uint32_t stream_id)
{
    lea_server_service_t* service = &g_lea_server_service;
    lea_audio_stream_t* audio_stream;

    pthread_mutex_lock(&service->stream_lock);
    audio_stream = bt_list_find(service->leas_stream, lea_server_stream_cmp, &stream_id);
    bt_list_remove(service->leas_stream, audio_stream);
    pthread_mutex_unlock(&service->stream_lock);
}

void lea_server_remove_streams()
{
    lea_server_service_t* service = &g_lea_server_service;

    pthread_mutex_lock(&service->stream_lock);
    bt_list_clear(service->leas_stream);
    pthread_mutex_unlock(&service->stream_lock);
}

void lea_server_notify_stack_state_changed(lea_server_stack_state_t
        enabled)
{
    lea_server_service_t* service = &g_lea_server_service;

    BT_LOGD("%s", __func__);
    LEAS_CALLBACK_FOREACH(service->callbacks,
        server_stack_state_cb, enabled);
}

void lea_server_notify_connection_state_changed(bt_address_t* addr,
    profile_connection_state_t state)
{
    lea_server_service_t* service = &g_lea_server_service;
    BT_LOGD("%s", __func__);

    lea_server_update_connection_state(addr, state);
    LEAS_CALLBACK_FOREACH(service->callbacks,
        server_connection_state_cb, state, addr);
}

void lea_server_on_stack_state_changed(lea_server_stack_state_t enabled)
{
    lea_server_msg_t* msg = lea_server_msg_new(STACK_EVENT_STACK_STATE,
        NULL);
    if (!msg)
        return;

    msg->data.valueint1 = enabled;
    lea_server_send_message(msg);
}

void lea_server_on_connection_state_changed(bt_address_t* addr,
    profile_connection_state_t state)
{
    lea_server_msg_t* msg = lea_server_msg_new(STACK_EVENT_CONNECTION_STATE,
        addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    lea_server_send_message(msg);
}

void lea_server_on_storage_changed(void* value, uint32_t size)
{
    lea_server_msg_t* msg = lea_server_msg_new_ext(STACK_EVENT_STORAGE, NULL, value, size);
    if (!msg)
        return;

    lea_server_send_message(msg);
}

void lea_server_on_stream_added(bt_address_t* addr, uint32_t stream_id)
{
    lea_server_msg_t* msg = lea_server_msg_new(STACK_EVENT_STREAM_ADDED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_removed(bt_address_t* addr, uint32_t stream_id)
{
    lea_server_msg_t* msg = lea_server_msg_new(STACK_EVENT_STREAM_REMOVED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_started(lea_audio_stream_t* audio)
{
    lea_audio_stream_t* stream;
    lea_server_msg_t* msg;

    stream = lea_server_find_stream(audio->stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, audio->stream_id);
        return;
    }

    if (stream->is_source) {
        lea_audio_source_set_callback(&g_lea_source_callbacks);
    } else {
        lea_audio_sink_set_callback(&g_lea_sink_callbacks);
    }

    msg = lea_server_msg_new_ext(STACK_EVENT_STREAM_STARTED,
        &stream->addr, audio, sizeof(lea_audio_stream_t));
    if (!msg)
        return;

    lea_server_send_message(msg);
}

void lea_server_on_stream_stopped(uint32_t stream_id)
{
    lea_audio_stream_t* stream;
    lea_server_msg_t* msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
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
    lea_audio_stream_t* stream;
    lea_server_msg_t* msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
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
    lea_audio_stream_t* stream;
    lea_server_msg_t* msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
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
    lea_audio_stream_t* stream;
    lea_server_msg_t* msg;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    msg = lea_server_msg_new(STACK_EVENT_METADATA_UPDATED, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_server_send_message(msg);
}

void lea_server_on_stream_recv(uint32_t stream_id, uint32_t time_stamp,
    uint16_t seq_number, uint8_t* sdu, uint16_t size)
{
    lea_audio_stream_t* stream;
    lea_recv_iso_data_t* packet;

    stream = lea_server_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    packet = lea_audio_sink_packet_alloc(time_stamp, seq_number, sdu, size);
    if (!packet)
        return;

    // todo mix from many stream ?
    lea_audio_sink_packet_recv(packet);
}

bt_status_t lea_server_streams_started(bt_address_t* addr)
{
    lea_server_service_t* service = &g_lea_server_service;
    bt_list_t* list = service->leas_stream;
    lea_audio_stream_t* stream;
    bt_list_node_t* node;
    lea_server_msg_t* msg;
    lea_server_state_machine_t* leas_sm;

    leas_sm = get_state_machine(addr);
    if (!leas_sm) {
        BT_LOGE("failed, %s leas_sm null", __func__);
        return BT_STATUS_NOMEM;
    }

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        stream = bt_list_node(node);
        BT_LOGD("%s addr:%s, started:%d, stream_id:0x%08x", __func__, bt_addr_str(&stream->addr),
            stream->started, stream->stream_id);
        if (stream->started && (bt_addr_compare(addr, &stream->addr) == 0)) {
            msg = lea_server_msg_new_ext(STACK_EVENT_STREAM_STARTED,
                &stream->addr, stream, sizeof(lea_audio_stream_t));
            if (!msg)
                return BT_STATUS_NOMEM;

            lea_server_state_machine_dispatch(leas_sm, msg);
        }
    }

    return BT_STATUS_SUCCESS;
}

void lea_server_on_ascs_event(bt_address_t* addr, uint8_t id, uint8_t state, uint16_t type)
{
    lea_server_device_t* device;
    lea_server_event_t event;

    device = find_lea_server_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device(%s) not exist", __func__, bt_addr_str(addr));
        return;
    }

    update_server_ase(device, id, state, type);
    switch (state) {
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
        BT_LOGE("%s, unexpect state:%d", __func__, state);
        return;
    };
    }

    lea_server_send_event(addr, event);
}

void lea_server_on_csis_lock_state_changed(uint32_t csis_id, bt_address_t* addr, uint8_t lock)
{
    char* state[] = { "NA", "Unlocked", "Locked" };
    BT_LOGD("%s, addr:%s(%s)", __func__, bt_addr_str(addr), state[lock]);
}

bool lea_server_on_pacs_info_request(lea_pacs_info_t* pacs_info)
{
    lea_server_service_t* service = &g_lea_server_service;

    pacs_info->pac_number = sizeof(g_pacs_info) / sizeof(g_pacs_info[0]);
    pacs_info->pac_list = g_pacs_info;

    pacs_info->sink_location = service->sink_location;
    pacs_info->supported_ctx.sink = LEAS_CONTEXT_TYPE_ALL;
    pacs_info->available_ctx.sink = LEAS_CONTEXT_TYPE_ALL;

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER_SOURCE
    pacs_info->source_location = service->source_location;
    pacs_info->supported_ctx.source = CONFIG_LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX | ADPT_LEA_CONTEXT_TYPE_UNSPECIFIED;
    pacs_info->available_ctx.source = CONFIG_LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX | ADPT_LEA_CONTEXT_TYPE_UNSPECIFIED;
#endif

    return true;
}

bool lea_server_on_ascs_info_request(lea_ascs_info_t* ascs_info)
{
    ascs_info->sink_ase_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SINK_ASE_NUMBER;
    ascs_info->source_ase_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SOURCE_ASE_NUMBER;

    return true;
}

bool lea_server_on_bass_info_request(lea_bass_info_t* bass_info)
{
    bass_info->bass_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_BASS_STATE_NUMBER;
    return true;
}

bool lea_server_on_csis_info_request(lea_csis_infos_t* csis_info)
{
    uint8_t number;
    lea_csis_info_t* info;

    number = sizeof(g_csis_info) / sizeof(g_csis_info[0]);
    csis_info->csis_number = number;
    csis_info->csis_info = g_csis_info;

#if defined(CONFIG_KVDB) && defined(__NuttX__)
    for (uint8_t index = 0; index < number; index++) {
        info = &g_csis_info[index];
        info->set_size = property_get_int32("persist.bluetooth.csis.set_size", 1);
        info->rank = property_get_int32("persist.bluetooth.csis.rank", 1);
        property_get_buffer("persist.bluetooth.csis.set_sirk", info->sirk, 16);
    }
#endif

    return true;
}

static const profile_service_t lea_server_service = {
    .auto_start = true,
    .name = PROFILE_LEA_SERVER_NAME,
    .id = PROFILE_LEAUDIO_SERVER,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = lea_server_init,
    .startup = lea_server_startup,
    .shutdown = lea_server_shutdown,
    .process_msg = lea_server_process_msg,
    .get_state = NULL,
    .get_profile_interface = get_leas_profile_interface,
    .cleanup = lea_server_cleanup,
    .dump = lea_server_dump,
};

void register_lea_server_service(void)
{
    register_service(&lea_server_service);
}
