/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#define LOG_TAG "a2dp_source"
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_service_base.h"

#include "a2dp_ipc.h"
#include "btm_manager.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_control.h"
#include "bts_a2dp_event.h"
#include "bts_a2dp_source.h"
#include "bts_a2dp_source_audio.h"
#include "bts_a2dp_state_machine.h"
#include "bts_service.h"
#include "utils/utils.h"
#include "utils/log.h"

#ifndef CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#define A2DP_MAX_CONNECTION (1)
#else
#define A2DP_MAX_CONNECTION CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#endif

typedef struct {
    struct list_node node;
    a2dp_state_machine_t* a2dp_sm;
    bt_address bd_addr;
    const uint8_t peer_sep_;
} a2dp_device_t;

static void adp_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state);
static void adp_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state);
static void adp_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S* config);
static void adp_stream_channel_mtu_cb(BD_ADDR remote_addr, uint16_t stream_chnl_mtu);
static void a2dp_source_init(void);
static void a2dp_source_cleanup(void);

static a2dp_source_t a2dp_source;

A2DP_SOURCE_CALLBACKS_S a2dp_callback = {
    sizeof(a2dp_callback),
    adp_connection_state_changed_cb,
    adp_stream_state_changed_cb,
    adp_stream_config_changed_cb,
    adp_stream_channel_mtu_cb
};

a2dp_device_t* find_a2dp_device_by_addr(bt_address bd_addr)
{
    a2dp_device_t* device;
    struct list_node* node;

    list_for_every(&a2dp_source.device_list, node)
    {
        device = (a2dp_device_t*)node;
        if (memcmp(device->bd_addr, bd_addr, sizeof(bt_address)) == 0)
            return device;
    }

    return NULL;
}

static void set_active_peer(bt_address bd_addr)
{
    memcpy(a2dp_source.active_peer, bd_addr, 6);
}

static uint8_t* get_active_peer(void)
{
    return a2dp_source.active_peer;
}

static a2dp_device_t* a2dp_device_new(a2dp_state_machine_t* sm, bt_address bd_addr)
{
    a2dp_device_t* device;

    set_active_peer(bd_addr);
    device = (a2dp_device_t*)malloc(sizeof(a2dp_device_t));
    if (!device)
        return NULL;

    memcpy(device->bd_addr, bd_addr, sizeof(bt_address));
    device->a2dp_sm = sm;
    list_add_tail(&a2dp_source.device_list, &device->node);

    return device;
}

static void a2dp_device_delete(a2dp_device_t* device)
{
    a2dp_event_t* a2dp_event;

    if (!device)
        return;
    a2dp_event = a2dp_event_new(DISCONNECT_REQ, NULL);
    a2dp_state_machine_handle_event(device->a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
    a2dp_state_machine_destory(device->a2dp_sm);
    list_delete(&device->node);
    free((void*)device);
}

static a2dp_state_machine_t* get_state_machine(bt_address bd_addr)
{
    a2dp_state_machine_t* a2dp_sm;
    a2dp_device_t* device = find_a2dp_device_by_addr(bd_addr);
    if (device)
        return device->a2dp_sm;

    a2dp_sm = a2dp_state_machine_new(&a2dp_source, bd_addr);
    if (!a2dp_sm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    device = a2dp_device_new(a2dp_sm, bd_addr);
    if (!device) {
        BT_LOGE("New device alloc failed");
        a2dp_state_machine_destory(a2dp_sm);
        return NULL;
    }

    return a2dp_sm;
}

static void a2dp_service_handle_event(void* event, size_t size)
{
    a2dp_event_t* a2dp_event = (a2dp_event_t*)event;
    switch (a2dp_event->event) {
    case ENABLE:
        a2dp_source_init();
        break;
    case CLEANUP:
        a2dp_source_cleanup();
        break;
    case CODEC_CONFIG_EVT: {
        a2dp_codec_config_t* config;

        config = a2dp_event->event_data.data;
        bts_a2dp_codec_set_config(config);
        BT_LOGD("CODEC_CONFIG_EVT : codec_type: %d, sample_rate: %d, bits_per_sample: %d, channel_mode: %d",
            config->codec_type,
            config->sample_rate,
            config->bits_per_sample,
            config->channel_mode);
        break;
    }
    default: {
        a2dp_state_machine_t* a2dp_sm;
        a2dp_sm = get_state_machine(a2dp_event->event_data.bd_addr);
        if (!a2dp_sm)
            return;
        a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    } break;
    }

    a2dp_event_destory(a2dp_event);
}

static void do_in_a2dp_service(a2dp_event_t* a2dp_event)
{
    bts_send_uv_msg(BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID, a2dp_event, sizeof(a2dp_event_t));
}

static void adp_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    a2dp_state_machine_t* a2dp_sm;
    a2dp_event_type_t event;

    a2dp_sm = get_state_machine(remote_addr);
    if (!a2dp_sm)
        return;

    switch (state) {
    case SERVICE_PROFILE_DISCONNECTED:
        event = DISCONNECTED_EVT;
        break;
    case SERVICE_PROFILE_CONNECTED:
        event = CONNECTED_EVT;
        break;
    default:
        return;
    }

    do_in_a2dp_service(a2dp_event_new(event, remote_addr));
}

static void adp_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state)
{
    a2dp_event_type_t event;

    switch (state) {
    case A2DP_STREAM_UNKNOWN:
    case A2DP_STREAM_IDLE:
    case A2DP_STREAM_OPENED:
        return;
    case A2DP_STREAM_CLOSED:
        event = STREAM_CLOSED_EVT;
        break;
    case A2DP_STREAM_SUSPENDED:
        event = STREAM_SUSPENDED_EVT;
        break;
    case A2DP_STREAM_STREAMING: {
        event = STREAM_STARTED_EVT;
        break;
    }
    }

    do_in_a2dp_service(a2dp_event_new(event, remote_addr));
}

