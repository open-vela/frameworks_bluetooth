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
#define LOG_TAG "lea_client_stm"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_addr.h"
#include "bt_lea_client.h"
#include "bt_list.h"
#include "lea_audio_sink.h"
#include "lea_audio_source.h"
#include "lea_client_service.h"
#include "lea_client_state_machine.h"
#include "sal_adapter_interface.h"
#include "sal_lea_client_interface.h"
#include "service_loop.h"

#include "bt_utils.h"
#include "utils/log.h"

typedef struct _lea_client_state_machine {
    state_machine_t sm;
    bt_address_t addr;
    void *service;
} lea_client_state_machine_t;

#define LEA_SERVER_STM_DEBUG 1

#if LEA_SERVER_STM_DEBUG
static void lea_client_trans_debug(state_machine_t *sm, bt_address_t *addr,
                                   const char *action);
static void lea_client_event_debug(state_machine_t *sm, bt_address_t *addr,
                                   uint32_t event);
static const char *stack_event_to_string(lea_client_event_t event);

#define LEAS_DBG_ENTER(__sm, __addr)          lea_client_trans_debug(__sm, __addr, "Enter")
#define LEAS_DBG_EXIT(__sm, __addr)           lea_client_trans_debug(__sm, __addr, "Exit ")
#define LEAS_DBG_EVENT(__sm, __addr, __event) lea_client_event_debug(__sm, __addr, __event);
#else
#define LEAS_DBG_ENTER(__sm, __addr)
#define LEAS_DBG_EXIT(__sm, __addr)
#define LEAS_DBG_EVENT(__sm, __addr, __event)
#endif

static void closed_enter(state_machine_t *sm);
static void closed_exit(state_machine_t *sm);
static void opening_enter(state_machine_t *sm);
static void opening_exit(state_machine_t *sm);
static void opened_enter(state_machine_t *sm);
static void opened_exit(state_machine_t *sm);
static void started_enter(state_machine_t *sm);
static void started_exit(state_machine_t *sm);
static void closing_enter(state_machine_t *sm);
static void closing_exit(state_machine_t *sm);

static bool closed_process_event(state_machine_t *sm, uint32_t event,
                                 void *p_data);
static bool opening_process_event(state_machine_t *sm, uint32_t event,
                                  void *p_data);
static bool opened_process_event(state_machine_t *sm, uint32_t event,
                                 void *p_data);
static bool started_process_event(state_machine_t *sm, uint32_t event,
                                  void *p_data);
static bool closing_process_event(state_machine_t *sm, uint32_t event,
                                  void *p_data);

static const state_t closed_state = {
    .state_name = "Closed",
    .enter = closed_enter,
    .exit = closed_exit,
    .process_event = closed_process_event,
};

static const state_t opening_state = {
    .state_name = "Opening",
    .enter = opening_enter,
    .exit = opening_exit,
    .process_event = opening_process_event,
};

static const state_t opened_state = {
    .state_name = "Opened",
    .enter = opened_enter,
    .exit = opened_exit,
    .process_event = opened_process_event,
};

static const state_t started_state = {
    .state_name = "Started",
    .enter = started_enter,
    .exit = started_exit,
    .process_event = started_process_event,
};

static const state_t closing_state = {
    .state_name = "Closing",
    .enter = closing_enter,
    .exit = closing_exit,
    .process_event = closing_process_event,
};

#if LEA_SERVER_STM_DEBUG
static void lea_client_trans_debug(state_machine_t *sm, bt_address_t *addr, const char *action)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s State=%s, Peer=[%s]", action, hsm_get_current_state_name(sm), addr_str);
}

static void lea_client_event_debug(state_machine_t *sm, bt_address_t *addr, uint32_t event)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm),
            addr_str, stack_event_to_string(event));
}

