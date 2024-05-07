/****************************************************************************
 * frameworks/bluetooth/btservice/leaudio/audio_sink.c
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
#define LOG_TAG "lea_audio_sink"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "audio_transport.h"
#include "bt_time.h"
#include "bt_utils.h"
#include "lea_audio_sink.h"
#include "media_system.h"
#include "service_loop.h"
#include "utils/log.h"
#include "uv.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CTRL_EVT_HEADER_LEN 1
#define LEA_SINK_MEDIA_TICK_MS 10
#define LEA_MAX_DELAY_PACKET_COUNT 5
#define LEA_MAX_ENQUEUE_PACKET_COUNT 14
#define LEA_ASYNC_SEND_COUNT 14

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum {
    AUDIO_CTRL_CMD_START,
    AUDIO_CTRL_CMD_STOP,
    AUDIO_CTRL_CMD_CONFIG_DONE
} audio_ctrl_cmd_t;

typedef enum {
    AUDIO_CTRL_EVT_STARTED,
    AUDIO_CTRL_EVT_START_FAIL,
    AUDIO_CTRL_EVT_STOPPED,
    AUDIO_CTRL_EVT_UPDATE_CONFIG
} audio_ctrl_evt_t;

typedef enum {
    STREAM_STATE_OFF,
    STREAM_STATE_RUNNING,
    STREAM_STATE_FLUSHING
} stream_state_t;

typedef struct {
    bool ready;
    bool offloading;
    uint8_t packet_sending_cnt;
    uint64_t underflow_ts;
    uint32_t block_ticks;
    stream_state_t state;
    uv_mutex_t queue_lock;
    service_timer_t* recv_timer;
    struct list_node packet_queue;
    lea_audio_config_t audio_config;
} lea_sink_stream_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lea_sink_stream_t g_sink_stream;
static lea_sink_callabcks_t* g_sink_callbacks;
static audio_transport_t* g_sink_transport;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static bt_status_t lea_sink_update_codec(lea_audio_config_t* audio_config, bool enable);

/****************************************************************************
 * Private function
 ****************************************************************************/

static void lea_ctrl_event_with_data(uint8_t ch_id, audio_ctrl_evt_t event, uint8_t* data, uint8_t data_len)
{
    uint8_t stream[128];
    uint8_t* p = stream;

    BT_LOGD("%s, event:%d", __func__, event);

    UINT8_TO_STREAM(p, event);
    if (data_len) {
        ARRAY_TO_STREAM(p, data, data_len);
    }

    if (g_sink_transport != NULL) {
        audio_transport_write(g_sink_transport, ch_id, stream, data_len + CTRL_EVT_HEADER_LEN, NULL);
    }
}

static void lea_control_event(uint8_t ch_id, audio_ctrl_evt_t evt)
{
    lea_ctrl_event_with_data(ch_id, evt, NULL, 0);
}

static const char* audio_event_to_string(audio_ctrl_cmd_t event)
{
    switch (event) {
        CASE_RETURN_STR(AUDIO_CTRL_CMD_START)
        CASE_RETURN_STR(AUDIO_CTRL_CMD_STOP)
        CASE_RETURN_STR(AUDIO_CTRL_CMD_CONFIG_DONE)
    default:
        return "UNKNOWN_EVENT";
    }
}

static void lea_sink_recv_ctrl_data(audio_ctrl_cmd_t cmd)
{
    BT_LOGD("%s: lea-ctrl-cmd : %s", __func__, audio_event_to_string(cmd));

    switch (cmd) {
    case AUDIO_CTRL_CMD_START: {
        lea_audio_sink_start();
        lea_control_event(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL, AUDIO_CTRL_EVT_STARTED);
        if (g_sink_callbacks) {
            g_sink_callbacks->lea_audio_resume_cb();
        }
        break;
    }
    case AUDIO_CTRL_CMD_STOP: {
        lea_audio_sink_stop(false);
        if (g_sink_callbacks) {
            g_sink_callbacks->lea_audio_suspend_cb();
        }
        break;
    }
    case AUDIO_CTRL_CMD_CONFIG_DONE: {
        if (g_sink_callbacks) {
            g_sink_callbacks->lea_audio_meatadata_updated_cb();
        }
        break;
    }
    default: {
        BT_LOGW("%s: ### EVENT %d NOT HANDLED ###", __func__, cmd);
        break;
    }
    }
}

