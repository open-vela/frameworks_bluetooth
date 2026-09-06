/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bt_hid_host.h"
#include "bt_tools.h"

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int get_report_cmd(void* handle, int argc, char* argv[]);
static int set_report_cmd(void* handle, int argc, char* argv[]);
static int set_protocol_cmd(void* handle, int argc, char* argv[]);
static int suspend_cmd(void* handle, int argc, char* argv[]);
static int exit_suspend_cmd(void* handle, int argc, char* argv[]);
static int mode_cmd(void* handle, int argc, char* argv[]);
static int mouse_stats_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_hidh_tables[] = {
    { "connect", connect_cmd, 0, "\"connect HID device: <address>\"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect HID device: <address>\"" },
    { "get_report", get_report_cmd, 0, "\"get report: <address> <report_id> <report_type(1=IN,2=OUT,3=FEAT)>\"" },
    { "set_report", set_report_cmd, 0, "\"set report: <address> <report_id> <report_type> <hex_data>\"" },
    { "set_protocol", set_protocol_cmd, 0, "\"set protocol: <address> <mode(0=boot,1=report)>\"" },
    { "suspend", suspend_cmd, 0, "\"suspend: <address>\"" },
    { "exit_suspend", exit_suspend_cmd, 0, "\"exit suspend: <address>\"" },
    { "mode", mode_cmd, 0, "\"mode: <address> [default|sci] [low|medium|high|auto]\"" },
    { "mouse_stats", mouse_stats_cmd, 0, "\"mouse stats: <address> <on|off>\"" },
};

static void* hidh_callbacks = NULL;

/* Mouse stats */
static struct {
    bool enabled;
    uint32_t count;
    struct timespec last_ts;
    struct timespec period_start;
    uint32_t min_us;
    uint32_t max_us;
    uint64_t sum_us;
    uint8_t last_seq;
    uint32_t lost;
} g_mouse_stats;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_hidh_tables); i++) {
        printf("\t%-16s\t%s\n", g_hidh_tables[i].cmd, g_hidh_tables[i].help);
    }
}

/* ---- Callbacks ---- */

static void hidh_connection_state_cb(void* cookie, bt_address_t* addr,
    bt_transport_t transport, profile_connection_state_t state)
{
    PRINT_ADDR("HIDH conn state: %s, transport:%d, state:%d", addr, transport, state);
}

static void hidh_report_map_cb(void* cookie, bt_address_t* addr,
    uint8_t service_index, const uint8_t* data, uint16_t len)
{
    PRINT_ADDR("HIDH report map[%s]: svc=%u, %u bytes", addr, service_index, len);
    if (data && len > 0) {
        lib_dumpbuffer("report_map:", data, len);
    }
}

