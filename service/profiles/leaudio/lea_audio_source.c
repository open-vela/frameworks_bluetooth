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
#define LOG_TAG "lea_audio_source"

#include <errno.h>
#include <nuttx/mm/circbuf.h>
#include <stdlib.h>
#include <string.h>

#include "audio_transport.h"
#include "bt_utils.h"
#include "lea_audio_sink.h"
#include "lea_audio_source.h"
#include "media_system.h"
#include "service_loop.h"
#include "utils/log.h"
#include "uv.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

#define CTRL_EVT_HEADER_LEN 1

#define LEA_SINK_MEDIA_TICK_MS 20
#define LEA_MAX_DELAY_PACKET_COUNT 5
#define LEA_MAX_ENQUEUE_PACKET_COUNT 14
#define LEA_ASYNC_SEND_COUNT 14
#define MAX_FRAME_NUM_PER_TICK 14
#define STREAM_DELAY_MS 10
#define STREAM_FLUSH_SIZE 1024

typedef enum {
    STREAM_STATE_OFF,
    STREAM_STATE_RUNNING,
    STREAM_STATE_FLUSHING,
    STREAM_STATE_CONNECTING,
} stream_state_t;

typedef struct {
    bool offloading;
    stream_state_t stream_state;
    uint8_t read_congest;
    uint16_t sdu_size;
    uint32_t interval_ms;
    uint32_t sequence_number;
    uint32_t max_tx_length;
    service_timer_t* send_timer;
    lea_audio_config_t audio_config;
    struct circbuf_s stream_pool;
} lea_source_stream_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lea_source_stream_t g_source_stream;
static lea_source_callabcks_t* g_source_callbacks;
static audio_transport_t* g_source_transport;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static bt_status_t lea_source_update_codec(lea_audio_config_t* audio_config, bool enable);

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

    /* send event */
    if (g_source_transport != NULL) {
        audio_transport_write(g_source_transport, ch_id, stream, data_len + CTRL_EVT_HEADER_LEN, NULL);
    }
}

static void lea_control_event(uint8_t ch_id, audio_ctrl_evt_t evt)
{
    lea_ctrl_event_with_data(ch_id, evt, NULL, 0);
}

static void lea_audio_sink_alloc(uint8_t ch_id, uint8_t** buffer, size_t* len)
{
    lea_source_stream_t* stream = &g_source_stream;
    int space, next_to_read;
    uint8_t* alloc_buffer;

    if (stream->stream_state == STREAM_STATE_FLUSHING) {
        *len = STREAM_FLUSH_SIZE;
        *buffer = (uint8_t*)malloc(STREAM_FLUSH_SIZE);
        return;
    }

    // check pool buffer space enough to read one frame
    space = circbuf_space(&stream->stream_pool);
    if (space == 0) {
        BT_LOGE("%s, no enough space to read", __func__);
        *buffer = NULL;
        return;
    }

    next_to_read = space > stream->sdu_size ? stream->sdu_size : space;
    alloc_buffer = (void*)malloc(next_to_read);

    if (!alloc_buffer) {
        *buffer = NULL;
        audio_transport_read_stop(g_source_transport, ch_id);
        return;
    }

    *buffer = alloc_buffer;
    *len = next_to_read;
}

static void lea_audio_sink_recv(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    lea_source_stream_t* stream = &g_source_stream;
    int space;

    if (buffer == NULL)
        return;

    if (len <= 0) {
        if (len < 0)
            audio_transport_read_stop(g_source_transport, ch_id);

        goto out;
    }

    space = circbuf_space(&stream->stream_pool);
    if (len > space) {
        BT_LOGE("%s, unexpected len:%d", __func__, len);
        goto out;
    }

    circbuf_write(&stream->stream_pool, buffer, len);
    space = circbuf_space(&stream->stream_pool);
    if (space == 0) {
        BT_LOGD("%s, stream_pool over", __func__);
        audio_transport_read_stop(g_source_transport, ch_id);
    }

out:
    free(buffer);
}

static int lea_audio_source_read(uint8_t* buf, uint16_t frame_len)
{
    lea_source_stream_t* stream = &g_source_stream;

    return circbuf_read(&stream->stream_pool, buf, frame_len);
}