static uint32_t adp_stream_codec_transfer(uint8_t codec)
{
    bts_a2dp_codec_index_t index;

    switch (codec) {
    case SERVICE_AVDTP_CODEC_TYPE_SBC:
        index = BTS_A2DP_CODEC_INDEX_SOURCE_SBC;
        break;
    case SERVICE_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
        index = BTS_A2DP_CODEC_INDEX_SOURCE_AAC;
        break;
    default:
        index = BTS_A2DP_CODEC_INDEX_SOURCE_SBC;
        break;
    }

    return index;
}

static uint32_t adp_stream_sample_rate_transfer(uint32_t sample_rate)
{
    bts_a2dp_codec_sample_rate_t samplerate;

    switch (sample_rate) {
    case 44100:
        samplerate = BTS_A2DP_CODEC_SAMPLE_RATE_44100;
        break;
    case 48000:
        samplerate = BTS_A2DP_CODEC_SAMPLE_RATE_48000;
        break;
    case 88200:
        samplerate = BTS_A2DP_CODEC_SAMPLE_RATE_88200;
        break;
    case 96000:
        samplerate = BTS_A2DP_CODEC_SAMPLE_RATE_96000;
        break;
    case 176400:
        samplerate = BTS_A2DP_CODEC_SAMPLE_RATE_176400;
        break;
    case 192000:
        samplerate = BTS_A2DP_CODEC_SAMPLE_RATE_192000;
        break;
    default:
        BT_LOGE("Invalid sample rate: %d", sample_rate);
        return BTS_A2DP_CODEC_SAMPLE_RATE_44100;
    }

    return samplerate;
}

static void adp_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S* config)
{
    a2dp_event_t* event;
    a2dp_codec_config_t codec_config;

    codec_config.codec_type = adp_stream_codec_transfer(config->codec);
    codec_config.sample_rate = adp_stream_sample_rate_transfer(config->sample_rate);
    codec_config.channel_mode = config->channel == SERVICE_CHANNEL_MONO ? BTS_A2DP_CODEC_CHANNEL_MODE_MONO : BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
    codec_config.bits_per_sample = BTS_A2DP_CODEC_BITS_PER_SAMPLE_16;

    event = a2dp_event_new(CODEC_CONFIG_EVT, remote_addr);
    event->event_data.data = malloc(sizeof(codec_config));
    memcpy(event->event_data.data, &codec_config, sizeof(codec_config));

    do_in_a2dp_service(event);
}

static void adp_stream_channel_mtu_cb(BD_ADDR remote_addr, uint16_t stream_chnl_mtu)
{
    a2dp_event_t* event;

    event = a2dp_event_new(STREAM_MTU_CONFIG_EVT, remote_addr);
    event->event_data.mtu = stream_chnl_mtu;
    do_in_a2dp_service(event);
}

