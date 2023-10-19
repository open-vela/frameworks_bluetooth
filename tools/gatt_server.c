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

#include "ble_gatts.h"
#include "bluetooth.h"
#include "bt_tools.h"

static int register_cmd(void *handle, int argc, char *argv[]);
static int unregister_cmd(void *handle, int argc, char *argv[]);
static int start_cmd(void *handle, int argc, char *argv[]);
static int stop_cmd(void *handle, int argc, char *argv[]);
static int connect_cmd(void *handle, int argc, char *argv[]);
static int disconnect_cmd(void *handle, int argc, char *argv[]);
static int notify_bas_cmd(void *handle, int argc, char *argv[]);
static int notify_cus_cmd(void *handle, int argc, char *argv[]);
static int indicate_cus_cmd(void *handle, int argc, char *argv[]);

static gatts_handle_t g_dis_handle = NULL;
static gatts_handle_t g_bas_handle = NULL;
static gatts_handle_t g_custom_handle = NULL;

enum {
    GATT_SERVICE_DIS = 1,
    GATT_SERVICE_BAS = 2,
    GATT_SERVICE_CUSTOM = 3
};

#define GET_SERVICE_HANDLE(id, handle)           \
    {                                            \
        switch (id) {                            \
        case GATT_SERVICE_DIS:                   \
            handle = g_dis_handle;               \
            break;                               \
        case GATT_SERVICE_BAS:                   \
            handle = g_bas_handle;               \
            break;                               \
        case GATT_SERVICE_CUSTOM:                \
            handle = g_custom_handle;            \
            break;                               \
        default:                                 \
            PRINT("invalid service id: %d", id); \
            return CMD_INVALID_OPT;              \
        }                                        \
    }

enum {
    /* IDs of Device Information service */
    DIS_SERVICE_ID = 1,
    DIS_MODEL_NUMBER_CHR_ID,
    DIS_MANUFACTURER_NAME_CHR_ID,
    DIS_PNP_CHR_ID,
};

enum {
    /* IDs of Battery service */
    BAS_SERVICE_ID = 1,
    BAS_BATTERY_LEVEL_CHR_ID,
    BAS_BATTERY_LEVEL_CHR_CCC_ID,
};

enum {
    /* IDs of Private IOT service */
    IOT_SERVICE_ID = 1,
    IOT_SERVICE_TX_CHR_ID,
    IOT_SERVICE_TX_CHR_CCC_ID,
    IOT_SERVICE_RX_CHR_ID,
    IOT_SERVICE_READ_CHR_ID,
};

uint8_t read_char_value[] = { 'H', 'e', 'l', 'l', 'o', ' ', 'V', 'E', 'L', 'A', '!' };

uint16_t tx_char_ccc_changed(void *srv_handle, uint16_t attr_handle, const uint8_t *value, uint16_t length, uint16_t offset)
{
    printf("gatts service TX char ccc changed, new value:");
    for (int i = 0; i < length; i++) {
        printf("0x%02x ", value[i]);
    }
    printf("\n");
    return length;
}

uint16_t rx_char_on_read(void *srv_handle, uint16_t attr_handle, uint32_t req_handle)
{
    PRINT("gatts service RX char received read request.");
    bt_status_t ret = ble_gatts_response(srv_handle, req_handle, read_char_value, sizeof(read_char_value));
    PRINT("gatts service RX char response. status: %d", ret);
    return 0;
}

uint16_t rx_char_on_write(void *srv_handle, uint16_t attr_handle, const uint8_t *value, uint16_t length, uint16_t offset)
{
    printf("gatts service RX char received write request, value:");
    for (int i = 0; i < length; i++) {
        printf("0x%02x ", value[i]);
    }
    printf("\n");
    return length;
}

const char model_number_str[] = "vela_bt";
const char manufacturer_name_str[] = "xiaomi";
const uint8_t pnp_id[7] = {
    0x02, // pnp_vid_src
    0x8F, 0x03, // pnp_vid
    0x34, 0x12, // pnp_pid
    0x00, 0x01 // pnp_ver
};

static gatt_attr_db_t s_dis_attr_db[] = {
    GATT_H_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x180A), DIS_SERVICE_ID),
    GATT_H_CHARACTERISTIC_AUTO_RSP(BT_UUID_DECLARE_16(0x2A24), GATT_PROP_READ, GATT_PERM_READ, (uint8_t *)model_number_str, sizeof(model_number_str), DIS_MODEL_NUMBER_CHR_ID),
    GATT_H_CHARACTERISTIC_AUTO_RSP(BT_UUID_DECLARE_16(0x2A29), GATT_PROP_READ, GATT_PERM_READ, (uint8_t *)manufacturer_name_str, sizeof(manufacturer_name_str), DIS_MANUFACTURER_NAME_CHR_ID),
    GATT_H_CHARACTERISTIC_AUTO_RSP(BT_UUID_DECLARE_16(0x2A50), GATT_PROP_READ, GATT_PERM_READ, (uint8_t *)pnp_id, sizeof(pnp_id), DIS_PNP_CHR_ID),
};

