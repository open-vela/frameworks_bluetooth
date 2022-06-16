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
#define LOG_TAG "a2dp_sink"
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <connectivity/bt.h>
#include <uORB/uORB.h>
#include "stack_adapter_a2dp_sink.h"
#include "stack_adapter_service_base.h"

#include "a2dp_ipc.h"
#include "btm_manager.h"
#include "btm_a2dp_sink.h"
#include "bts_a2dp_sink.h"
#include "bts_a2dp_device.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_event.h"
#include "bts_a2dp_audio.h"
#include "bts_a2dp_state_machine.h"
#include "bts_service.h"
#include "utils/log.h"
#include "utils/utils.h"

#ifndef CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#define A2DP_MAX_CONNECTION (1)
#else
#define A2DP_MAX_CONNECTION CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#endif

static void adpt_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state);
static void adpt_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state);
static void adpt_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S *config);
static void adpt_packet_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_SINK_DATA_S *data);
static void adpt_stream_req_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_REQUEST request);
static void do_in_a2dp_snk_service(a2dp_event_t* a2dp_event);

static a2dp_sink_t a2dp_sink = {.enabled = false};

static A2DP_SINK_CALLBACKS_S a2dp_sink_callback = {
    sizeof(a2dp_sink_callback),
    adpt_connection_state_changed_cb,
    adpt_stream_state_changed_cb,
    adpt_stream_config_changed_cb,
    //CONFIG_SBC should be disabled, we dont need it.
    NULL,
    adpt_packet_received_cb,
    adpt_stream_req_received_cb,
};

static void set_active_peer(bt_address bd_addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&a2dp_sink.device_list, bd_addr);

    a2dp_sink.active_peer = &device->peer;
}

static a2dp_peer_t* get_active_peer(void)
{
    return a2dp_sink.active_peer;
}

static void a2dp_sink_init(void)
{
    SERVICE_BT_STATUS status;

    status = service_adapter_a2dp_sink_init(A2DP_MAX_CONNECTION, &a2dp_sink_callback);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s failed", __func__);
        return;
    }

    #if 0
    a2dp_sink.orb_fd = orb_advertise(ORB_ID(a2dp_state), NULL);
    if (a2dp_sink.orb_fd < 0) {
        BT_LOGE("a2dp_sink.orb_fd advertise failed");
        return;
    }
    #endif

    bts_a2dp_audio_init(SVR_SINK);
}

static void a2dp_sink_cleanup(void)
{
    a2dp_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    a2dp_sink.callbacks = NULL;
    a2dp_sink.active_peer = NULL;
    if (a2dp_sink.orb_fd > 0)
        orb_unadvertise(a2dp_sink.orb_fd);
    a2dp_sink.orb_fd = -1;
    bts_unregister_profile_process(BT_PROFILE_ADVANCED_AUDIO_SINK_ID);
    list_for_every_safe(&a2dp_sink.device_list, node, tmp)
    {
        device = (a2dp_device_t*)node;
        a2dp_device_delete(device);
    }
    bts_a2dp_audio_cleanup(SVR_SINK);
    service_adapter_a2dp_sink_cleanup();
}

static a2dp_device_t* find_or_create_device(bt_address bd_addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&a2dp_sink.device_list, bd_addr);
    if (device)
        return device;

    device = a2dp_device_new(&a2dp_sink, SEP_SRC, bd_addr);
    if (!device) {
        BT_LOGE("A2DP new source device alloc failed");
        return NULL;
    }
    list_add_tail(&a2dp_sink.device_list, &device->node);

    return device;
}

static a2dp_state_machine_t* get_state_machine(bt_address bd_addr)
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
    bts_a2dp_codec_set_config(SEP_SRC, &peer->codec_config);
}

static void a2dp_snk_service_handle_event(a2dp_event_t* a2dp_event, uint8_t peer_sep, size_t size)
{
    switch (a2dp_event->event) {
    case ENABLE:
        a2dp_sink_init();
        break;
    case CLEANUP:
        a2dp_sink_cleanup();
        break;
    case CODEC_CONFIG_EVT: {
        a2dp_codec_config_t* config;
        a2dp_device_t* device;

        device = find_or_create_device(a2dp_event->event_data.bd_addr);
        if (!device)
            break;

        config = a2dp_event->event_data.data;
        BT_LOGD("CODEC_CONFIG_EVT : codec_type: %d, sample_rate: %" PRIu32", bits_per_sample: %d, channel_mode: %d",
            config->codec_type,
            config->sample_rate,
            config->bits_per_sample,
            config->channel_mode);
        save_a2dp_codec_config(&device->peer, config);
        break;
    }
    case PEER_STREAM_START_REQ:
        service_adapter_a2dp_sink_accept_start_stream_req(a2dp_event->event_data.bd_addr);
        break;
    default: {
        a2dp_state_machine_t* a2dp_sm;

        a2dp_sm = get_state_machine(a2dp_event->event_data.bd_addr);
        if (!a2dp_sm)
            break;

        if (a2dp_event->event == CONNECTED_EVT)
            set_active_peer(a2dp_event->event_data.bd_addr);

        a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
        break;
    }
    }

    a2dp_event_destory(a2dp_event);
}