static const char *stack_event_to_string(lea_client_event_t event)
{
    switch (event) {
        CASE_RETURN_STR(CONNECT_DEVICE)
        CASE_RETURN_STR(DISCONNECT_DEVICE)
        CASE_RETURN_STR(CONNECT_AUDIO)
        CASE_RETURN_STR(DISCONNECT_AUDIO)
        CASE_RETURN_STR(STARTUP)
        CASE_RETURN_STR(SHUTDOWN)
        CASE_RETURN_STR(TIMEOUT)
        CASE_RETURN_STR(STACK_EVENT_STACK_STATE)
        CASE_RETURN_STR(STACK_EVENT_CONNECTION_STATE)
        CASE_RETURN_STR(STACK_EVENT_METADATA_UPDATED)
        CASE_RETURN_STR(STACK_EVENT_STORAGE)
        CASE_RETURN_STR(STACK_EVENT_SERVICE)
        CASE_RETURN_STR(STACK_EVENT_STREAM_ADDED)
        CASE_RETURN_STR(STACK_EVENT_STREAM_REMOVED)
        CASE_RETURN_STR(STACK_EVENT_STREAM_STARTED)
        CASE_RETURN_STR(STACK_EVENT_STREAM_STOPPED)
        CASE_RETURN_STR(STACK_EVENT_STREAM_RESUME)
        CASE_RETURN_STR(STACK_EVENT_STREAM_SUSPEND)
        CASE_RETURN_STR(STACK_EVENT_STREAN_RECV)
        CASE_RETURN_STR(STACK_EVENT_STREAN_SENT)
        CASE_RETURN_STR(STACK_EVENT_ASE_CODEC_CONFIG)
        CASE_RETURN_STR(STACK_EVENT_ASE_QOS_CONFIG)
        CASE_RETURN_STR(STACK_EVENT_ASE_ENABLING)
        CASE_RETURN_STR(STACK_EVENT_ASE_STREAMING)
        CASE_RETURN_STR(STACK_EVENT_ASE_DISABLING)
        CASE_RETURN_STR(STACK_EVENT_ASE_RELEASING)
        CASE_RETURN_STR(STACK_EVENT_ASE_IDLE)
    default:
        return "UNKNOWN_HF_EVENT";
    }
}
#endif

static void closed_enter(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_ENTER(sm, &leas_sm->addr);
    if (hsm_get_previous_state(sm)) {
        lea_client_notify_connection_state_changed(&leas_sm->addr,
                                                   PROFILE_STATE_DISCONNECTED);
    }
}

static void closed_exit(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_EXIT(sm, &leas_sm->addr);
}

static bool closed_process_event(state_machine_t *sm, uint32_t event,
                                 void *p_data)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;
    lea_client_data_t *data = (lea_client_data_t *)p_data;

    LEAS_DBG_EVENT(sm, &leas_sm->addr, event);

    switch (event) {
    case STACK_EVENT_STACK_STATE: {
        lea_client_notify_stack_state_changed(data->valueint1);
        break;
    }
    case CONNECT_DEVICE: {
        bt_status_t ret;

        ret = bt_sal_lea_client_connect(&leas_sm->addr);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s, ret:%d", __func__, ret);
        }
        break;
    }
    case STACK_EVENT_CONNECTION_STATE: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_CONNECTED: {
            lea_client_notify_connection_state_changed(&leas_sm->addr, state);
            hsm_transition_to(sm, &opening_state);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    return true;
}

static void opening_enter(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_ENTER(sm, &leas_sm->addr);
}

static void opening_exit(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_EXIT(sm, &leas_sm->addr);
}

static bool opening_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;
    lea_client_data_t *data = (lea_client_data_t *)p_data;

    LEAS_DBG_EVENT(sm, &leas_sm->addr, event);

    switch (event) {
    case STACK_EVENT_CONNECTION_STATE: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &closed_state);
            break;
        default:
            break;
        }
        break;
    }
    case CONNECT_AUDIO: {
        lea_client_ucc_add_streams(data->valueint1, &leas_sm->addr);
        lea_client_ucc_config_codec(data->valueint1, &leas_sm->addr);
        hsm_transition_to(sm, &opened_state);
        break;
    }
    case DISCONNECT_DEVICE: {
        bt_sal_lea_client_disconnect(&leas_sm->addr);
        break;
    }
    default:
        break;
    }

    return true;
}

static void opened_enter(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_ENTER(sm, &leas_sm->addr);
}

static void opened_exit(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_EXIT(sm, &leas_sm->addr);
}