static gatt_srv_db_t s_dis_service_db = {
    .attr_db = s_dis_attr_db,
    .attr_num = sizeof(s_dis_attr_db) / sizeof(gatt_attr_db_t),
};

static uint8_t battery_level = 100U;

static gatt_attr_db_t s_bas_attr_db[] = {
    GATT_H_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x180F), BAS_SERVICE_ID),
    GATT_H_CHARACTERISTIC_AUTO_RSP(BT_UUID_DECLARE_16(0x2A19), GATT_PROP_READ | GATT_PROP_NOTIFY, GATT_PERM_READ, &battery_level, sizeof(battery_level), BAS_BATTERY_LEVEL_CHR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, tx_char_ccc_changed, BAS_BATTERY_LEVEL_CHR_CCC_ID),
};

static gatt_srv_db_t s_bas_service_db = {
    .attr_db = s_bas_attr_db,
    .attr_num = sizeof(s_bas_attr_db) / sizeof(gatt_attr_db_t),
};

static gatt_attr_db_t s_iot_attr_db[] = {
    /* Private IOT Service - 0xFF00 */
    GATT_H_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0xFF00), IOT_SERVICE_ID),
    /* Private Characteristic for TX - 0xFF01 */
    GATT_H_CHARACTERISTIC_AUTO_RSP(BT_UUID_DECLARE_16(0xFF01), GATT_PROP_NOTIFY | GATT_PROP_INDICATE, 0, NULL, 0, IOT_SERVICE_TX_CHR_ID),
    /* Client Characteristic Configuration Descriptor - 0x2902 */
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, tx_char_ccc_changed, IOT_SERVICE_TX_CHR_CCC_ID),
    /* Private Characteristic for RX - 0xFF02 */
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(0xFF02), GATT_PROP_READ | GATT_PROP_WRITE_NR, GATT_PERM_READ | GATT_PERM_WRITE, rx_char_on_read, rx_char_on_write, IOT_SERVICE_RX_CHR_ID),
    /* Private Characteristic for read operation demo - 0xFF05 */
    GATT_H_CHARACTERISTIC_AUTO_RSP(BT_UUID_DECLARE_16(0xFF05), GATT_PROP_READ, GATT_PERM_READ, read_char_value, sizeof(read_char_value), IOT_SERVICE_READ_CHR_ID),
};

static gatt_srv_db_t s_iot_service_db = {
    .attr_db = s_iot_attr_db,
    .attr_num = sizeof(s_iot_attr_db) / sizeof(gatt_attr_db_t),
};

static bt_command_t g_gatts_tables[] = {
    {"register",         register_cmd,     0, "\"register gatt service(DIS = 1, BAS = 2, CUSTOM = 3) :<id>\""},
    { "unregister",      unregister_cmd,   0, "\"unregister gatt service :<id>\""                            },
    { "start",           start_cmd,        0, "\"start gatt service :<id>\""                                 },
    { "stop",            stop_cmd,         0, "\"stop gatt service :<id>\""                                  },
    { "connect",         connect_cmd,      0, "\"connect remote device :<id><address>\""                     },
    { "disconnect",      disconnect_cmd,   0, "\"disconnect remote device :<id>\""                           },
    { "notify_battery",  notify_bas_cmd,   0, "\"send battery notification :<level>(0-100)\""                },
    { "notify_custom",   notify_cus_cmd,   0, "\"send custom notification :<playload>\""                     },
    { "indicate_custom", indicate_cus_cmd, 0, "\"send custom indication   :<playload>\""                     },
};

static struct option gatts_options[] = {
    {"help", 0, 0, 'h'},
    { 0,     0, 0, 0  }
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_gatts_tables); i++) {
        printf("\t%-8s\t%s\n", g_gatts_tables[i].cmd, g_gatts_tables[i].help);
    }
}