static void hidh_input_report_cb(void* cookie, bt_address_t* addr,
    uint8_t service_index, uint8_t report_id,
    const uint8_t* data, uint16_t len)
{
    if (g_mouse_stats.enabled && data && len >= 4) {
        struct timespec now;
        uint32_t delta_us;
        uint64_t elapsed_us;

        clock_gettime(CLOCK_MONOTONIC, &now);

        if (g_mouse_stats.count > 0) {
            uint8_t expected;

            delta_us = (uint32_t)((now.tv_sec - g_mouse_stats.last_ts.tv_sec) * 1000000 + (now.tv_nsec - g_mouse_stats.last_ts.tv_nsec) / 1000);
            if (delta_us < g_mouse_stats.min_us) {
                g_mouse_stats.min_us = delta_us;
            }

            if (delta_us > g_mouse_stats.max_us) {
                g_mouse_stats.max_us = delta_us;
            }

            g_mouse_stats.sum_us += delta_us;

            /* Check seq_num in data[3] (Wheel field) */
            expected = (g_mouse_stats.last_seq + 1) & 0xFF;
            if (data[3] != expected && g_mouse_stats.count > 1) {
                g_mouse_stats.lost++;
            }
        }

        g_mouse_stats.last_ts = now;
        g_mouse_stats.last_seq = data[3];
        g_mouse_stats.count++;

        /* Print stats every ~1 second */
        elapsed_us = (uint64_t)(now.tv_sec - g_mouse_stats.period_start.tv_sec) * 1000000 + (now.tv_nsec - g_mouse_stats.period_start.tv_nsec) / 1000;
        if (elapsed_us >= 1000000 && g_mouse_stats.count > 1) {
            uint32_t avg_us = (uint32_t)(g_mouse_stats.sum_us / (g_mouse_stats.count - 1));
            uint32_t rate = (uint32_t)((uint64_t)g_mouse_stats.count * 1000000 / elapsed_us);

            PRINT("[SCI-STATS] rate=%" PRIu32 "Hz avg=%" PRIu32 "us min=%" PRIu32 "us max=%" PRIu32 "us jitter=%" PRIu32 "us lost=%" PRIu32,
                rate, avg_us, g_mouse_stats.min_us, g_mouse_stats.max_us,
                g_mouse_stats.max_us - g_mouse_stats.min_us, g_mouse_stats.lost);

            /* Reset for next period */
            g_mouse_stats.count = 0;
            g_mouse_stats.sum_us = 0;
            g_mouse_stats.min_us = UINT32_MAX;
            g_mouse_stats.max_us = 0;
            g_mouse_stats.lost = 0;
            g_mouse_stats.period_start = now;
        }

        return;
    }

    PRINT_ADDR("HIDH input report: %s svc=%u id=%u len=%u", addr,
        service_index, report_id, len);
    if (data && len > 0) {
        lib_dumpbuffer("input_report:", data, len);
    }
}

static void hidh_get_report_cb(void* cookie, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type,
    const uint8_t* data, uint16_t len)
{
    PRINT_ADDR("HIDH get report: %s id=%u type=%u len=%u", addr,
        report_id, report_type, len);
    if (data && len > 0) {
        lib_dumpbuffer("get_report:", data, len);
    }
}

static void hidh_mode_changed_cb(void* cookie, bt_address_t* addr,
    uint8_t mode, int status)
{
    PRINT_ADDR("HIDH mode changed: %s mode=0x%02x status=%d", addr, mode, status);
}

static void hidh_pnp_id_cb(void* cookie, bt_address_t* addr,
    uint8_t vid_src, uint16_t vid, uint16_t pid, uint16_t version)
{
    PRINT_ADDR("HIDH PnP ID: %s vid_src=%u vid=0x%04x pid=0x%04x ver=0x%04x",
        addr, vid_src, vid, pid, version);
}

static void hidh_battery_level_cb(void* cookie, bt_address_t* addr,
    uint8_t bat_index, uint8_t level)
{
    PRINT_ADDR("HIDH battery: %s bat[%u]=%u%%", addr, bat_index, level);
}

static const hid_host_callbacks_t hidh_test_cbs = {
    sizeof(hid_host_callbacks_t),
    hidh_connection_state_cb,
    hidh_report_map_cb,
    hidh_input_report_cb,
    hidh_get_report_cb,
    hidh_pnp_id_cb,
    hidh_battery_level_cb,
    hidh_mode_changed_cb,
};

/* ---- Commands ---- */

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hid_host_connect(handle, &addr, BT_TRANSPORT_BLE) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("HIDH connect: %s", argv[0]);
    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hid_host_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("HIDH disconnect: %s", argv[0]);
    return CMD_OK;
}