static bool opened_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;
    lea_client_data_t *data = (lea_client_data_t *)p_data;

    LEAS_DBG_EVENT(sm, &leas_sm->addr, event);

    switch (event) {
    case STACK_EVENT_CONNECTION_STATE: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &closed_state);
            break;
        default:
            break;
        }
        break;
    }
    case STACK_EVENT_STREAM_ADDED: {
        lea_client_add_stream(data->valueint1, &leas_sm->addr);
        break;
    }
    case STACK_EVENT_STREAM_REMOVED: {
        lea_client_remove_stream(data->valueint1);
        break;
    }
    case STACK_EVENT_ASE_CODEC_CONFIG: {
        if (data->valueint2) {
            BT_LOGD("addr%s, stream:0x%08x, codec fail result:%d", bt_addr_str(&leas_sm->addr), data->valueint1, data->valueint2);
            return false;
        }
        lea_client_ucc_config_qos(data->valueint3, &leas_sm->addr, data->valueint1);
        break;
    }
    case STACK_EVENT_ASE_QOS_CONFIG: {
        if (data->valueint2) {
            BT_LOGD("addr%s, stream:0x%08x, qos fail result:%d", bt_addr_str(&leas_sm->addr), data->valueint1, data->valueint2);
            return false;
        }
        lea_client_ucc_enable(data->valueint3, &leas_sm->addr, data->valueint1);
        break;
    }
    case STACK_EVENT_ASE_ENABLING: {
        hsm_transition_to(sm, &started_state);
        break;
    }
    case DISCONNECT_DEVICE: {
        bt_sal_lea_client_disconnect(&leas_sm->addr);
        break;
    }
    default:
        break;
    }

    return true;
}

static void started_enter(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_ENTER(sm, &leas_sm->addr);
}

static void started_exit(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_EXIT(sm, &leas_sm->addr);
}

static uint8_t lea_client_get_channels(uint32_t allocation)
{
    uint8_t channels = 0;

    while (allocation) {
        if (allocation & 1) {
            channels++;
        }
        allocation >>= 1;
    }

    return channels;
}

static uint8_t lea_client_channel_mode(uint32_t allocation)
{
    uint8_t channels;
    uint8_t mode;

    channels = lea_client_get_channels(allocation);

    switch (channels) {
    case 1:
        mode = 0; // CHANNEL_MODE_MONO
        break;
    case 2:
        mode = 1; // CHANNEL_MODE_STEREO
        break;
    default:
        mode = 0; // CHANNEL_MODE_MONO
        break;
    }

    return mode;
}

static uint32_t lea_client_get_bitrate(lea_codec_config_t *config)
{
    uint8_t channels;
    uint8_t duration;

    channels = lea_client_get_channels(config->allocation);
    duration = config->duration == 0 ? 134 : 100; // 7.5 ms or 10 ms

    return 8 * channels * config->octets * duration;
}

static lea_audio_config_t lea_client_covert_audio_codec(lea_codec_config_t *config)
{
    lea_audio_config_t audio_config;
    uint32_t sample_rate_table[] = {
        0, 8000, 11025, 16000, 22050, 24000,
        32000, 44100, 48000, 88200, 96000,
        176400, 192000, 384000
    };

    memset(&audio_config, 0, sizeof(lea_codec_config_t));
    audio_config.codec_type = 11; // CODECT_TYPE_LC3
    audio_config.sample_rate = sample_rate_table[config->frequency];
    audio_config.bits_per_sample = 1; // CODEC_BITS_PER_SAMPLE_16
    audio_config.channel_mode = lea_client_channel_mode(config->allocation);
    audio_config.bit_rate = lea_client_get_bitrate(config);
    audio_config.frame_size = audio_config.sample_rate * (config->duration == 0 ? 0.0075 : 0.01);
    audio_config.packet_size = config->octets * lea_client_get_channels(config->allocation) * config->blocks;

    return audio_config;
}