static int connect_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[1], &addr) < 0)
        return CMD_INVALID_ADDR;

    gatts_handle_t service_handle;
    int service_id = atoi(argv[0]);
    GET_SERVICE_HANDLE(service_id, service_handle)

    if (ble_gatts_connect(service_handle, &addr, BT_LE_ADDR_TYPE_UNKNOWN) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    gatts_handle_t service_handle;
    int service_id = atoi(argv[0]);
    GET_SERVICE_HANDLE(service_id, service_handle)

    if (ble_gatts_disconnect(service_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int start_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    gatts_handle_t service_handle;
    gatt_srv_db_t *service_db;
    int service_id = atoi(argv[0]);
    switch (service_id) {
    case GATT_SERVICE_DIS:
        service_handle = g_dis_handle;
        service_db = &s_dis_service_db;
        break;
    case GATT_SERVICE_BAS:
        service_handle = g_bas_handle;
        service_db = &s_bas_service_db;
        break;
    case GATT_SERVICE_CUSTOM:
        service_handle = g_custom_handle;
        service_db = &s_iot_service_db;
        break;
    default:
        PRINT("invalid service id: %d", service_id);
        return CMD_INVALID_OPT;
    }

    if (ble_gatts_create_service_table(service_handle, service_db) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    if (ble_gatts_start(service_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int stop_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    gatts_handle_t service_handle;
    int service_id = atoi(argv[0]);
    GET_SERVICE_HANDLE(service_id, service_handle)

    if (ble_gatts_stop(service_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void notify_complete_cb(void *srv_handle, gatt_status_t status, uint16_t attr_handle)
{
    PRINT("gatts service notify complete, handle 0x%04x status:%d", attr_handle, status);
}

static int notify_bas_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int new_level = atoi(argv[0]);
    if (new_level < 0 || new_level > 100) {
        PRINT("invalid battery level: %d", new_level);
        return CMD_INVALID_OPT;
    }

    battery_level = new_level;
    if (ble_gatts_notify(g_bas_handle, BAS_BATTERY_LEVEL_CHR_ID, (uint8_t *)&battery_level, sizeof(battery_level), notify_complete_cb) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int notify_cus_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (ble_gatts_notify(g_custom_handle, IOT_SERVICE_TX_CHR_ID, (uint8_t *)argv[0], strlen(argv[0]), notify_complete_cb) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void indicate_complete_cb(void *srv_handle, gatt_status_t status, uint16_t attr_handle)
{
    PRINT("gatts service indicate complete, status:%d", status);
}

static int indicate_cus_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (ble_gatts_indicate(g_custom_handle, IOT_SERVICE_TX_CHR_ID, (uint8_t *)argv[0], strlen(argv[0]), indicate_complete_cb) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void connect_callback(void *srv_handle, bt_address_t *addr)
{
    PRINT_ADDR("gatts_connect_callback, addr:%s", addr);
}

static void disconnect_callback(void *srv_handle, bt_address_t *addr, uint8_t reason)
{
    PRINT_ADDR("gatts_disconnect_callback, addr:%s, reason:%d", addr, reason);
}

static void start_callback(void *srv_handle, gatt_status_t status)
{
    PRINT("gatts service start complete, status:%d", status);
}

static void stop_callback(void *srv_handle, gatt_status_t status)
{
    PRINT("gatts service stop complete, status:%d", status);
}

static void mtu_change_callback(void *srv_handle, bt_address_t *addr, uint32_t mtu)
{
    PRINT_ADDR("gatts_mtu_change_callback, addr:%s, mtu:%d", addr, mtu);
}

static gatts_callbacks_t gatts_cbs = {
    sizeof(gatts_cbs),
    connect_callback,
    disconnect_callback,
    start_callback,
    stop_callback,
    mtu_change_callback,
};

static int register_cmd(void *handle, int argc, char *argv[])
{
    int ret = 0;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int service_id = atoi(argv[0]);
    switch (service_id) {
    case GATT_SERVICE_DIS:
        if (g_dis_handle) {
            PRINT("dis has registed, please unregister then try again");
            return CMD_OK;
        }
        ret = ble_gatts_register_service(handle, &g_dis_handle, &gatts_cbs);
        break;
    case GATT_SERVICE_BAS:
        if (g_bas_handle) {
            PRINT("bas has registed, please unregister then try again");
            return CMD_OK;
        }
        ret = ble_gatts_register_service(handle, &g_bas_handle, &gatts_cbs);
        break;
    case GATT_SERVICE_CUSTOM:
        if (g_custom_handle) {
            PRINT("custom service has registed, please unregister then try again");
            return CMD_OK;
        }
        ret = ble_gatts_register_service(handle, &g_custom_handle, &gatts_cbs);
        break;
    default:
        PRINT("invalid service id: %d", service_id);
        return CMD_INVALID_OPT;
    }

    if (ret != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("register service successful, service_id: %d", service_id);

    return CMD_OK;
}

static int unregister_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    gatts_handle_t service_handle;
    int service_id = atoi(argv[0]);
    GET_SERVICE_HANDLE(service_id, service_handle)

    if (ble_gatts_unregister_service(service_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("unregister service succed, service_id: %d", service_id);
    return CMD_OK;
}

int gatts_command_init(void *handle)
{
    return 0;
}

int gatts_command_uninit(void *handle)
{
    return 0;
}

int gatts_command_exec(void *handle, int argc, char *argv[])
{
    int opt, ret = CMD_USAGE_FAULT;

    while ((opt = getopt_long(argc, argv, "h", gatts_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return CMD_OK;
        default:
            break;
        }
    }

    if (argc > 0)
        ret = execute_command_in_table(handle, g_gatts_tables, ARRAY_SIZE(g_gatts_tables), argc, argv);

    if (ret < 0) {
        printf("Erroneous command %s\n", argv[0]);
        usage();
    }

    return ret;
}