static int get_report_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    uint8_t report_id = atoi(argv[1]);
    uint8_t report_type = atoi(argv[2]);

    if (bt_hid_host_get_report(handle, &addr, report_id, report_type) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int set_report_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint8_t data[64];

    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    uint8_t report_id = atoi(argv[1]);
    uint8_t report_type = atoi(argv[2]);
    int slen = strlen(argv[3]);

    if (slen % 2 != 0 || slen == 0)
        return CMD_INVALID_PARAM;

    int len = slen / 2;

    if (len > (int)sizeof(data))
        len = sizeof(data);

    for (int i = 0; i < len; i++) {
        char hex[3] = { argv[3][i * 2], argv[3][i * 2 + 1], 0 };
        data[i] = strtol(hex, NULL, 16);
    }

    if (bt_hid_host_set_report(handle, &addr, report_id, report_type, data, len) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int set_protocol_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    uint8_t mode = atoi(argv[1]);

    if (bt_hid_host_set_protocol(handle, &addr, mode) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int suspend_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hid_host_suspend(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int exit_suspend_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hid_host_exit_suspend(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

/* ---- Mode Commands ---- */

static int mode_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint8_t mode = BT_HID_HOST_MODE_DEFAULT;
    uint8_t level = BT_HID_HOST_LEVEL_HIGH;

    if (argc < 2) {
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        return CMD_INVALID_ADDR;
    }

    /* Parse level (optional last arg): low|medium|high */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "low") == 0)
            level = BT_HID_HOST_LEVEL_LOW;
        else if (strcmp(argv[i], "medium") == 0)
            level = BT_HID_HOST_LEVEL_MEDIUM;
        else if (strcmp(argv[i], "high") == 0)
            level = BT_HID_HOST_LEVEL_HIGH;
        else if (strcmp(argv[i], "auto") == 0)
            level = BT_HID_HOST_LEVEL_AUTO;
    }

    if (strcmp(argv[1], "default") == 0) {
        mode = BT_HID_HOST_MODE_DEFAULT;
    } else if (strcmp(argv[1], "sci") == 0) {
        mode = BT_HID_HOST_MODE_SCI;
    } else {
        PRINT("Unknown mode: %s (use default|sci)", argv[1]);
        return CMD_ERROR;
    }

    PRINT("HIDH mode: %s mode=0x%02x level=%u", argv[0], mode, level);
    if (bt_hid_host_set_mode(handle, &addr, mode, level) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mouse_stats_cmd(void* handle, int argc, char* argv[])
{
    (void)handle;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (strcmp(argv[1], "on") == 0) {
        memset(&g_mouse_stats, 0, sizeof(g_mouse_stats));
        g_mouse_stats.enabled = true;
        g_mouse_stats.min_us = UINT32_MAX;
        clock_gettime(CLOCK_MONOTONIC, &g_mouse_stats.period_start);
        PRINT("Mouse stats ON");
    } else {
        if (g_mouse_stats.enabled && g_mouse_stats.count > 1) {
            uint32_t avg_us = (uint32_t)(g_mouse_stats.sum_us / (g_mouse_stats.count - 1));
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            uint64_t elapsed_us = (uint64_t)(now.tv_sec - g_mouse_stats.period_start.tv_sec) * 1000000 + (now.tv_nsec - g_mouse_stats.period_start.tv_nsec) / 1000;
            uint32_t rate = elapsed_us > 0 ? (uint32_t)((uint64_t)g_mouse_stats.count * 1000000 / elapsed_us) : 0;
            PRINT("Mouse stats FINAL: rate=%" PRIu32 "/s avg=%" PRIu32 "us min=%" PRIu32 "us max=%" PRIu32 "us jitter=%" PRIu32 "us lost=%" PRIu32,
                rate, avg_us, g_mouse_stats.min_us, g_mouse_stats.max_us,
                g_mouse_stats.max_us - g_mouse_stats.min_us, g_mouse_stats.lost);
        }

        g_mouse_stats.enabled = false;
        PRINT("Mouse stats OFF");
    }

    return CMD_OK;
}

/* ---- Init/Uninit/Exec ---- */

int hidh_command_init(void* handle)
{
    hidh_callbacks = bt_hid_host_register_callbacks(handle, &hidh_test_cbs);
    return 0;
}

void hidh_command_uninit(void* handle)
{
    bt_hid_host_unregister_callbacks(handle, hidh_callbacks);
}

int hidh_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_hidh_tables, ARRAY_SIZE(g_hidh_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
