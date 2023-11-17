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

#include <stdlib.h>
#include <string.h>

#include "bt_pbap_pce.h"
#include "bt_tools.h"

#define PROPERTY_NAME_STR "Name"
#define PROPERTY_NUMBER_STR "Number"
#define PROPERTY_SOUND_STR "Sound"

#define CHECK_VCARD_PROPERTY(in, property_str) \
    (strlen(in) == strlen(property_str) && strncasecmp(in, property_str, strlen(property_str)) == 0)

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int change_directory_cmd(void* handle, int argc, char* argv[]);
static int pull_vcard_listing_cmd(void* handle, int argc, char* argv[]);
static int pull_vcard_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_pce_tables[] = {
    { "connect", connect_cmd, 0, "connect to the PBAP server, param: <address>" },
    { "disconnect", disconnect_cmd, 0, "disconnect from a PBAP server, param: <address>" },
    { "cd", change_directory_cmd, 0, "change directory, param: <address> <directory>, e.g.,\r\n"
                                     "\t\t\t  \"cd <address> telecom/pb\", or\r\n"
                                     "\t\t\t  \"cd <address> ..\" to the parent directory" },
    { "find", pull_vcard_listing_cmd, 0, "get the vCard Listing via SearchProperty, "
                                         "param: <address> <property> <value>, e.g.,\r\n"
                                         "\t\t\t  \"find <address> name Terry\", or\r\n"
                                         "\t\t\t  \"find <address> number 10086\", or\r\n"
                                         "\t\t\t  \"find <address>\" for the entire list" },
    { "get", pull_vcard_cmd, 0, "get the vCard Entry via vCard name, "
                                "param: <address> <name> <filter>, e.g.,\r\n"
                                "\t\t\t  \"get <address> 01.vcf\"\r\n"
                                "\t\t\t  filter example: 0000 - Show all properties\r\n"
                                "\t\t\t                  0087 - VERSION, N, FN and TEL" },
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

static void pce_dir_changed_cb(void* cookie, bt_address_t* bd_addr, uint16_t status)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(bd_addr, addr_str);
    PRINT("%s, addr: %s, status: %d", __func__, addr_str, status);
}

static void pce_vcard_listing_data_cb(void* cookie, bt_address_t* bd_addr, uint16_t len,
    char* data)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(bd_addr, addr_str);
    PRINT("%s, addr: %s, data len: %d", __func__, addr_str, len);
    for (int i = 0; i < len; i++)
        printf("%c", data[i]);
    printf("\r\n");
}

static void pce_vcard_listing_end_cb(void* cookie, bt_address_t* bd_addr, uint16_t status)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(bd_addr, addr_str);
    PRINT("%s, addr: %s, status: %d", __func__, addr_str, status);
}

static void pce_vcard_data_cb(void* cookie, bt_address_t* bd_addr, uint16_t len, char* data)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(bd_addr, addr_str);
    PRINT("%s, addr: %s, data len: %d", __func__, addr_str, len);
    for (int i = 0; i < len; i++)
        printf("%c", data[i]);
    printf("\r\n");
}

static void pce_vcard_end_cb(void* cookie, bt_address_t* bd_addr, uint16_t status)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(bd_addr, addr_str);
    PRINT("%s, addr: %s, status: %d", __func__, addr_str, status);
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

static int change_directory_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s, directory:%s", __func__, argv[0], argv[1]);

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_pbap_pce_change_directory(handle, &addr, argv[1]) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int pull_vcard_listing_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    pbap_search_property_t property;
    char* value;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s, property:%s, value:%s", __func__, argv[0], argv[1], argv[2]);

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (argc == 1) { /* no property specified */
        property = PCE_SEARCH_PROPERTY_NONE;
        value = NULL;
    } else { /* a property shall be selected */
        if (argc < 3)
            return CMD_PARAM_NOT_ENOUGH;
        if CHECK_VCARD_PROPERTY (argv[1], PROPERTY_NAME_STR) {
            property = PBAP_SEARCH_PROPERTY_NAME;
            value = argv[2];
        } else if CHECK_VCARD_PROPERTY (argv[1], PROPERTY_NUMBER_STR) {
            property = PBAP_SEARCH_PROPERTY_NUMBER;
            value = argv[2];
        } else if CHECK_VCARD_PROPERTY (argv[1], PROPERTY_SOUND_STR) {
            property = PBAP_SEARCH_PROPERTY_SOUND;
            value = argv[2];
        } else {
            return CMD_INVALID_PARAM;
        }
    }

    if (bt_pbap_pce_pull_vcard_listing(handle, &addr, property, value) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int pull_vcard_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint64_t filter = PCE_PROPERTY_MASK_ALL;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s, object:%s", __func__, argv[0], argv[1]);

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (argc >= 3)
        filter = strtoull(argv[2], NULL, 16);

    if (bt_pbap_pce_pull_vcard(handle, &addr, argv[1], filter) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static const pbap_pce_callbacks_t pce_test_cbs = {
    sizeof(pbap_pce_callbacks_t),
    pce_connection_state_cb,
    pce_dir_changed_cb,
    pce_vcard_listing_data_cb,
    pce_vcard_listing_end_cb,
    pce_vcard_data_cb,
    pce_vcard_end_cb,
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