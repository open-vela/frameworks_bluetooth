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

#include "ble_gattc.h"
#include "bluetooth.h"
#include "bt_tools.h"

static int create_cmd(void *handle, int argc, char *argv[]);
static int delete_cmd(void *handle, int argc, char *argv[]);
static int connect_cmd(void *handle, int argc, char *argv[]);
static int disconnect_cmd(void *handle, int argc, char *argv[]);
static int discover_services_cmd(void *handle, int argc, char *argv[]);
static int read_request_cmd(void *handle, int argc, char *argv[]);
static int write_request_cmd(void *handle, int argc, char *argv[]);
static int enable_cccd_cmd(void *handle, int argc, char *argv[]);
static int disable_cccd_cmd(void *handle, int argc, char *argv[]);
static int exchange_mtu_cmd(void *handle, int argc, char *argv[]);

#define GATTC_CONNECTION_MAX (CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS)
static gattc_handle_t g_gattc_handles[GATTC_CONNECTION_MAX] = { 0 };

#define CHECK_CONNCTION_ID(id)                      \
    {                                               \
        if (id < 0 || id >= GATTC_CONNECTION_MAX) { \
            PRINT("invalid connection id: %d", id); \
            return CMD_INVALID_OPT;                 \
        }                                           \
    }

static bt_command_t g_gattc_tables[] = {
    { "create",        create_cmd,            0, "\"create gatt client :\""                                         },
    { "delete",        delete_cmd,            0, "\"delete gatt client :<conn id>\""                                },
    { "connect",       connect_cmd,           0, "\"connect remote device :<conn id><address>\""                    },
    { "disconnect",    disconnect_cmd,        0, "\"disconnect remote device :<conn id>\""                          },
    { "discover",      discover_services_cmd, 0, "\"discover all services :<conn id>\""                             },
    { "read_request",  read_request_cmd,      0, "\"read request :<conn id><char id>\""                             },
    { "write_request", write_request_cmd,     0, "\"write request :<conn id><char id><type>(str or hex)<playload>\n"
                                            "\t\t\t  e.g., write_request 0 0001 str HelloWorld!\n"
                                            "\t\t\t  e.g., write_request 0 0001 hex 00 01 02 03\n"                  },
    { "enable_cccd",   enable_cccd_cmd,       0, "\"enable cccd :<conn id><char id><cccd_id>\""                     },
    { "disable_cccd",  disable_cccd_cmd,      0, "\"disable cccd :<conn id><char id><cccd_id>\""                    },
    { "exchange_mtu",  exchange_mtu_cmd,      0, "\"exchange mtu :<conn id><mtu>\""                                 },
};

static struct option gattc_options[] = {
    {"help", 0, 0, 'h'},
    { 0,     0, 0, 0  }
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_gattc_tables); i++) {
        printf("\t%-8s\t%s\n", g_gattc_tables[i].cmd, g_gattc_tables[i].help);
    }
}