static void adpt_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    a2dp_event_type_t event;

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

    do_in_a2dp_snk_service(a2dp_event_new(event, remote_addr));
}

static void adpt_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state)
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
    case A2DP_STREAM_STREAMING:
        event = STREAM_STARTED_EVT;
        break;
    default:
        return;
    }

    do_in_a2dp_snk_service(a2dp_event_new(event, remote_addr));
}

static void adpt_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S *config)
{
    a2dp_event_t* event;
    a2dp_codec_config_t codec_config = {0};

    codec_config.codec_type = config->codec;
    codec_config.sample_rate = config->sample_rate;
    codec_config.channel_mode = config->channel;
    codec_config.bits_per_sample = config->bit_width;
    memcpy(codec_config.specific_info, config->codec_info, config->codec_info_len);
    event = a2dp_event_new(CODEC_CONFIG_EVT, remote_addr);
    event->event_data.data = malloc(sizeof(codec_config));
    memcpy(event->event_data.data, &codec_config, sizeof(codec_config));

    do_in_a2dp_snk_service(event);
}

static void adpt_packet_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_SINK_DATA_S *data)
{
    uint8_t *p;
    uint8_t offset;
    uint16_t seq, pktlen;
    uint32_t timestamp;
    a2dp_event_t * event;
    a2dp_sink_packet_t *packet;
    if (data == NULL || data->p_buffer == NULL)
        return;

    p = data->p_buffer;
    offset = 12 + (*p & 0x0F) * 4; //rtp header + ssrc
    pktlen = data->length - offset;
    p += 2;
    BE_STREAM_TO_UINT16(seq, p);
    BE_STREAM_TO_UINT32(timestamp, p);
    packet = bts_a2dp_sink_new_packet(timestamp, seq, data->p_buffer + offset, pktlen);
    if (packet == NULL) {
        BT_LOGE("%s, packet malloc failed", __func__);
        return;
    }
    event = a2dp_event_new(DATA_IND_EVT, remote_addr);
    event->event_data.packet = packet;
    do_in_a2dp_snk_service(event);
    //bts_a2dp_sink_packet_recieve(packet);
}

static void adpt_stream_req_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_REQUEST request)
{
    do_in_a2dp_snk_service(a2dp_event_new(PEER_STREAM_START_REQ, remote_addr));
}

static void bts_a2dp_snk_service_handle_event(bt_profile_id id, void* data, size_t size)
{
    a2dp_snk_service_handle_event((a2dp_event_t*)data, SEP_SRC, size);
}

static void do_in_a2dp_snk_service(a2dp_event_t* a2dp_event)
{
    if (a2dp_event == NULL)
        return;

    bts_send_uv_msg(BT_PROFILE_ADVANCED_AUDIO_SINK_ID, a2dp_event, sizeof(a2dp_event_t));
}

a2dp_peer_t* bts_a2dp_sink_find_peer(bt_address addr)
{
    a2dp_device_t* device = find_a2dp_device_by_addr(&a2dp_sink.device_list, addr);

    if (!device)
        return NULL;

    return &device->peer;
}

bool bts_a2dp_sink_stream_ready(void)
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

bool bts_a2dp_sink_stream_started(void)
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

void bts_a2dp_sink_codec_state_change(void)
{
    a2dp_peer_t* peer = get_active_peer();
    if (!peer)
        return;

    do_in_a2dp_snk_service(a2dp_event_new(DEVICE_CODEC_STATE_CHANGE_EVT, peer->bd_addr));
}

bt_result_code bts_a2dp_sink_init(const a2dp_sink_callbacks_t* callbacks)
{
    if (a2dp_sink.enabled)
        return BT_RESULT_SUCCESS;

    a2dp_sink.enabled = true;
    a2dp_sink.callbacks = callbacks;
    list_initialize(&a2dp_sink.device_list);
    bts_register_profile_process(BT_PROFILE_ADVANCED_AUDIO_SINK_ID,
        bts_a2dp_snk_service_handle_event);
    do_in_a2dp_snk_service(a2dp_event_new(ENABLE, NULL));
    a2dp_sink.enabled = true;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_a2dp_sink_connect(bt_address addr)
{
    if (!a2dp_sink.enabled)
        return BT_RESULT_FAILED;

    do_in_a2dp_snk_service(a2dp_event_new(CONNECT_REQ, addr));

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_a2dp_sink_disconnect(bt_address addr)
{
    if (!a2dp_sink.enabled)
        return BT_RESULT_FAILED;

    do_in_a2dp_snk_service(a2dp_event_new(DISCONNECT_REQ, addr));

    return BT_RESULT_SUCCESS;
}

void bts_a2dp_sink_cleanup(void)
{
    if (!a2dp_sink.enabled)
        return;

    a2dp_sink.enabled = false;
    do_in_a2dp_snk_service(a2dp_event_new(CLEANUP, NULL));
}

//show Device[1]: Addr: 04:7F:0E:00:00:1B, State: Opened, Active: true
void bts_a2dp_sink_dump(void)
{

}