static void lea_sink_ctrl_buffer_alloc(uint8_t ch_id, uint8_t** buffer, size_t* len)
{
    if (ch_id != CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL) {
        BT_LOGE("fail, ch_id:%d", ch_id);
        return;
    }

    *len = 128;
    *buffer = malloc(*len);
}

static void lea_sink_ctrl_data_received(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    audio_ctrl_cmd_t cmd;
    uint8_t* pbuf = buffer;

    if (ch_id != CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL) {
        BT_LOGE("fail, ch_id:%d", ch_id);
        return;
    }

    if (len < 0) {
        BT_LOGE("%s, len:%d", __func__, len);
        audio_transport_read_stop(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL);
    }

    while (len > 0) {
        STREAM_TO_UINT8(cmd, pbuf);
        len--;
        lea_sink_recv_ctrl_data(cmd);
    }
    // free the buffer alloced by lea_sink_ctrl_buffer_alloc
    free(buffer);
}

static void lea_sink_ctrl_start(void)
{
    audio_transport_read_start(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL, lea_sink_ctrl_buffer_alloc, lea_sink_ctrl_data_received);
}

static void lea_sink_ctrl_stop(void)
{
    audio_transport_read_stop(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL);
}

static void lea_sink_ctrl_cb(uint8_t ch_id, audio_transport_event_t event)
{
    BT_LOGD("%s: ch_id:%d lea-ctrl-cmd : %s", __func__, ch_id, audio_transport_dump_event(event));

    if (ch_id != CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL) {
        BT_LOGE("fail, ch_id:%d", ch_id);
        return;
    }

    switch (event) {
    case TRANSPORT_OPEN_EVT: {
        lea_sink_stream_t* stream = &g_sink_stream;

        lea_sink_ctrl_start();
        if (stream->state == STREAM_STATE_RUNNING) {
            lea_sink_update_codec(&stream->audio_config, true);
        }
        break;
    }
    case TRANSPORT_CLOSE_EVT: {
        lea_sink_ctrl_stop();
        break;
    }
    default: {
        BT_LOGW("%s: ### EVENT %d NOT HANDLED ###", __func__, event);
        break;
    }
    }
}

static void lea_sink_data_cb(uint8_t ch_id, audio_transport_event_t event)
{
    BT_LOGD("%s: ch_id:%d lea-ctrl-cmd : %s", __func__, ch_id, audio_transport_dump_event(event));

    switch (event) {
    case TRANSPORT_OPEN_EVT: {
        break;
    }
    case TRANSPORT_CLOSE_EVT: {
        break;
    }
    default: {
        BT_LOGW("%s: ### LEA-DATA EVENT %d NOT HANDLED ###", __func__, event);
        break;
    }
    }
}

static void lea_sink_write_done(uint8_t ch_id, uint8_t* buffer)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    if (ch_id != CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_AUDIO) {
        BT_LOGE("%s, error ch_id:%d", __func__, ch_id);
        return;
    }

    if (stream->packet_sending_cnt > 0) {
        stream->packet_sending_cnt--;
    }
}

static void lea_sink_audio_handle_timer(service_timer_t* timer, void* data)
{
    lea_sink_stream_t* stream = (lea_sink_stream_t*)data;
    struct list_node* queue = &stream->packet_queue;
    lea_recv_iso_data_t* packet = NULL;
    struct list_node *node, *tmp;
    int ret;

    uv_mutex_lock(&stream->queue_lock);
    if (list_is_empty(queue) == true) {
        if (!stream->underflow_ts)
            stream->underflow_ts = get_os_timestamp_us();
        goto out;
    }

    if (stream->underflow_ts) {
        uint64_t now_us = get_os_timestamp_us();
        uint64_t miss_tick = (now_us - stream->underflow_ts) / (uint64_t)(LEA_SINK_MEDIA_TICK_MS * 1000);
        if (miss_tick > 2) {
            BT_LOGD("%s underflow, miss ticks: %" PRIu64, __func__, miss_tick);
        }
        stream->underflow_ts = 0;
    }

    list_for_every_safe(queue, node, tmp)
    {
        if (stream->packet_sending_cnt == LEA_ASYNC_SEND_COUNT) {
            if (stream->block_ticks++ > 2) {
                BT_LOGD("%s transport blocking, block ticks:%" PRIu32, __func__, stream->block_ticks);
            }
            goto out;
        }

        stream->block_ticks = 0;
        packet = (lea_recv_iso_data_t*)node;
        ret = audio_transport_write(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_AUDIO,
            packet->sdu, packet->length, lea_sink_write_done);
        if (ret != 0) {
            BT_LOGE("%s, packet write failed", __func__);
            goto out;
        }

        stream->packet_sending_cnt++;
        list_delete(node);
        lea_audio_sink_packet_free(packet);
    }

out:
    uv_mutex_unlock(&stream->queue_lock);
}

