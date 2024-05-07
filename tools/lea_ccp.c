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
#include "bt_lea_ccp.h"
#include "bt_tools.h"

/// lea_interface_t
static int ccp_read_bearer_provider_name(void* handle, int argc, char* argv[]);
static int ccp_read_bearer_uci(void* handle, int argc, char* argv[]);
static int ccp_read_bearer_technology(void* handle, int argc, char* argv[]);
static int ccp_read_bearer_uri_schemes_supported_list(void* handle, int argc, char* argv[]);
static int ccp_read_bearer_signal_strength(void* handle, int argc, char* argv[]);
static int ccp_read_bearer_signal_strength_report_interval(void* handle, int argc, char* argv[]);
static int ccp_read_content_control_id(void* handle, int argc, char* argv[]);
static int ccp_read_status_flags(void* handle, int argc, char* argv[]);
static int ccp_read_call_control_optional_opcodes(void* handle, int argc, char* argv[]);
static int ccp_read_incoming_call(void* handle, int argc, char* argv[]);
static int ccp_read_incoming_call_target_bearer_uri(void* handle, int argc, char* argv[]);
static int ccp_read_call_state(void* handle, int argc, char* argv[]);
static int ccp_read_bearer_list_current_calls(void* handle, int argc, char* argv[]);
static int ccp_read_call_friendly_name(void* handle, int argc, char* argv[]);
static int ccp_call_control_by_index(void* handle, int argc, char* argv[]);
static int ccp_originate_call(void* handle, int argc, char* argv[]);
static int ccp_join_calls(void* handle, int argc, char* argv[]);

#define ccp_CALL_CONTROL "call control by index                   param: <addr><opcode>"
#define ccp_ORIGINATE_CALL "originate                               param: <addr><uri>"
#define ccp_JOIN_CALL "join                                    param: <addr><number><call_index1><call_index2>"

static bt_command_t g_lea_ccp_tables[] = {
    { "readprovidername", ccp_read_bearer_provider_name, 0, "read bearer provider name               param: <addr>" },
    { "readuci", ccp_read_bearer_uci, 0, "read bearer uci                         param: <addr>" },
    { "readtech", ccp_read_bearer_technology, 0, "read bearer technology                  param: <addr>" },
    { "readurischemeslist", ccp_read_bearer_uri_schemes_supported_list, 0, "read bearer uri schemes supported list  param: <addr>" },
    { "readstrength", ccp_read_bearer_signal_strength, 0, "read bearer signal strength             param: <addr>" },
    { "readinterval", ccp_read_bearer_signal_strength_report_interval, 0, "read ss report interval                 param: <addr>" },
    { "readccid", ccp_read_content_control_id, 0, "read ccid                               param: <addr>" },
    { "readstatusflags", ccp_read_status_flags, 0, "read status flags                       param: <addr>" },
    { "readopcode", ccp_read_call_control_optional_opcodes, 0, "read call control optional opcode       param: <addr>" },
    { "readincoming", ccp_read_incoming_call, 0, "read incoming call                      param: <addr>" },
    { "readtargeturi", ccp_read_incoming_call_target_bearer_uri, 0, "read incoming call target bearer uri    param: <addr>" },
    { "readcallstate", ccp_read_call_state, 0, "read call state                         param: <addr>" },
    { "readlistcall", ccp_read_bearer_list_current_calls, 0, "read bearer list current call           param: <addr>" },
    { "readfriendlyname", ccp_read_call_friendly_name, 0, "read call friendly name                 param: <addr>" },
    { "callcontrol", ccp_call_control_by_index, 0, ccp_CALL_CONTROL },
    { "originate", ccp_originate_call, 0, ccp_ORIGINATE_CALL },
    { "join", ccp_join_calls, 0, ccp_JOIN_CALL },
};

static void* ccp_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_ccp_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_ccp_tables[i].cmd, g_lea_ccp_tables[i].help);
    }
}

/* interface */
static int ccp_read_bearer_provider_name(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_bearer_provider_name(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_bearer_uci(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_bearer_uci(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_bearer_technology(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_bearer_technology(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_bearer_uri_schemes_supported_list(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_bearer_uri_schemes_supported_list(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_bearer_signal_strength(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_bearer_signal_strength(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_bearer_signal_strength_report_interval(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_bearer_signal_strength_report_interval(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_content_control_id(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_content_control_id(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_status_flags(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_status_flags(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_call_control_optional_opcodes(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_call_control_optional_opcodes(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_incoming_call(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_incoming_call(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_incoming_call_target_bearer_uri(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_incoming_call_target_bearer_uri(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_call_state(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_call_state(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_bearer_list_current_calls(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_bearer_list_current_calls(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_read_call_friendly_name(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_ccp_read_call_friendly_name(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

/*write opcode */
static int ccp_call_control_by_index(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    uint8_t opcode;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    opcode = atoi(argv[1]);

    if (bt_lea_ccp_call_control_by_index(handle, &addr, opcode) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_originate_call(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    uint8_t* uri;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    uri = (uint8_t*)argv[1];

    if (bt_lea_ccp_originate_call(handle, &addr, uri) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int ccp_join_calls(void* handle, int argc, char* argv[])
{
    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    uint8_t number;
    uint8_t list_of_call_indexex[5];
    uint8_t* call_indexes;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    number = (uint8_t)atoi(argv[1]);
    list_of_call_indexex[0] = (uint8_t)atoi(argv[2]);
    list_of_call_indexex[1] = (uint8_t)atoi(argv[3]);
    call_indexes = list_of_call_indexex;

    if (bt_lea_ccp_join_calls(handle, &addr, number, call_indexes) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void ccp_test_callback(void* context, bt_address_t* addr)
{
    PRINT_ADDR("ccp_test_callback, addr:%s", addr);
}

static const lea_ccp_callbacks_t lea_ccp_cbs = {
    sizeof(lea_ccp_cbs),
    ccp_test_callback,
};

int lea_ccp_command_init(void* handle)
{
    ccp_callbacks = bt_lea_ccp_register_callbacks(handle, &lea_ccp_cbs);

    return CMD_OK;
}

void lea_ccp_command_uninit(void* handle)
{
    bt_status_t ret;

    bt_lea_ccp_unregister_callbacks(handle, ccp_callbacks);

    ret = bluetooth_stop_service(handle, PROFILE_LEAUDIO_CCP);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
    }
}

int lea_ccp_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_ccp_tables, ARRAY_SIZE(g_lea_ccp_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
