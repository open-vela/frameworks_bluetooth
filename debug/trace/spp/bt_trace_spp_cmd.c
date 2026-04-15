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
#include "bt_tools.h"
#include "trace/spp/bt_trace_spp.h"

static int dir_cmd(void* handle, int argc, char* argv[])
{
    uint8_t mask;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (!strcmp(argv[0], "tx"))
        mask = BT_TRACE_DIR_TX;
    else if (!strcmp(argv[0], "rx"))
        mask = BT_TRACE_DIR_RX;
    else if (!strcmp(argv[0], "all"))
        mask = BT_TRACE_DIR_ALL;
    else
        return CMD_INVALID_PARAM;

    bt_trace_spp_set_direction(mask);
    PRINT("spp trace direction: %s", argv[0]);
    return CMD_OK;
}

static int layer_cmd(void* handle, int argc, char* argv[])
{
    uint8_t mask = 0;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "hci"))
            mask |= BT_TRACE_LAYER_HCI;
        else if (!strcmp(argv[i], "rfcomm"))
            mask |= BT_TRACE_LAYER_RFCOMM;
        else if (!strcmp(argv[i], "spp"))
            mask |= BT_TRACE_LAYER_SPP;
        else if (!strcmp(argv[i], "all"))
            mask = BT_TRACE_LAYER_ALL;
        else
            return CMD_INVALID_PARAM;
    }

    bt_trace_spp_set_layer(mask);
    PRINT("spp trace layer: 0x%02x", mask);
    return CMD_OK;
}

/*
 * port <port>
 *
 * Filter SPP-layer probes by the port (conn_id) value reported in
 * spp_connection_state_callback. Note: port=0 is a valid value.
 * Read from SPP service log, e.g.:
 *   "spp_connection_state_cb, scn: 5, port: 0"
 */
static int port_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint16_t port = (uint16_t)strtoul(argv[0], NULL, 0);

    bt_trace_spp_set_conn_port(port);
    PRINT("spp trace port: port=%u (filter enabled)", port);
    return CMD_OK;
}

/* acl <acl_handle> */
static int acl_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint16_t acl_handle = (uint16_t)strtoul(argv[0], NULL, 0);

    bt_trace_spp_set_acl_handle(acl_handle);
    PRINT("spp trace acl: acl_handle=0x%04x(%u)", acl_handle, acl_handle);
    return CMD_OK;
}

/* dlci <value> — filter HCI/RFCOMM layers by exact DLCI value */
static int dlci_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint16_t dlci = (uint16_t)strtoul(argv[0], NULL, 0);
    if (dlci < 2 || dlci > 61) {
        PRINT("dlci must be in range 2-61 (= scn<<1 | direction_bit)");
        return CMD_INVALID_PARAM;
    }

    bt_trace_spp_set_dlci((uint8_t)dlci);
    PRINT("spp trace dlci: dlci=0x%02x(%u)", (uint8_t)dlci, (uint8_t)dlci);
    return CMD_OK;
}

static int reset_cmd(void* handle, int argc, char* argv[])
{
    bt_trace_spp_filter_reset();
    PRINT("spp trace filters reset");
    return CMD_OK;
}

static bt_command_t g_trace_spp_tables[] = {
    { "dir", dir_cmd, 0, "<tx|rx|all> — set direction filter" },
    { "layer", layer_cmd, 0, "<hci|rfcomm|spp|all> ... — set layer filter" },
    { "port", port_cmd, 0, "<port> — set SPP port filter (SPP layer), read from spp_connection_state_cb log" },
    { "dlci", dlci_cmd, 0, "<dlci> — set RFCOMM DLCI filter (HCI/RFCOMM layers), e.g. 10 or 11" },
    { "acl", acl_cmd, 0, "<acl_handle> — set ACL handle filter" },
    { "reset", reset_cmd, 0, "reset all filters" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\ttrace spp <command> [parameters]\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_trace_spp_tables); i++) {
        printf("\t%-8s\t%s\n", g_trace_spp_tables[i].cmd,
            g_trace_spp_tables[i].help);
    }
}

int trace_spp_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_trace_spp_tables,
            ARRAY_SIZE(g_trace_spp_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
