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
#include <unistd.h>

#include "bluetooth.h"
#include "bt_config.h"
#include "bt_device.h"
#include "bt_gattc.h"
#include "bt_tools.h"

#define THROUGHTPUT_HORIZON 5

typedef struct {
    gattc_handle_t handle;
    bt_address_t remote_address;
    connection_state_t conn_state;
    uint16_t gatt_mtu;
} gattc_device_t;

static int create_cmd(void* handle, int argc, char* argv[]);
static int delete_cmd(void* handle, int argc, char* argv[]);
static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int discover_services_cmd(void* handle, int argc, char* argv[]);
static int read_request_cmd(void* handle, int argc, char* argv[]);
static int write_request_cmd(void* handle, int argc, char* argv[]);
static int enable_cccd_cmd(void* handle, int argc, char* argv[]);
static int disable_cccd_cmd(void* handle, int argc, char* argv[]);
static int exchange_mtu_cmd(void* handle, int argc, char* argv[]);
static int update_conn_cmd(void* handle, int argc, char* argv[]);
static int read_phy_cmd(void* handle, int argc, char* argv[]);
static int update_phy_cmd(void* handle, int argc, char* argv[]);
static int read_rssi_cmd(void* handle, int argc, char* argv[]);
static int throughput_cmd(void* handle, int argc, char* argv[]);

#define GATTC_CONNECTION_MAX (CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS)
static gattc_device_t g_gattc_devies[GATTC_CONNECTION_MAX];
static volatile uint32_t throughtput_cursor = 0;

#define CHECK_CONNCTION_ID(id)                           \
    {                                                    \
        if (id < 0 || id >= GATTC_CONNECTION_MAX) {      \
            PRINT("invalid connection id: %d", id);      \
            return CMD_INVALID_OPT;                      \
        }                                                \
        if (!g_gattc_devies[id].handle) {                \
            PRINT("connection[%d] is not created!", id); \
            return CMD_INVALID_OPT;                      \
        }                                                \
    }

