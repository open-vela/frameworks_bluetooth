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
#include "bt_lea_tbs.h"
#include "bt_tools.h"

static int tbs_add(void* handle, int argc, char* argv[]);
static int tbs_remove(void* handle, int argc, char* argv[]);
static int tbs_set_telephone_bearer_info(void* handle, int argc, char* argv[]);
static int tbs_add_call(void* handle, int argc, char* argv[]);
static int tbs_remove_call(void* handle, int argc, char* argv[]);
static int tbs_provider_name_changed(void* handle, int argc, char* argv[]);
static int tbs_bearer_technology_changed(void* handle, int argc, char* argv[]);
static int tbs_uri_schemes_supported_list_changed(void* handle, int argc, char* argv[]);
static int tbs_signal_strength_changed(void* handle, int argc, char* argv[]);
static int tbs_signal_strength_report_interval_changed(void* handle, int argc, char* argv[]);
static int tbs_status_flags_changed(void* handle, int argc, char* argv[]);
static int tbs_call_state_changed(void* handle, int argc, char* argv[]);
static int tbs_notify_termination_reason(void* handle, int argc, char* argv[]);
static int tbs_call_control_response(void* handle, int argc, char* argv[]);

#define TBS_SET_BEARER "set bearer param: <ref><name><uci><uri_schemes><tech><strength><interval><status_flags><optional_op>"
#define TBS_ADD_CALL "add a call param: <index><state><flags><call_uri><incoming_target_uri><friendly_name>"

static bt_command_t g_lea_tbs_tables[] = {
    { "add", tbs_add, 0, "add TBS instance                         param: <NULL>" },
    { "remove", tbs_remove, 0, "remove TBS instance                      param: <NULL>" },
    { "tele", tbs_set_telephone_bearer_info, 0, TBS_SET_BEARER },
    { "call", tbs_add_call, 0, TBS_ADD_CALL },
    { "rmcall", tbs_remove_call, 0, "remove a call                            param: <call_index>" },
    { "providername", tbs_provider_name_changed, 0, "TBS notify provider name changed         param: <name>" },
    { "technology", tbs_bearer_technology_changed, 0, "TBS notify bearer tech changed           param: <technology>" },
    { "urischemes", tbs_uri_schemes_supported_list_changed, 0, "TBS notify uri supported list changed    param: <uri_schemes>" },
    { "signalstrength", tbs_signal_strength_changed, 0, "TBS notify signal strength changed       param: <strength>" },
    { "reportinterval", tbs_signal_strength_report_interval_changed, 0, "TBS notify ss report interval changed    param: <interval>" },
    { "statusflags", tbs_status_flags_changed, 0, "TBS notify status flags changed          param: <status_flags>" },
    { "state", tbs_call_state_changed, 0, "TBS notify call state                    param: <number><index><state><flags>" },
    { "term_reason", tbs_notify_termination_reason, 0, "TBS notify termination reason            param: <call_index><reason>" },
    { "resp", tbs_call_control_response, 0, "TBS call control response                param: <call_index><result>" },
};

static void* tbs_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_tbs_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_tbs_tables[i].cmd, g_lea_tbs_tables[i].help);
    }
}

