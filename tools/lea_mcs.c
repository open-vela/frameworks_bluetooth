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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_lea_mcs.h"
#include "bt_tools.h"

static int le_mcs_add(void* handle, int argc, char* argv[]);
static int le_mcs_remove(void* handle, int argc, char* argv[]);
static int mcs_media_state_changed(void* handle, int argc, char* argv[]);
static int mcs_set_media_player_info(void* handle, int argc, char* argv[]);
static int mcs_media_control_response(void* handle, int argc, char* argv[]);
static int mcs_playing_order_changed(void* handle, int argc, char* argv[]);
static int mcs_playback_speed_changed(void* handle, int argc, char* argv[]);
static int mcs_seeking_speed_changed(void* handle, int argc, char* argv[]);
static int mcs_track_title_changed(void* handle, int argc, char* argv[]);
static int mcs_track_duration_changed(void* handle, int argc, char* argv[]);
static int mcs_track_position_changed(void* handle, int argc, char* argv[]);
static int mcs_current_track_changed(void* handle, int argc, char* argv[]);
static int mcs_next_track_changed(void* handle, int argc, char* argv[]);
static int mcs_current_group_changed(void* handle, int argc, char* argv[]);
static int mcs_parent_group_changed(void* handle, int argc, char* argv[]);

static bt_command_t g_lea_mcs_tables[] = {
    { "add", le_mcs_add, 0, "MCS instance add                             param: <NULL>" },
    { "remove", le_mcs_remove, 0, "MCS instance remove                          param: <NULL>" },
    { "mediastatechanged", mcs_media_state_changed, 0, "MCS notify media state                       param: <state>" },
    { "setmediaplayer", mcs_set_media_player_info, 0, "set media player info                        param: <NULL>" },
    { "mediacontrolrsp", mcs_media_control_response, 0, "notify media control point process result    param: <result>" },
    { "playingorderchanged", mcs_playing_order_changed, 0, "MCS notify playing order changed             param: <order>" },
    { "playbackspeedchanged", mcs_playback_speed_changed, 0, "MCS notify playback speed changed            param: <speed>" },
    { "seekingspeedchanged", mcs_seeking_speed_changed, 0, "MCS notify seeking speed changed             param: <speed>" },
    { "tracktitlechanged", mcs_track_title_changed, 0, "MCS notify track title changed               param: <title>" },
    { "trackdurationchanged", mcs_track_duration_changed, 0, "MCS notify track duration changed            param: <duration>" },
    { "trackpositionchanged", mcs_track_position_changed, 0, "MCS notify track position changed            param: <position>" },
    { "currenttrackchanged", mcs_current_track_changed, 0, "MCS notify current track changed             param: <track_id[6] six octets>" },
    { "nexttrackchanged", mcs_next_track_changed, 0, "MCS notify next track changed                param: <track_id[6] six octets>" },
    { "currentgroupchanged", mcs_current_group_changed, 0, "MCS notify current group changed             param: <group_id[6] six octets>" },
    { "parentgroupchanged", mcs_parent_group_changed, 0, "MCS notify parent group changed              param: <group_id[6] six octets>" },
};

static void* mcs_callbacks = NULL;
static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_mcs_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_mcs_tables[i].cmd, g_lea_mcs_tables[i].help);
    }
}

/* interface */
static int le_mcs_add(void* handle, int argc, char* argv[])
{
    if (bt_lea_mcs_service_add(handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int le_mcs_remove(void* handle, int argc, char* argv[])
{
    if (bt_lea_mcs_service_remove(handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_media_state_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    lea_adpt_mcs_media_state_t state = atoi(argv[0]);

    if (bt_lea_mcs_media_state_changed(handle, state) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_set_media_player_info(void* handle, int argc, char* argv[])
{
    if (bt_lea_mcs_set_media_player_info(handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_media_control_response(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    lea_adpt_mcs_media_control_result_t result = atoi(argv[0]);

    if (bt_lea_mcs_media_control_point_response(handle, result) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_playing_order_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t order = atoi(argv[0]);

    if (bt_lea_mcs_playing_order_changed(handle, order) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_playback_speed_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t speed = atoi(argv[0]);

    if (bt_lea_mcs_playback_speed_changed(handle, speed) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_seeking_speed_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t speed = atoi(argv[0]);

    if (bt_lea_mcs_seeking_speed_changed(handle, speed) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_track_title_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t* title = (uint8_t*)strdup(argv[0]);

    if (bt_lea_mcs_track_title_changed(handle, title) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_track_duration_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t duration = atoi(argv[0]);

    if (bt_lea_mcs_track_duration_changed(handle, duration) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_track_position_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int32_t position = atoi(argv[0]);

    if (bt_lea_mcs_track_position_changed(handle, position) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_current_track_changed(void* handle, int argc, char* argv[])
{
    if (argc != 6)
        return CMD_PARAM_NOT_ENOUGH;

    lea_object_id track_id;
    for (int i = 0; i <= 5; i++) {
        track_id[i] = strtol(argv[i], NULL, 16);
    }

    if (bt_lea_mcs_current_track_change(handle, track_id) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_next_track_changed(void* handle, int argc, char* argv[])
{
    if (argc != 6)
        return CMD_PARAM_NOT_ENOUGH;

    lea_object_id track_id;
    for (int i = 0; i <= 5; i++) {
        track_id[i] = strtol(argv[i], NULL, 16);
    }

    if (bt_lea_mcs_next_track_changed(handle, track_id) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_current_group_changed(void* handle, int argc, char* argv[])
{
    if (argc != 6)
        return CMD_PARAM_NOT_ENOUGH;

    lea_object_id group_id;
    for (int i = 0; i <= 5; i++) {
        group_id[i] = strtol(argv[i], NULL, 16);
    }

    if (bt_lea_mcs_current_group_changed(handle, group_id) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcs_parent_group_changed(void* handle, int argc, char* argv[])
{
    if (argc != 6)
        return CMD_PARAM_NOT_ENOUGH;

    lea_object_id group_id;
    for (int i = 0; i <= 5; i++) {
        group_id[i] = strtol(argv[i], NULL, 16);
    }

    if (bt_lea_mcs_parent_group_changed(handle, group_id) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void lea_mcs_test_callback(void* cookie, uint8_t event)
{
    printf("lea_mcs_test_callback");
}

static const lea_mcs_callbacks_t lea_mcs_cbs = {
    sizeof(lea_mcs_cbs),
    lea_mcs_test_callback,
};

int lea_mcs_commond_init(void* handle)
{
    mcs_callbacks = bt_lea_mcs_register_callbacks(handle, &lea_mcs_cbs);

    return CMD_OK;
}

void lea_mcs_commond_uninit(void* handle)
{
    bt_status_t ret;

    bt_lea_mcs_unregister_callbacks(handle, mcs_callbacks);
    ret = bluetooth_stop_service(handle, PROFILE_LEAUDIO_MCS);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
    }
}

int lea_mcs_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_mcs_tables, ARRAY_SIZE(g_lea_mcs_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