static int connect_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    bt_address_t addr;
    if (bt_addr_str2ba(argv[1], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (ble_gattc_connect(g_gattc_handles[conn_id], &addr, BT_LE_ADDR_TYPE_UNKNOWN) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (ble_gattc_disconnect(g_gattc_handles[conn_id]) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int discover_services_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (ble_gattc_discover_service(g_gattc_handles[conn_id], NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int read_request_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint16_t attr_handle = strtol(argv[1], NULL, 16);

    if (ble_gattc_read(g_gattc_handles[conn_id], attr_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int write_request_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    int len, i;
    uint8_t *value = NULL;
    CHECK_CONNCTION_ID(conn_id);

    uint16_t attr_handle = strtol(argv[1], NULL, 16);

    if (!strcmp(argv[2], "str")) {
        if (ble_gattc_write_without_response(g_gattc_handles[conn_id], attr_handle,
            (uint8_t*)argv[3], strlen(argv[3])) != BT_STATUS_SUCCESS)
            return CMD_ERROR;
    } else if (!strcmp(argv[2], "hex")) {
        len = argc - 3;
        if (len <= 0 || len > 0xFFFF)
            return CMD_USAGE_FAULT;

        value = malloc(len);
        if (!value)
            return CMD_ERROR;

        for (i = 0; i < len; i++)
            value[i] = (uint8_t)(strtol(argv[3 + i], NULL, 16) & 0xFF);
        if (ble_gattc_write_without_response(g_gattc_handles[conn_id], attr_handle, value, len) != BT_STATUS_SUCCESS)
            goto error;
    } else
        return CMD_INVALID_PARAM;

    if (value)
        free(value);

    return CMD_OK;
error:
    if (value)
        free(value);
    return CMD_ERROR;
}

static void notify_received_cb(void *conn_handle, uint16_t attr_handle,
                               uint8_t *value, uint16_t length)
{
    PRINT("gattc connection receive notify, handle 0x%04x:", attr_handle);
    for (int i = 0; i < length; i++) {
        printf("0x%02x ", value[i]);
    }
    printf("\n");
}

static int enable_cccd_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint16_t value_handle = strtol(argv[1], NULL, 16);
    uint16_t cccd_handle = 0;
    if (argc > 2)
        cccd_handle = strtol(argv[2], NULL, 16);

    if (ble_gattc_subscribe(g_gattc_handles[conn_id], value_handle, cccd_handle, notify_received_cb) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disable_cccd_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint16_t value_handle = strtol(argv[1], NULL, 16);
    uint16_t cccd_handle = 0;
    if (argc > 2)
        cccd_handle = strtol(argv[2], NULL, 16);

    if (ble_gattc_unsubscribe(g_gattc_handles[conn_id], value_handle, cccd_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int exchange_mtu_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    int mtu = atoi(argv[1]);

    if (ble_gattc_exchange_mtu(g_gattc_handles[conn_id], mtu) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void connect_callback(void *conn_handle, bt_address_t *addr)
{
    PRINT_ADDR("gattc_connect_callback, addr:%s", addr);
}

static void disconnect_callback(void *conn_handle, bt_address_t *addr, uint8_t reason)
{
    PRINT_ADDR("gattc_disconnect_callback, addr:%s, reason:%d", addr, reason);
}

static void discover_callback(void *conn_handle, gatt_status_t status, bt_uuid_t *uuid, uint16_t start_handle, uint16_t end_handle)
{
    gatt_attr_desc_t attr_desc;

    if (status != GATT_STATUS_SUCCESS) {
        PRINT("gattc_discover_callback error %d", status);
        return;
    }

    if (!uuid) {
        PRINT("gattc_discover_callback completed");
        return;
    }

    PRINT("gattc_discover_callback result, attr_handle: 0x%04x - 0x%04x", start_handle, end_handle);

    for (uint16_t attr_handle = start_handle; attr_handle <= end_handle; attr_handle++) {
        if (ble_gattc_get_attribute_by_handle(conn_handle, attr_handle, &attr_desc) != BT_STATUS_SUCCESS) {
            continue;
        }

        switch (attr_desc.type) {
        case GATT_PRIMARY_SERVICE:
            printf(">[0x%04x][PRI]", attr_desc.handle);
            break;
        case GATT_SECONDARY_SERVICE:
            printf(">[0x%04x][SND]", attr_desc.handle);
            break;
        case GATT_INCLUDED_SERVICE:
            printf(">  [0x%04x][INC]", attr_desc.handle);
            break;
        case GATT_CHARACTERISTIC:
            printf(">  [0x%04x][CHR]", attr_desc.handle);
            break;
        case GATT_DESCRIPTOR:
            printf(">    [0x%04x][DES]", attr_desc.handle);
            break;
        }
        printf("[PROP:0x%04x", attr_desc.properties);
        if (attr_desc.properties) {
            printf(",");
            if (attr_desc.properties & GATT_PROP_READ) {
                printf("R");
            }
            if (attr_desc.properties & GATT_PROP_WRITE_NR) {
                printf("Wn");
            }
            if (attr_desc.properties & GATT_PROP_WRITE) {
                printf("W");
            }
            if (attr_desc.properties & GATT_PROP_NOTIFY) {
                printf("N");
            }
            if (attr_desc.properties & GATT_PROP_INDICATE) {
                printf("I");
            }
        }
        printf("]");

        uint8_t *b_uuid = attr_desc.uuid.val.u128;
        printf("[0x%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x]\r\n",
               b_uuid[15], b_uuid[14], b_uuid[13], b_uuid[12],
               b_uuid[11], b_uuid[10], b_uuid[9], b_uuid[8],
               b_uuid[7], b_uuid[6], b_uuid[5], b_uuid[4],
               b_uuid[3], b_uuid[2], b_uuid[1], b_uuid[0]);
    }
    printf(">");
}

static void mtu_exchange_callback(void *conn_handle, gatt_status_t status, uint32_t mtu)
{
    PRINT("gattc_mtu_exchange_callback, status:%d, mtu:%d", status, mtu);
}

static void read_complete_callback(void *conn_handle, gatt_status_t status, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    PRINT("gattc connection read complete, handle 0x%04x status:%d", attr_handle, status);
    for (int i = 0; i < length; i++) {
        printf("0x%02x ", value[i]);
    }
    printf("\n");
}

static void write_complete_callback(void *conn_handle, gatt_status_t status, uint16_t attr_handle, uint16_t offset)
{
    PRINT("gattc connection write complete, handle 0x%04x status:%d", attr_handle, status);
}

static gattc_callbacks_t gattc_cbs = {
    sizeof(gattc_cbs),
    connect_callback,
    disconnect_callback,
    discover_callback,
    read_complete_callback,
    write_complete_callback,
    mtu_exchange_callback,
};

static int create_cmd(void *handle, int argc, char *argv[])
{
    int conn_id;

    for (conn_id = 0; conn_id < GATTC_CONNECTION_MAX; conn_id++) {
        if (g_gattc_handles[conn_id] == NULL)
            break;
    }

    if (conn_id >= GATTC_CONNECTION_MAX) {
        PRINT("No unused connection id");
        return CMD_OK;
    }

    if (ble_gattc_create_connect(handle, &g_gattc_handles[conn_id], &gattc_cbs) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("create connection successful, conn_id: %d", conn_id);
    return CMD_OK;
}

static int delete_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (ble_gattc_delete_connect(g_gattc_handles[conn_id]) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("delete connection successful, conn_id: %d", conn_id);
    return CMD_OK;
}

int gattc_command_init(void *handle)
{
    return 0;
}

int gattc_command_uninit(void *handle)
{
    return 0;
}

int gattc_command_exec(void *handle, int argc, char *argv[])
{
    int opt, ret = CMD_USAGE_FAULT;

    while ((opt = getopt_long(argc, argv, "h", gattc_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return CMD_OK;
        default:
            break;
        }
    }

    if (argc > 0)
        ret = execute_command_in_table(handle, g_gattc_tables, ARRAY_SIZE(g_gattc_tables), argc, argv);

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[0]);
        usage();
    }

    return ret;
}