static void a2dp_source_init(void)
{
    SERVICE_BT_STATUS status;

    status = service_adapter_a2dp_source_init(A2DP_MAX_CONNECTION, &a2dp_callback);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s failed", __func__);
        return;
    }
    bts_a2dp_source_audio_init();
}

static void a2dp_source_cleanup(void)
{
    a2dp_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    a2dp_source.callbacks = NULL;
    bts_unregister_profile_process(BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID);
    list_for_every_safe(&a2dp_source.device_list, node, tmp)
    {
        device = (a2dp_device_t*)node;
        a2dp_device_delete(device);
    }
    service_adapter_a2dp_source_cleanup();
}

void bts_a2dp_service_handle_event(bt_profile_id id, void* data, size_t size)
{
    a2dp_service_handle_event(data, size);
}

uint8_t* bts_a2dp_source_active_peer(void)
{
    return get_active_peer();
}

void bts_a2dp_source_stream_start(void)
{
    uint8_t* addr = bts_a2dp_source_active_peer();

    do_in_a2dp_service(a2dp_event_new(STREAM_START_REQ, addr));
}

void bts_a2dp_source_stream_stop(void)
{
    uint8_t* addr = bts_a2dp_source_active_peer();

    do_in_a2dp_service(a2dp_event_new(STREAM_SUSPEND_REQ, addr));
}

void bts_a2dp_source_stream_suspend(void)
{
    uint8_t* addr = bts_a2dp_source_active_peer();

    do_in_a2dp_service(a2dp_event_new(STREAM_SUSPEND_REQ, addr));
}

bool bts_a2dp_source_stream_ready(void)
{
    a2dp_state_machine_t* a2dp_sm;

    uint8_t* addr = bts_a2dp_source_active_peer();
    a2dp_sm = get_state_machine(addr);
    if (!a2dp_sm)
        return false;

    return a2dp_state_machine_get_state(a2dp_sm) == A2DP_STATE_OPENED;
}

bool bts_a2dp_source_stream_started(void)
{
    a2dp_state_machine_t* a2dp_sm;

    uint8_t* addr = bts_a2dp_source_active_peer();
    a2dp_sm = get_state_machine(addr);
    if (!a2dp_sm)
        return false;

    return a2dp_state_machine_get_state(a2dp_sm) == A2DP_STATE_STARTED;
}

void bts_a2dp_source_codec_state_change(void)
{
    uint8_t* addr = bts_a2dp_source_active_peer();

    do_in_a2dp_service(a2dp_event_new(DEVICE_CODEC_STATE_CHANGE_EVT, addr));
}

bt_result_code bts_a2dp_source_init(const a2dp_source_callbacks_t* callbacks)
{
    a2dp_source.callbacks = callbacks;
    list_initialize(&a2dp_source.device_list);
    bts_register_profile_process(BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID,
        bts_a2dp_service_handle_event);
    do_in_a2dp_service(a2dp_event_new(ENABLE, NULL));

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_a2dp_source_connect(bt_address addr)
{
    do_in_a2dp_service(a2dp_event_new(CONNECT_REQ, addr));

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_a2dp_source_disconnect(bt_address addr)
{
    do_in_a2dp_service(a2dp_event_new(DISCONNECT_REQ, addr));

    return BT_RESULT_SUCCESS;
}

void bts_a2dp_source_cleanup(void)
{
    do_in_a2dp_service(a2dp_event_new(CLEANUP, NULL));
}

//show Device[1]: Addr: 04:7F:0E:00:00:1B, State: Opened, Active: true
void bts_a2dp_source_dump(void)
{
    a2dp_device_t* device;
    struct list_node* node;
    int i = 0;
    uint8_t is_active;
    const char *state;
    list_for_every(&a2dp_source.device_list, node)
    {
        i++;
        device = (a2dp_device_t*)node;
        if (memcmp(device->bd_addr, a2dp_source.active_peer, 6) == 0)
            is_active = 1;
        else
            is_active = 0;
        state = a2dp_state_machine_current_state(device->a2dp_sm);
        printf("\tDevice[%d]: Addr: %s, State: %s, Active: %s\n", i, addr_str(device->bd_addr), state, is_active ? "true" : "false");
    }
    if (i == 0)
        printf("\tNo A2dp Sink device found\n");
}