static void lea_audio_sink_handler(service_timer_t* timer, void* data)
{
    lea_source_stream_t* stream = (lea_source_stream_t*)data;
    uint8_t buf[512];
    int size;

    if (stream->stream_state != STREAM_STATE_RUNNING) {
        BT_LOGD("%s state:%d", __func__, stream->stream_state);
        return;
    }

    if (!g_source_callbacks) {
        BT_LOGE("%s, callbacks null", __func__);
        return;
    }

    audio_transport_read_start(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_AUDIO, lea_audio_sink_alloc, lea_audio_sink_recv);

    if (stream->sdu_size > 512) {
        BT_LOGE("%s, sdu_size:%d over", __func__, stream->sdu_size);
        return;
    }

    while (circbuf_used(&stream->stream_pool) >= stream->sdu_size) {
        size = lea_audio_source_read(buf, stream->sdu_size);
        g_source_callbacks->lea_audio_send_cb(buf, size);
    }
}

static void lea_ctrl_buffer_alloc(uint8_t ch_id, uint8_t** buffer, size_t* len)
{
    *len = 128;
    *buffer = malloc(*len);
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

static void lea_recv_ctrl_data(uint8_t ch_id, audio_ctrl_cmd_t cmd)
{
    BT_LOGD("%s: lea-ctrl-cmd : %s", __func__, audio_event_to_string(cmd));

    switch (cmd) {
    case AUDIO_CTRL_CMD_START: {
        lea_audio_source_start();
        lea_control_event(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL, AUDIO_CTRL_EVT_STARTED);
        if (g_source_callbacks) {
            g_source_callbacks->lea_audio_resume_cb();
        }
        break;
    }

    case AUDIO_CTRL_CMD_STOP: {
        lea_audio_source_stop(false);
        if (g_source_callbacks) {
            g_source_callbacks->lea_audio_suspend_cb();
        }
        break;
    }

    case AUDIO_CTRL_CMD_CONFIG_DONE: {
        if (g_source_callbacks) {
            g_source_callbacks->lea_audio_meatadata_updated_cb();
        }
        break;
    }

    default:
        BT_LOGD("%s: UNSUPPORTED CMD (%d)", __func__, cmd);
        break;
    }
}

static void lea_ctrl_data_received(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    audio_ctrl_cmd_t cmd;
    uint8_t* pbuf = buffer;

    if (len <= 0) {
        free(buffer);
        if (len < 0)
            audio_transport_read_stop(g_source_transport, ch_id);
        return;
    }

    while (len) {
        /* get cmd code*/
        STREAM_TO_UINT8(cmd, pbuf);
        len--;
        /* process cmd*/
        lea_recv_ctrl_data(ch_id, cmd);
    }
    // free the buffer alloced by lea_ctrl_buffer_alloc
    free(buffer);
}

static void lea_source_ctrl_start(void)
{
    audio_transport_read_start(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL, lea_ctrl_buffer_alloc, lea_ctrl_data_received);
}

static void lea_source_ctrl_stop(void)
{
    audio_transport_read_stop(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL);
}

static void lea_source_ctrl_cb(uint8_t ch_id, audio_transport_event_t event)
{
    BT_LOGD("%s: ch_id:%d audio-ctrl-cmd : %s", __func__, ch_id, audio_transport_dump_event(event));

    if (ch_id != CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL) {
        BT_LOGE("fail, ch_id:%d", ch_id);
        return;
    }

    switch (event) {
    case TRANSPORT_OPEN_EVT: {
        lea_source_stream_t* stream = &g_source_stream;

        lea_source_ctrl_start();
        if (stream->stream_state == STREAM_STATE_CONNECTING) {
            lea_source_update_codec(&stream->audio_config, true);
        }
        break;
    }
    case TRANSPORT_CLOSE_EVT: {
        lea_source_ctrl_stop();
        break;
    }
    default: {
        BT_LOGW("%s: ### EVENT %d NOT HANDLED ###", __func__, event);
        break;
    }
    }
}

static void lea_source_data_cb(uint8_t ch_id, audio_transport_event_t event)
{
    BT_LOGD("%s: ch_id:%d audio-ctrl-cmd : %s", __func__, ch_id, audio_transport_dump_event(event));

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

static bt_status_t lea_source_update_codec(lea_audio_config_t* audio_config, bool enable)
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

    lea_ctrl_event_with_data(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL, AUDIO_CTRL_EVT_UPDATE_CONFIG, buffer, len);
    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * Public function
 ****************************************************************************/

void lea_audio_source_set_callback(lea_source_callabcks_t* callback)
{
    g_source_callbacks = callback;
}

bt_status_t lea_audio_source_init(bool offloading)
{
    lea_source_stream_t* stream = &g_source_stream;

    BT_LOGD("%s, offloading:%d", __func__, offloading);
    if (g_source_transport) {
        BT_LOGD("%s, already inited", __func__);
        return BT_STATUS_SUCCESS;
    }

    stream->offloading = offloading;
    stream->send_timer = NULL;
    stream->stream_state = STREAM_STATE_OFF;
    stream->interval_ms = 10; // todo get from lc3 config

    g_source_transport = audio_transport_init(get_service_uv_loop());
    if (!g_source_transport) {
        BT_LOGE("fail, audio_transport_init");
        return BT_STATUS_FAIL;
    }

    if (!audio_transport_open(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL,
            CONFIG_BLUETOOTH_LEA_SOURCE_CTRL_PATH, lea_source_ctrl_cb)) {
        BT_LOGE("fail, audio_transport_open source ctrl");
        return BT_STATUS_FAIL;
    }

    if (stream->offloading) {
        return BT_STATUS_SUCCESS;
    }

    if (!audio_transport_open(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_AUDIO,
            CONFIG_BLUETOOTH_LEA_SOURCE_DATA_PATH, lea_source_data_cb)) {
        BT_LOGE("fail, audio_transport_open source audio");
        return BT_STATUS_FAIL;
    }

    circbuf_init(&stream->stream_pool, NULL, 4096);
    return BT_STATUS_SUCCESS;
}

bt_status_t lea_audio_source_start(void)
{
    lea_source_stream_t* stream = &g_source_stream;

    BT_LOGD("%s", __func__);
    if (stream->stream_state == STREAM_STATE_RUNNING) {
        BT_LOGD("%s. was running", __func__);
        return BT_STATUS_SUCCESS;
    }

    stream->stream_state = STREAM_STATE_RUNNING;
    if (stream->offloading) {
        return BT_STATUS_SUCCESS;
    }

    circbuf_reset(&stream->stream_pool);
    service_loop_cancel_timer(stream->send_timer);
    stream->send_timer = service_loop_timer(stream->interval_ms, stream->interval_ms, lea_audio_sink_handler, stream);
    return BT_STATUS_SUCCESS;
}

bt_status_t lea_audio_source_stop(bool update_codec)
{
    lea_source_stream_t* stream = &g_source_stream;

    BT_LOGD("%s,update_codec:%d", __func__, update_codec);

    if (stream->stream_state != STREAM_STATE_RUNNING) {
        BT_LOGE("%s, was stopped", __func__);
        return BT_STATUS_SUCCESS;
    }

    if (update_codec) {
        lea_source_update_codec(&stream->audio_config, false);
    }

    if (!stream->offloading) {
        audio_transport_read_stop(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_AUDIO);
        service_loop_cancel_timer(stream->send_timer);
        stream->send_timer = NULL;
    }

    stream->stream_state = STREAM_STATE_OFF;
    lea_control_event(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL, AUDIO_CTRL_EVT_STOPPED);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_audio_source_suspend(void)
{
    return lea_audio_source_stop(false);
}

bt_status_t lea_audio_source_resume(void)
{
    return lea_audio_source_start();
}

bt_status_t lea_audio_source_update_codec(lea_audio_config_t* audio_config, uint16_t sdu_size)
{
    lea_source_stream_t* stream = &g_source_stream;

    memcpy(&stream->audio_config, audio_config, sizeof(lea_audio_config_t));
    stream->sdu_size = sdu_size;

    BT_LOGD("%s, codec_type:%d, sample_rate:%d, bits_per_sample:%d, channel_mode:%d, bit_rate:%d", __func__, audio_config->codec_type, audio_config->sample_rate, audio_config->bits_per_sample, audio_config->channel_mode, audio_config->bit_rate);

    bt_media_set_lea_available();
    if (!lea_audio_source_ctrl_is_connected()) {
        stream->stream_state = STREAM_STATE_CONNECTING;
        BT_LOGD("failed, %s ctrl transport was not connected", __func__);
        return BT_STATUS_IPC_ERROR;
    }

    return lea_source_update_codec(audio_config, true);
}

bool lea_audio_source_is_started(void)
{
    lea_source_stream_t* stream = &g_source_stream;

    return stream->stream_state != STREAM_STATE_OFF;
}

bool lea_audio_source_ctrl_is_connected(void)
{
    transport_conn_state_t state;

    state = audio_transport_get_state(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL);
    if (state == IPC_CONNTECTED) {
        return true;
    }

    return false;
}

void lea_audio_source_cleanup(void)
{
    lea_source_stream_t* stream = &g_source_stream;

    stream->stream_state = STREAM_STATE_OFF;
    audio_transport_close(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL);
    g_source_transport = NULL;
    g_source_callbacks = NULL;

    if (stream->offloading) {
        return;
    }

    audio_transport_close(g_source_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_AUDIO);
}
