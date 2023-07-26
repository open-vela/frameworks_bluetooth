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
#define LOG_TAG "a2dp_src_tool"

#include <sys/stat.h>

#include "bt_tools.h"
#include "btm_a2dp_source.h"
#include "btm_manager.h"
#include "bts_a2dp_source.h"
#include "bts_service.h"
#include "debug.h"
#include "stdlib.h"
#include "string.h"
#include "sys/ioctl.h"
#include "sys/un.h"
#include "utils/log.h"

#define A2DPSRC_SAMPLE_FRAME_PER_SDU (7)
#define A2DPSRC_SAMPLE_FRAME_SIZE (77)
#define A2DPSRC_INTERVAL_MS (20)

typedef enum {
    SAMPLE_AUDIO_ACTIVE,
    SAMPLE_AUDIO_TERMINATING
} a2dpsrc_sample_audio_state_t;

enum {
    A2DPSRC_CTRL_CMD_START,
    A2DPSRC_CTRL_CMD_STOP,
    A2DPSRC_CTRL_CMD_CONFIG_DONE
};

typedef struct {
    int ctrl_fd;
    int data_fd;
    int audio_fd;
    uv_poll_t* poll;
    uv_poll_t* event;
    uv_mutex_t lock;
    a2dpsrc_sample_audio_state_t state;
} a2dpsrc_sample_audio_t;

static a2dpsrc_sample_audio_t* g_a2dpsrc_sample_info;
static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int play_cmd(void* handle, int argc, char* argv[]);
static int stop_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);

static const a2dp_source_interface_t* a2dp_source_interface = NULL;
static bt_command_t g_a2dp_source_tables[] = {
    BT_CMD("connect", connect_cmd, "\"connect a2dp sink device         param: <address> \""),
    BT_CMD("disconnect", disconnect_cmd, "\"disconnect peer a2dp sink device param: <address>\""),
    BT_CMD("play", play_cmd, "\"play sample audio repeatedly     param: <path>\""),
    BT_CMD("stop", stop_cmd, "\"stop playing sample audio\""),
    BT_CMD("dump", dump_cmd, "\"dump a2dp device state\""),
};

static struct option a2dp_source_options[] = {
#ifndef CONFIG_BLUETOOTH_DISABLE_HELP
    { "help", 0, 0, 'h' },
#endif
    { 0, 0, 0, 0 }
};

static void usage(void)
{
#ifndef CONFIG_BLUETOOTH_DISABLE_HELP
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_a2dp_source_tables); i++) {
        printf("\t%-8s\t%s\n", g_a2dp_source_tables[i].cmd, g_a2dp_source_tables[i].help);
    }
#endif
}

static void connection_state_callback(bt_address addr, a2dp_connection_state_t state)
{
    BT_LOGD("%s addr: %s, state:%d", __func__, addr_str(addr), state);
}

static void audio_state_callback(bt_address addr, a2dp_audio_state_t state)
{
    BT_LOGD("%s addr: %s, state:%d", __func__, addr_str(addr), state);
}

static void audio_source_config_callback(bt_address addr)
{
    BT_LOGD("%s addr: %s", __func__, addr_str(addr));
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 1)
        return -1;

    str2ba(argv[0], addr);
    a2dp_source_interface->connect(NULL, addr);

    return 0;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 1)
        return -1;

    str2ba(argv[0], addr);
    a2dp_source_interface->disconnect(NULL, addr);

    return 0;
}

static void a2dpsrc_sample_audio_free(a2dpsrc_sample_audio_t* info)
{
    if (info) {
        if (info->poll)
            bts_uv_poll_stop(info->poll);
        if (info->event)
            bts_service_event_poll_stop(info->event);
        if (info->data_fd >= 0)
            close(info->data_fd);
        if (info->ctrl_fd >= 0)
            close(info->ctrl_fd);
        if (info->audio_fd >= 0)
            close(info->audio_fd);
        free(info);
        BT_LOGD("sample audio released");
    }

    g_a2dpsrc_sample_info = NULL;
}

static void a2dpsrc_event_poll_cb(void* data)
{
    a2dpsrc_sample_audio_t* info = data;
    uint8_t buff;

    if (info->state != SAMPLE_AUDIO_ACTIVE) {
        buff = A2DPSRC_CTRL_CMD_STOP;
        if (send(info->ctrl_fd, &buff, 1, MSG_NOSIGNAL) < 0)
            info->state = SAMPLE_AUDIO_TERMINATING;
        a2dpsrc_sample_audio_free(info);
    }
}