static bt_command_t g_gattc_tables[] = {
    { "create", create_cmd, 0, "\"create gatt client :\"" },
    { "delete", delete_cmd, 0, "\"delete gatt client :<conn id>\"" },
    { "connect", connect_cmd, 0, "\"connect remote device :<conn id><address><addr type>\"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect remote device :<conn id>\"" },
    { "discover", discover_services_cmd, 0, "\"discover all services :<conn id>\"" },
    { "read_request", read_request_cmd, 0, "\"read request :<conn id><char id>\"" },
    { "write_request", write_request_cmd, 0, "\"write request :<conn id><char id><type>(str or hex)<playload>\n"
                                             "\t\t\t  e.g., write_request 0 0001 str HelloWorld!\n"
                                             "\t\t\t  e.g., write_request 0 0001 hex 00 01 02 03\"" },
    { "enable_cccd", enable_cccd_cmd, 0, "\"enable cccd(1: NOTIFY, 2: INDICATE) :<conn id><char id><ccc value>\"" },
    { "disable_cccd", disable_cccd_cmd, 0, "\"disable cccd :<conn id><char id>\"" },
    { "exchange_mtu", exchange_mtu_cmd, 0, "\"exchange mtu :<conn id><mtu>\"" },
    { "update_conn", update_conn_cmd, 0, "\"update connection parameter :<conn id><min_interval><max_interval><latency><timeout><min_connection_event_length><max_connection_event_length>\"" },
    { "read_phy", read_phy_cmd, 0, "\"read phy :<conn id>\"" },
    { "update_phy", update_phy_cmd, 0, "\"update phy(0: 1M, 1: 2M, 3: LE_Coded) :<conn id><tx><rx>\"" },
    { "read_rssi", read_rssi_cmd, 0, "\"read remote rssi :<conn id>\"" },
    { "throughput", throughput_cmd, 0, "\"throughput test :<conn id><char id><seconds>\"" },
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

static gattc_device_t* find_gattc_device(void* handle)
{
    for (int i = 0; i < GATTC_CONNECTION_MAX; i++) {
        if (g_gattc_devies[i].handle == handle)
            return &g_gattc_devies[i];
    }
    return NULL;
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    ble_addr_type_t addr_type = BT_LE_ADDR_TYPE_RANDOM;
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    bt_address_t addr;
    if (bt_addr_str2ba(argv[1], &addr) < 0)
        return CMD_INVALID_ADDR;

    addr_type = atoi(argv[2]);
    if (addr_type > BT_LE_ADDR_TYPE_ANONYMOUS || addr_type < BT_LE_ADDR_TYPE_PUBLIC) {
        return CMD_INVALID_OPT;
    }

    if (bt_gattc_connect(g_gattc_devies[conn_id].handle, &addr, addr_type) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (bt_gattc_disconnect(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int discover_services_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (bt_gattc_discover_service(g_gattc_devies[conn_id].handle, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int read_request_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint16_t attr_handle = strtol(argv[1], NULL, 16);

    if (bt_gattc_read(g_gattc_devies[conn_id].handle, attr_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int write_request_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    int len, i;
    uint8_t* value = NULL;
    CHECK_CONNCTION_ID(conn_id);

    uint16_t attr_handle = strtol(argv[1], NULL, 16);

    if (!strcmp(argv[2], "str")) {
        if (bt_gattc_write_without_response(g_gattc_devies[conn_id].handle, attr_handle,
                (uint8_t*)argv[3], strlen(argv[3]))
            != BT_STATUS_SUCCESS)
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
        if (bt_gattc_write_without_response(g_gattc_devies[conn_id].handle, attr_handle, value, len) != BT_STATUS_SUCCESS)
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

static int enable_cccd_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint16_t attr_handle = strtol(argv[1], NULL, 16);
    uint16_t ccc_value = atoi(argv[2]);

    if (bt_gattc_subscribe(g_gattc_devies[conn_id].handle, attr_handle, ccc_value) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disable_cccd_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint16_t attr_handle = strtol(argv[1], NULL, 16);

    if (bt_gattc_unsubscribe(g_gattc_devies[conn_id].handle, attr_handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int exchange_mtu_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint32_t mtu = atoi(argv[1]);

    if (bt_gattc_exchange_mtu(g_gattc_devies[conn_id].handle, mtu) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int update_conn_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 7)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    uint32_t min_interval = atoi(argv[1]);
    uint32_t max_interval = atoi(argv[2]);
    uint32_t latency = atoi(argv[3]);
    uint32_t timeout = atoi(argv[4]);
    uint32_t min_connection_event_length = atoi(argv[5]);
    uint32_t max_connection_event_length = atoi(argv[6]);

    if (bt_gattc_update_connection_parameter(g_gattc_devies[conn_id].handle, min_interval, max_interval, latency,
            timeout, min_connection_event_length, max_connection_event_length)
        != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int read_phy_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (bt_gattc_read_phy(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int update_phy_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    int tx = atoi(argv[1]);
    int rx = atoi(argv[2]);

    if (bt_gattc_update_phy(g_gattc_devies[conn_id].handle, tx, rx) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int read_rssi_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (bt_gattc_read_rssi(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int throughput_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    int32_t test_time = atoi(argv[2]);
    if (test_time <= 0)
        return CMD_INVALID_OPT;

    if (g_gattc_devies[conn_id].conn_state != CONNECTION_STATE_CONNECTED) {
        PRINT("connection[%d] is not connected to any device!", conn_id);
        return CMD_ERROR;
    }

    uint16_t attr_handle = strtol(argv[1], NULL, 16);

    uint32_t write_length = g_gattc_devies[conn_id].gatt_mtu;
    uint8_t* payload = (uint8_t*)malloc(sizeof(uint8_t) * write_length);
    if (!payload) {
        PRINT("write payload malloc failed");
        return CMD_ERROR;
    }

    int32_t run_time = 0;
    uint32_t write_count = 0;
    uint32_t bit_rate = 0;
    struct timespec start_ts;

    clock_gettime(CLOCK_BOOTTIME, &start_ts);
    throughtput_cursor = 0;

    PRINT("gattc write throughput test start, mtu = %" PRIu32 ", time = %" PRId32 "s.", write_length, test_time);
    while (1) {
        struct timespec current_ts;
        clock_gettime(CLOCK_BOOTTIME, &current_ts);

        if (run_time < (current_ts.tv_sec - start_ts.tv_sec)) {
            run_time = (current_ts.tv_sec - start_ts.tv_sec);
            bit_rate = write_length * write_count / run_time;
            PRINT("gattc write Bit rate = %" PRIu32 " Byte/s, = %" PRIu32 " bit/s, time = %" PRId32 "s.", bit_rate, bit_rate << 3, run_time);
        }

        if (run_time >= test_time || g_gattc_devies[conn_id].conn_state != CONNECTION_STATE_CONNECTED) {
            break;
        }

        if (throughtput_cursor >= THROUGHTPUT_HORIZON) {
            usleep(500);
            continue;
        }

        memset(payload, write_count & 0xFF, write_length);
        int ret = bt_gattc_write_without_response(g_gattc_devies[conn_id].handle, attr_handle, payload, write_length);
        if (ret != BT_STATUS_SUCCESS) {
            PRINT("write failed, ret: %d.", ret);
            break;
        }
        throughtput_cursor++;
        write_count++;
    }
    free(payload);

    if (run_time <= 0) {
        PRINT("gattc write throughput test failed due to an unexpected interruption!");
        return CMD_ERROR;
    }

    bit_rate = write_length * write_count / run_time;
    PRINT("gattc write throughput test finish, Bit rate = %" PRIu32 " Byte/s, = %" PRIu32 " bit/s, time = %" PRId32 "s.",
        bit_rate, bit_rate << 3, run_time);

    return CMD_OK;
}

static void connect_callback(void* conn_handle, bt_address_t* addr)
{
    gattc_device_t* device = find_gattc_device(conn_handle);

    assert(device);
    memcpy(&device->remote_address, addr, sizeof(bt_address_t));
    device->conn_state = CONNECTION_STATE_CONNECTED;
    PRINT_ADDR("gattc_connect_callback, addr:%s", addr);
}

static void disconnect_callback(void* conn_handle, bt_address_t* addr)
{
    gattc_device_t* device = find_gattc_device(conn_handle);

    assert(device);
    device->conn_state = CONNECTION_STATE_DISCONNECTED;
    PRINT_ADDR("gattc_disconnect_callback, addr:%s", addr);
}

static void discover_callback(void* conn_handle, gatt_status_t status, bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle)
{
    gatt_attr_desc_t attr_desc;

    if (status != GATT_STATUS_SUCCESS) {
        PRINT("gattc_discover_callback error %d", status);
        return;
    }

    if (!uuid || !uuid->type) {
        PRINT("gattc_discover_callback completed");
        return;
    }

    PRINT("gattc_discover_callback result, attr_handle: 0x%04x - 0x%04x", start_handle, end_handle);

    for (uint16_t attr_handle = start_handle; attr_handle <= end_handle; attr_handle++) {
        if (bt_gattc_get_attribute_by_handle(conn_handle, attr_handle, &attr_desc) != BT_STATUS_SUCCESS) {
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
        printf("[PROP:0x%04" PRIx32, attr_desc.properties);
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

        uint8_t* b_uuid = attr_desc.uuid.val.u128;
        printf("[0x%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x]\r\n",
            b_uuid[15], b_uuid[14], b_uuid[13], b_uuid[12],
            b_uuid[11], b_uuid[10], b_uuid[9], b_uuid[8],
            b_uuid[7], b_uuid[6], b_uuid[5], b_uuid[4],
            b_uuid[3], b_uuid[2], b_uuid[1], b_uuid[0]);
    }
    printf(">");
}

static void read_complete_callback(void* conn_handle, gatt_status_t status, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    PRINT("gattc connection read complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
    lib_dumpbuffer("read value:", value, length);
}

static void write_complete_callback(void* conn_handle, gatt_status_t status, uint16_t attr_handle)
{
    if (status != GATT_STATUS_SUCCESS) {
        PRINT("gattc connection write failed, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
        return;
    }

    if (throughtput_cursor) {
        throughtput_cursor--;
    } else {
        PRINT("gattc connection write complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
    }
}

static void subscribe_complete_callback(void* conn_handle, gatt_status_t status, uint16_t attr_handle, bool enable)
{
    PRINT("gattc connection subscribe complete, handle 0x%" PRIx16 ", status:%d, enable:%d", attr_handle, status, enable);
}

static void notify_received_callback(void* conn_handle, uint16_t attr_handle,
    uint8_t* value, uint16_t length)
{
    PRINT("gattc connection receive notify, handle 0x%" PRIx16, attr_handle);
    lib_dumpbuffer("notify value:", value, length);
}

static void mtu_updated_callback(void* conn_handle, gatt_status_t status, uint32_t mtu)
{
    gattc_device_t* device = find_gattc_device(conn_handle);

    assert(device);
    if (status == GATT_STATUS_SUCCESS) {
        device->gatt_mtu = mtu;
    }
    PRINT("gattc_mtu_updated_callback, status:%d, mtu:%" PRIu32, status, mtu);
}

static void phy_read_callback(void* conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    PRINT("gattc read phy complete, tx:%d, rx:%d", tx_phy, rx_phy);
}

static void phy_updated_callback(void* conn_handle, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    PRINT("gattc_phy_updated_callback, status:%d, tx:%d, rx:%d", status, tx_phy, rx_phy);
}

static void rssi_read_callback(void* conn_handle, gatt_status_t status, int32_t rssi)
{
    PRINT("gattc read rssi complete, status:%d, rssi:%" PRIi32, status, rssi);
}

static void conn_param_updated_callback(void* conn_handle, bt_status_t status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    PRINT("gattc connection paramter updated, status:%d, interval:%" PRIu16 ", latency:%" PRIu16 ", timeout:%" PRIu16,
        status, connection_interval, peripheral_latency, supervision_timeout);
}

static gattc_callbacks_t gattc_cbs = {
    sizeof(gattc_cbs),
    connect_callback,
    disconnect_callback,
    discover_callback,
    read_complete_callback,
    write_complete_callback,
    subscribe_complete_callback,
    notify_received_callback,
    mtu_updated_callback,
    phy_read_callback,
    phy_updated_callback,
    rssi_read_callback,
    conn_param_updated_callback,
};

static int create_cmd(void* handle, int argc, char* argv[])
{
    int conn_id;

    for (conn_id = 0; conn_id < GATTC_CONNECTION_MAX; conn_id++) {
        if (g_gattc_devies[conn_id].handle == NULL)
            break;
    }

    if (conn_id >= GATTC_CONNECTION_MAX) {
        PRINT("No unused connection id");
        return CMD_OK;
    }

    if (bt_gattc_create_connect(handle, &g_gattc_devies[conn_id].handle, &gattc_cbs) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("create connection success, conn_id: %d", conn_id);
    return CMD_OK;
}

static int delete_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int conn_id = atoi(argv[0]);
    CHECK_CONNCTION_ID(conn_id);

    if (bt_gattc_delete_connect(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("delete connection success, conn_id: %d", conn_id);
    return CMD_OK;
}

int gattc_command_init(void* handle)
{
    memset(g_gattc_devies, 0, sizeof(g_gattc_devies));
    return 0;
}

int gattc_command_uninit(void* handle)
{
    for (int i = 0; i < GATTC_CONNECTION_MAX; i++) {
        if (g_gattc_devies[i].handle) {
            bt_gattc_delete_connect(g_gattc_devies[i].handle);
        }
    }

    return 0;
}

int gattc_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_gattc_tables, ARRAY_SIZE(g_gattc_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
