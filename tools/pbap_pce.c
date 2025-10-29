/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#include <stdlib.h>
#include <string.h>

#include "bt_pbap_pce.h"
#include "bt_tools.h"

#define PROPERTY_NAME_STR "Name"
#define PROPERTY_NUMBER_STR "Number"
#define PROPERTY_SOUND_STR "Sound"

#define RETRIVE_BY_NAME 0
#define RETRIVE_BY_NUMBER 1

#define CHECK_VCARD_PROPERTY(in, property_str) \
    (strlen(in) == strlen(property_str) && strncasecmp(in, property_str, strlen(property_str)) == 0)

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int get_contact_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);
static int set_blacklist_cmd(void* handle, int argc, char* argv[]);
static int remove_blacklist_cmd(void* handle, int argc, char* argv[]);
static int is_in_blacklist_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_pce_tables[] = {
    { "connect", connect_cmd, 0, "connect to the PBAP server, param: <address>" },
    { "disconnect", disconnect_cmd, 0, "disconnect from a PBAP server, param: <address>" },
    { "get", get_contact_cmd, 0, "get the contact by phone book, "
                                 "param: <address> <property> <property_value>, e.g.,\r\n"
                                 "\t\t\t  \"get <address> 0 peter\"\r\n"
                                 "\t\t\t  \"get <address> 1 10086\"\r\n"
                                 "\t\t\t  property example: 0 by name\r\n"
                                 "\t\t\t                    1 by number" },
    {"setblacklist", set_blacklist_cmd, 0, "set blacklist, param: <address>\r\n"},
    {"removeblacklist", remove_blacklist_cmd, 0, "remove blacklist, param: <address>\r\n"},
    {"checkblacklist", is_in_blacklist_cmd, 0, "check blacklist, param: <address>\r\n"},
    { "dump", dump_cmd, 0, "dump PBAP current state" },
};

static void* pce_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\t<address>: The format of a Bluetooth address should be like 00:01:02:03:04:05.\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_pce_tables); i++) {
        printf("\t%-8s\t%s\n", g_pce_tables[i].cmd, g_pce_tables[i].help);
    }
}

static void pce_connection_state_cb(void* cookie, bt_address_t* bd_addr,
    profile_connection_state_t state)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(bd_addr, addr_str);
    PRINT("%s, addr: %s, state: %d", __func__, addr_str, state);
}

static void pce_get_contact_end_cb(void* cookie, bt_status_t status,
    bt_pce_get_contact_req_type_t req_type, char* req_data, bt_pce_contact_t* contact)
{
    PRINT("%s, status: %d", __func__, status);
    switch (req_type) {
    case PCE_GET_CONTACT_BY_NAME:
        PRINT("search by name: %s", req_data);
        break;
    case PCE_GET_CONTACT_BY_NUMBER:
        PRINT("search by number: %s", req_data);
        break;
    default:
        break;
    }

    PRINT("contact: %s", contact->name);
    PRINT("number 1: %s", contact->numbers[0]);
    PRINT("number 2: %s", contact->numbers[1]);
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_pbap_pce_connect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("%s, address:%s", __func__, argv[0]);

    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_pbap_pce_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int get_contact_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    int type;

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s, object:%s", __func__, argv[0], argv[1]);

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    type = atoi(argv[1]);

    switch (type) {
    case RETRIVE_BY_NAME:
        if (bt_pbap_pce_get_contact_by_name(handle, &addr, argv[2]) != BT_STATUS_SUCCESS)
            return CMD_ERROR;
        break;
    case RETRIVE_BY_NUMBER:
        if (bt_pbap_pce_get_contact_by_number(handle, &addr, argv[2]) != BT_STATUS_SUCCESS)
            return CMD_ERROR;
        break;
    default:
        break;
    }

    return CMD_OK;
}

static int set_blacklist_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_pbap_pce_add_to_blacklist(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int remove_blacklist_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_pbap_pce_remove_from_blacklist(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int is_in_blacklist_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    bool is_blacklisted;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    is_blacklisted = bt_pbap_pce_is_in_blacklist(handle, &addr);

    if (is_blacklisted)
        PRINT("addr %s is in blacklist", argv[0]);
    else
        PRINT("addr %s is not in blacklist", argv[0]);

    return CMD_OK;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static const pbap_pce_callbacks_t pce_test_cbs = {
    sizeof(pbap_pce_callbacks_t),
    pce_connection_state_cb,
    pce_get_contact_end_cb,
};

int pce_command_init(void* handle)
{
    pce_callbacks = bt_pbap_pce_register_callbacks(handle, &pce_test_cbs);

    return 0;
}

void pce_command_uninit(void* handle)
{
    bt_pbap_pce_unregister_callbacks(handle, pce_callbacks);
}

int pce_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_pce_tables, ARRAY_SIZE(g_pce_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}