static void lea_sink_flush_packet_queue(void)
{
    lea_sink_stream_t* stream = &g_sink_stream;
    struct list_node *node, *tmp;

    list_for_every_safe(&stream->packet_queue, node, tmp)
    {
        list_delete(node);
        free(node);
    }
}

static bt_status_t lea_sink_update_codec(lea_audio_config_t* audio_config, bool enable)
{
    uint8_t buffer[64];
    uint8_t len;
    uint8_t* p = buffer;

    len = 21;
    /* set valid code */
    UINT8_TO_STREAM(p, enable);
    /* set codec type*/
    UINT32_TO_STREAM(p, audio_config->codec_type);
    /* set sample rate*/
    UINT32_TO_STREAM(p, audio_config->sample_rate);
    /* set bits_per_sample*/
    UINT32_TO_STREAM(p, audio_config->bits_per_sample);
    /* set channel_mode*/
    UINT32_TO_STREAM(p, audio_config->channel_mode);
    /* set bit rate*/
    UINT32_TO_STREAM(p, audio_config->bit_rate);

    len += 8;
    UINT32_TO_STREAM(p, audio_config->frame_size);
    UINT32_TO_STREAM(p, audio_config->packet_size);

    lea_ctrl_event_with_data(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL, AUDIO_CTRL_EVT_UPDATE_CONFIG, buffer, len);
    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * Public function
 ****************************************************************************/

void lea_audio_sink_set_callback(lea_sink_callabcks_t* callback)
{
    g_sink_callbacks = callback;
}

bt_status_t lea_audio_sink_init(bool offloading)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    BT_LOGD("%s, offloading:%d", __func__, offloading);
    if (g_sink_transport) {
        BT_LOGD("%s, already inited", __func__);
        return BT_STATUS_SUCCESS;
    }

    stream->recv_timer = NULL;
    stream->state = STREAM_STATE_OFF;
    stream->offloading = offloading;

    g_sink_transport = audio_transport_init(get_service_uv_loop());
    if (!g_sink_transport) {
        BT_LOGE("fail, audio_transport_init");
        return BT_STATUS_FAIL;
    }

    if (!audio_transport_open(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL,
            CONFIG_BLUETOOTH_LEA_SINK_CTRL_PATH, lea_sink_ctrl_cb)) {
        BT_LOGE("fail, audio_transport_open sink ctrl");
        return BT_STATUS_FAIL;
    }

    if (stream->offloading) {
        return BT_STATUS_SUCCESS;
    }

    uv_mutex_init(&stream->queue_lock);
    list_initialize(&stream->packet_queue);
    if (!audio_transport_open(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_AUDIO,
            CONFIG_BLUETOOTH_LEA_SINK_DATA_PATH, lea_sink_data_cb)) {
        BT_LOGE("fail, audio_transport_open sink audio");
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

lea_recv_iso_data_t* lea_audio_sink_packet_alloc(uint32_t timestamp, uint16_t seq, uint8_t* data, uint16_t length)
{
    lea_recv_iso_data_t* packet;

    (void)seq;
    (void)timestamp;

    packet = malloc(sizeof(lea_recv_iso_data_t) + length);
    if (!packet) {
        BT_LOGE("fail, packet malloc");
        return NULL;
    }

    packet->length = length;
    packet->seq = seq;
    packet->time_stamp = timestamp;
    memcpy(packet->sdu, data, length);

    return packet;
}

void lea_audio_sink_packet_free(lea_recv_iso_data_t* packet)
{
    free(packet);
}

bt_status_t lea_audio_sink_start(void)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    stream->ready = true;
    if (stream->state == STREAM_STATE_RUNNING) {
        BT_LOGD("%s stream was started", __func__)
        return BT_STATUS_FAIL;
    }

    stream->state = STREAM_STATE_RUNNING;
    if (stream->offloading) {
        return BT_STATUS_SUCCESS;
    }

    lea_sink_flush_packet_queue();
    return BT_STATUS_SUCCESS;
}

bt_status_t lea_audio_sink_stop(bool update_codec)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    BT_LOGD("%s, update_codec:%d", __func__, update_codec);
    stream->ready = false;
    if (stream->state == STREAM_STATE_OFF) {
        BT_LOGD("%s stream was stopped", __func__)
        return BT_STATUS_FAIL;
    }

    if (update_codec) {
        lea_sink_update_codec(&stream->audio_config, false);
    }

    if (!stream->offloading) {
        service_loop_cancel_timer(stream->recv_timer);
        stream->recv_timer = NULL;
        lea_sink_flush_packet_queue();
    }

    stream->state = STREAM_STATE_OFF;
    lea_control_event(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL, AUDIO_CTRL_EVT_STOPPED);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_audio_sink_suspend(void)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    if (!stream->ready) {
        BT_LOGE("fail, %s stream not ready", __func__);
        return BT_STATUS_FAIL;
    }

    return lea_audio_sink_stop(false);
}