/* interface */
static int tbs_add(void* handle, int argc, char* argv[])
{
    if (bt_lea_tbs_service_add(handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_remove(void* handle, int argc, char* argv[])
{
    if (bt_lea_tbs_service_remove(handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_set_telephone_bearer_info(void* handle, int argc, char* argv[])
{
    if (argc < 9)
        return CMD_PARAM_NOT_ENOUGH;

    lea_tbs_telephone_bearer_t* bearer = (lea_tbs_telephone_bearer_t*)malloc(sizeof(lea_tbs_telephone_bearer_t));
    bearer->bearer_ref = (void*)atoi(argv[0]);
    strcpy((char*)bearer->provider_name, argv[1]);
    strcpy((char*)bearer->uci, argv[2]);
    strcpy((char*)bearer->uri_schemes, argv[3]);
    bearer->technology = (uint8_t)atoi(argv[4]);
    bearer->signal_strength = (uint8_t)atoi(argv[5]);
    bearer->signal_strength_report_interval = (uint8_t)atoi(argv[6]);
    bearer->status_flags = (uint8_t)atoi(argv[7]);
    bearer->optional_opcodes_supported = (uint8_t)atoi(argv[8]);
    if (bt_lea_tbs_set_telephone_bearer_info(handle, bearer) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_add_call(void* handle, int argc, char* argv[])
{
    if (argc < 6)
        return CMD_PARAM_NOT_ENOUGH;

    lea_tbs_calls_t* call_s = (lea_tbs_calls_t*)malloc(sizeof(lea_tbs_calls_t));

    call_s->index = (uint8_t)atoi(argv[0]);
    call_s->state = (uint8_t)atoi(argv[1]);
    call_s->flags = (uint8_t)atoi(argv[2]);
    strcpy((char*)call_s->call_uri, argv[3]);
    strcpy((char*)call_s->incoming_target_uri, argv[4]);
    strcpy((char*)call_s->friendly_name, argv[5]);

    if (bt_lea_tbs_add_call(handle, call_s) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_remove_call(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t call_index = (uint8_t)atoi(argv[0]);

    if (bt_lea_tbs_remove_call(handle, call_index) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_provider_name_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t* name = (uint8_t*)atoi(argv[0]);

    if (bt_lea_tbs_provider_name_changed(handle, name) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_bearer_technology_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t technology = (uint8_t)atoi(argv[0]);

    if (bt_lea_tbs_bearer_technology_changed(handle, technology) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_uri_schemes_supported_list_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t* uri_schemes = (uint8_t*)atoi(argv[0]);

    if (bt_lea_tbs_uri_schemes_supported_list_changed(handle, uri_schemes) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_signal_strength_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t strength = (uint8_t)atoi(argv[0]);

    if (bt_lea_tbs_rssi_value_changed(handle, strength) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_signal_strength_report_interval_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t interval = (uint8_t)atoi(argv[0]);

    if (bt_lea_tbs_rssi_interval_changed(handle, interval) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_status_flags_changed(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t status_flags = (uint8_t)atoi(argv[0]);

    if (bt_lea_tbs_status_flags_changed(handle, status_flags) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_call_state_changed(void* handle, int argc, char* argv[])
{
    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t number = (uint8_t)atoi(argv[0]);
    lea_tbs_call_state_t* states_s;
    states_s = (lea_tbs_call_state_t*)malloc(sizeof(lea_tbs_call_state_t) * number);
    lea_tbs_call_state_t* sub_state = states_s;

    for (int i = 0; i < number; i++) {
        (sub_state)->index = (uint8_t)atoi(argv[i]);
        (sub_state)->state = (uint8_t)atoi(argv[i]);
        (sub_state)->flags = (uint8_t)atoi(argv[i]);
        sub_state++;
    }

    if (bt_lea_tbs_call_state_changed(handle, number, states_s) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_notify_termination_reason(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t call_index = (uint8_t)atoi(argv[0]);
    uint8_t reason = (uint8_t)atoi(argv[1]);

    if (bt_lea_tbs_notify_termination_reason(handle, call_index, reason) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int tbs_call_control_response(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    uint8_t call_index = (uint8_t)atoi(argv[0]);
    uint8_t result = atoi(argv[1]);

    if (bt_lea_tbs_call_control_response(handle, call_index, result) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void tbs_test_callback(void* cookie, uint8_t value, bool added)
{
    printf("lea_tbs_state_callback");
}

static const lea_tbs_callbacks_t lea_tbs_cbs = {
    sizeof(lea_tbs_cbs),
    tbs_test_callback,
};

int lea_tbs_command_init(void* handle)
{
    tbs_callbacks = bt_lea_tbs_register_callbacks(handle, &lea_tbs_cbs);

    return CMD_OK;
}

void lea_tbs_command_uninit(void* handle)
{
    bt_status_t ret;

    bt_lea_tbs_unregister_callbacks(handle, tbs_callbacks);

    ret = bluetooth_stop_service(handle, PROFILE_LEAUDIO_TBS);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
    }
}

int lea_tbs_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_tbs_tables, ARRAY_SIZE(g_lea_tbs_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
