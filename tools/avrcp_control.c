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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_avrcp_control.h"
#include "bt_tools.h"

static int getattrs_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_avrcp_control_tables[] = {
    { "getattrs", getattrs_cmd, 0, "\"get element attributes from the peer device, params: <address>\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_avrcp_control_tables); i++) {
        printf("\t%-8s\t%s\n", g_avrcp_control_tables[i].cmd, g_avrcp_control_tables[i].help);
    }
}

static void* control_cbks_cookie = NULL;

static void avrcp_control_connection_state_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    PRINT_ADDR("avrcp_control_connection_state_cb, addr:%s, state:%d", addr, state);
}

static void avrcp_control_get_element_attribute_cb(void* cookie, bt_address_t* addr, uint8_t attrs_count, avrcp_element_attr_val_t* attrs)
{
    PRINT_ADDR("avrcp_control_get_element_attribute_cb, addr:%s, count:%d", addr, attrs_count);
    for (int i = 0; i < attrs_count; i++) {
        switch (attrs[i].attr_id) {
        case AVRCP_ATTR_TITLE:
            printf("title:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        case AVRCP_ATTR_ARTIST_NAME:
            printf("artist:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        case AVRCP_ATTR_ALBUM_NAME:
            printf("album:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        case AVRCP_ATTR_TRACK_NUMBER:
            printf("track number:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        case AVRCP_ATTR_TOTAL_NUMBER_OF_TRACKS:
            printf("total track number:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        case AVRCP_ATTR_GENRE:
            printf("genre:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        case AVRCP_ATTR_PLAYING_TIME_MS:
            printf("playing time:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        case AVRCP_ATTR_COVER_ART_HANDLE:
            printf("cover art handle:%s, charsetID:%d\n", attrs[i].text, attrs[i].chr_set);
            break;
        default:
            break;
        }
    }
}

static const avrcp_control_callbacks_t avrcp_control_cbs = {
    sizeof(avrcp_control_cbs),
    avrcp_control_connection_state_cb,
    avrcp_control_get_element_attribute_cb,
};

int avrcp_control_commond_init(void* handle)
{
    control_cbks_cookie = bt_avrcp_control_register_callbacks(handle, &avrcp_control_cbs);

    return 0;
}

int avrcp_control_commond_uninit(void* handle)
{
    bt_avrcp_control_unregister_callbacks(handle, control_cbks_cookie);

    return 0;
}

int avrcp_control_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_avrcp_control_tables, ARRAY_SIZE(g_avrcp_control_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}

static int getattrs_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_avrcp_control_get_element_attributes(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}