bt_status_t lea_audio_sink_resume(void)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    if (!stream->ready) {
        BT_LOGE("fail, %s stream not ready", __func__);
        return BT_STATUS_FAIL;
    }

    return lea_audio_sink_start();
}

bt_status_t lea_audio_sink_mute(bool mute)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    if (!stream->ready) {
        BT_LOGE("fail, %s stream not ready", __func__);
        return BT_STATUS_FAIL;
    }

    if (mute) {
        return lea_audio_sink_stop(false);
    } else {
        return lea_audio_sink_resume();
    }
}

bt_status_t lea_audio_sink_update_codec(lea_audio_config_t* audio_config, uint16_t sdu_size)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    BT_LOGD("%s, codec_type:%d, sample_rate:%d, bits_per_sample:%d, channel_mode:%d, bit_rate:%d", __func__, audio_config->codec_type, audio_config->sample_rate, audio_config->bits_per_sample, audio_config->channel_mode, audio_config->bit_rate);

    (void)sdu_size;
    memcpy(&stream->audio_config, audio_config, sizeof(lea_audio_config_t));

    bt_media_set_lea_available();
    if (!lea_audio_sink_ctrl_is_connected()) {
        BT_LOGD("failed, %s ctrl transport was not connected", __func__);
        return BT_STATUS_IPC_ERROR;
    }

    return lea_sink_update_codec(audio_config, true);
}

void lea_audio_sink_packet_recv(lea_recv_iso_data_t* packet)
{
    lea_sink_stream_t* stream = &g_sink_stream;
    struct list_node* queue = &stream->packet_queue;

    if (packet == NULL) {
        return;
    }

    if (stream->state != STREAM_STATE_RUNNING) {
        free(packet);
        return;
    }

    uv_mutex_lock(&stream->queue_lock);
    if (list_length(queue) == LEA_MAX_ENQUEUE_PACKET_COUNT) {
        BT_LOGD("%s queue is full, drop head packet", __func__);
        struct list_node* pkt = list_remove_head(queue);
        free(pkt);
        list_add_tail(queue, &packet->node);
        uv_mutex_unlock(&stream->queue_lock);
        return;
    }

    list_add_tail(queue, &packet->node);
    if (list_length(queue) >= LEA_MAX_DELAY_PACKET_COUNT && !stream->recv_timer) {
        BT_LOGD("%s start trans packet", __func__);
        stream->underflow_ts = 0;
        stream->block_ticks = 0;
        stream->recv_timer = service_loop_timer(10, LEA_SINK_MEDIA_TICK_MS, lea_sink_audio_handle_timer, stream);
    }
    uv_mutex_unlock(&stream->queue_lock);
}

bool lea_audio_sink_is_started(void)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    return stream->ready;
}

bool lea_audio_sink_ctrl_is_connected(void)
{
    transport_conn_state_t state;

    state = audio_transport_get_state(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL);
    if (state == IPC_CONNTECTED) {
        return true;
    }

    return false;
}

void lea_audio_sink_cleanup(void)
{
    lea_sink_stream_t* stream = &g_sink_stream;

    stream->ready = false;

    audio_transport_close(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL);
    g_sink_transport = NULL;
    g_sink_callbacks = NULL;

    if (stream->offloading) {
        return;
    }

    audio_transport_close(g_sink_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_AUDIO);
}