static void a2dpsrc_sample_poll_cb(uv_poll_t* handle, int status, int events)
{
    a2dpsrc_sample_audio_t* info = handle->data;
    uint8_t frame[A2DPSRC_SAMPLE_FRAME_SIZE];
    struct stat statbuf;
    int offset;
    int space;
    int ret;

    if (info->state != SAMPLE_AUDIO_ACTIVE)
        goto termination;

    ret = ioctl(info->data_fd, FIONSPACE, &space);
    if (ret < 0)
        goto termination;

    while (space >= sizeof(frame)) {
        offset = 0;
        do {
            ret = read(info->audio_fd, frame + offset, sizeof(frame) - offset);
            if (ret < 0)
                goto termination;

            offset += ret;

            if (ret == 0 || offset != sizeof(frame)) {
                if (lseek(info->audio_fd, 0, SEEK_SET) < 0)
                    goto termination;

                ret = fstat(info->audio_fd, &statbuf);
                if (ret < 0 || statbuf.st_size <= 0)
                    goto termination;
            }

        } while (offset < sizeof(frame));

        ret = send(info->data_fd, frame, sizeof(frame), MSG_NOSIGNAL);
        if (ret <= 0)
            goto termination;

        space -= sizeof(frame);
    }

    return;

termination:
    info->state = SAMPLE_AUDIO_TERMINATING;

    a2dpsrc_event_poll_cb(info);
}

static int play_cmd(void* handle, int argc, char* argv[])
{
    int flags = SOCK_STREAM | SOCK_CLOEXEC;
    a2dpsrc_sample_audio_t* info;
    struct sockaddr_un addr;
    uint8_t buff;
    int ret;

    /* Step 1. error detection */
    if (argc < 1)
        return -1;

    if (g_a2dpsrc_sample_info) {
        BT_LOGE("repeated attempts on A2DP sample audio");
        return 0;
    }

    /* Step 2. initialization of sample audio entry */
    info = malloc(sizeof(a2dpsrc_sample_audio_t));
    if (!info)
        return 0;

    info->state = SAMPLE_AUDIO_ACTIVE;
    info->ctrl_fd = -1;
    info->data_fd = -1;
    info->audio_fd = -1;
    info->poll = NULL;
    info->event = NULL;

    /* Step 3. open audio file */
    info->audio_fd = open(argv[0], O_RDONLY);
    if (info->audio_fd < 0)
        goto error;

    /* Step 4. initialization of socket to send test audio */
    info->ctrl_fd = socket(AF_LOCAL, flags, 0);
    if (info->ctrl_fd < 0)
        goto error;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "source_ctrl");
    ret = connect(info->ctrl_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0)
        goto error;

    info->data_fd = socket(AF_LOCAL, flags, 0);
    if (info->data_fd < 0)
        goto error;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "source_data");
    ret = connect(info->data_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0)
        goto error;

    ret = A2DPSRC_SAMPLE_FRAME_SIZE * A2DPSRC_SAMPLE_FRAME_PER_SDU;
    ret = ioctl(info->data_fd, PIPEIOC_POLLOUTTHRD, ret);
    if (ret < 0)
        goto error;

    /* Step 5. start AVDTP stream */
    buff = A2DPSRC_CTRL_CMD_CONFIG_DONE;
    ret = send(info->ctrl_fd, &buff, 1, MSG_NOSIGNAL);
    if (ret <= 0)
        goto error;

    buff = A2DPSRC_CTRL_CMD_START;
    ret = send(info->ctrl_fd, &buff, 1, MSG_NOSIGNAL);
    if (ret <= 0)
        goto error;

    info->event = bts_service_event_poll_start(a2dpsrc_event_poll_cb, info);
    if (!info->event)
        goto error;

    info->poll = bts_uv_poll_start(info->data_fd, UV_WRITABLE | UV_DISCONNECT,
        (uv_poll_cb)a2dpsrc_sample_poll_cb, info);
    if (!info->poll)
        goto error;

    g_a2dpsrc_sample_info = info;
    return 0;

error:
    a2dpsrc_sample_audio_free(info);
    return -1;
}

static int stop_cmd(void* handle, int argc, char* argv[])
{
    a2dpsrc_sample_audio_t* info = g_a2dpsrc_sample_info;

    if (info && info->state != SAMPLE_AUDIO_TERMINATING) {
        info->state = SAMPLE_AUDIO_TERMINATING;
        bts_service_event_poll_signal(info->event);
    }

    return 0;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    bts_a2dp_source_dump();
    return 0;
}

static a2dp_source_callbacks_t a2dp_source_test_cbs = {
    sizeof(a2dp_source_callbacks_t),
    connection_state_callback,
    audio_state_callback,
    audio_source_config_callback
};

int a2dp_source_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (a2dp_source_interface == NULL) {
        a2dp_source_interface = get_a2dp_source_interface();
        a2dp_source_interface->set_callbacks(NULL, &a2dp_source_test_cbs);
    }

    while ((opt = getopt_long(argc, argv, "h", a2dp_source_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_a2dp_source_tables); i++) {
            if (strcmp(g_a2dp_source_tables[i].cmd, argv[1]) == 0) {
                if (g_a2dp_source_tables[i].func) {
                    ret = g_a2dp_source_tables[i].func(handle, argc - 2, &argv[2]);
                }
            }
        }
    }

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[1]);
        usage();
    }

    return 0;
}