static bool started_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;
    lea_client_data_t *data = (lea_client_data_t *)p_data;

    LEAS_DBG_EVENT(sm, &leas_sm->addr, event);

    switch (event) {
    case STACK_EVENT_CONNECTION_STATE: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &closed_state);
            break;
        default:
            break;
        }
        break;
    }
    case STACK_EVENT_ASE_STREAMING: {
        break;
    }
    case STACK_EVENT_STREAM_STARTED: {
        lea_audio_stream_t *remote_stream = (lea_audio_stream_t *)data->dataarry;
        lea_audio_stream_t *audio_stream;
        lea_audio_config_t audio_config;

        audio_stream = lea_client_find_update_stream(remote_stream);
        if (!audio_stream) {
            return false;
        }

        memcpy(&audio_stream->addr, &leas_sm->addr, sizeof(bt_address_t));
        audio_stream->started = true;
        audio_config = lea_client_covert_audio_codec(&audio_stream->codec_cfg);
        if (!audio_stream->is_source) {
            if (lea_audio_source_is_started()) {
                BT_LOGD("%s, already started", __func__);
                break;
            }
            lea_audio_source_update_codec(audio_stream->stream_id, &audio_config, audio_stream->sdu_size);
        } else {
            if (lea_audio_sink_is_started()) {
                BT_LOGD("%s, already started", __func__);
                break;
            }
            lea_audio_sink_update_codec(audio_stream->stream_id, &audio_config, audio_stream->sdu_size);
            lea_audio_sink_start(audio_stream->stream_id);
        }
        break;
    }
    case STACK_EVENT_STREAM_STOPPED: {
        lea_audio_stream_t *stream;

        stream = lea_client_find_stream(data->valueint1);
        if (!stream) {
            BT_LOGE("failed, stream %d not found", data->valueint1);
            return false;
        }

        stream->started = false;
        if (!stream->is_source) {
            lea_audio_source_stop(data->valueint1);
        } else {
            lea_audio_sink_stop(data->valueint1);
        }
        break;
    }
    case STACK_EVENT_ASE_DISABLING: {
        lea_client_ucc_remove_streams(data->valueint3, &leas_sm->addr);
        hsm_transition_to(sm, &closing_state);
        break;
    }
    case STACK_EVENT_ASE_RELEASING: {
        hsm_transition_to(sm, &closed_state);
        break;
    }
    case DISCONNECT_DEVICE: {
        bt_sal_lea_client_disconnect(&leas_sm->addr);
        break;
    }
    case DISCONNECT_AUDIO: {
        lea_client_ucc_disable(data->valueint1, &leas_sm->addr);
        break;
    }
    default:
        break;
    }

    return true;
}

static void closing_enter(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_ENTER(sm, &leas_sm->addr);
}

static void closing_exit(state_machine_t *sm)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;

    LEAS_DBG_EXIT(sm, &leas_sm->addr);
}

static bool closing_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    lea_client_state_machine_t *leas_sm = (lea_client_state_machine_t *)sm;
    lea_client_data_t *data = (lea_client_data_t *)p_data;

    LEAS_DBG_EVENT(sm, &leas_sm->addr, event);

    switch (event) {
    case STACK_EVENT_CONNECTION_STATE: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &closed_state);
            break;
        default:
            break;
        }
        break;
    }
    case CONNECT_AUDIO: {
        lea_client_ucc_add_streams(data->valueint1, &leas_sm->addr);
        lea_client_ucc_config_codec(data->valueint1, &leas_sm->addr);
        hsm_transition_to(sm, &opened_state);
        break;
    }
    case STACK_EVENT_ASE_RELEASING: {
        hsm_transition_to(sm, &closed_state);
        break;
    }
    case STACK_EVENT_STREAM_ADDED: {
        lea_client_add_stream(data->valueint1, &leas_sm->addr);
        break;
    }
    case STACK_EVENT_STREAM_REMOVED: {
        lea_client_remove_stream(data->valueint1);
        break;
    }
    case DISCONNECT_DEVICE: {
        bt_sal_lea_client_disconnect(&leas_sm->addr);
        break;
    }
    default:
        break;
    }

    return true;
}

lea_client_state_machine_t *lea_client_state_machine_new(bt_address_t *addr,
                                                         void *context)
{
    lea_client_state_machine_t *leasm;

    leasm = (lea_client_state_machine_t *)malloc(
        sizeof(lea_client_state_machine_t));
    if (!leasm)
        return NULL;

    memset(leasm, 0, sizeof(lea_client_state_machine_t));
    leasm->service = context;
    memcpy(&leasm->addr, addr, sizeof(bt_address_t));

    hsm_ctor(&leasm->sm, (state_t *)&closed_state);

    return leasm;
}

void lea_client_state_machine_destory(lea_client_state_machine_t *leasm)
{
    if (!leasm)
        return;

    hsm_dtor(&leasm->sm);
    free((void *)leasm);
}

void lea_client_state_machine_dispatch(lea_client_state_machine_t *leasm,
                                       lea_client_msg_t *msg)
{
    if (!leasm || !msg)
        return;

    hsm_dispatch_event(&leasm->sm, msg->event, &msg->data);
}

uint32_t lea_client_state_machine_get_state(lea_client_state_machine_t *leasm)
{
    return hsm_get_current_state_value(&leasm->sm);
}