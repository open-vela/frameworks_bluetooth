/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "cm_dlf"

#include "connection_manager_dlf.h"
#include "bt_debug.h"
#include "bt_utils.h"
#include "hci_parser.h"
#include "sal_adapter_interface.h"
#include "service_loop.h"
#include "vendor/bt_vendor.h"
#include "vendor/bt_vendor_common.h"

// TBD
// Finding an appropriate timer duration for DLF, even dynamically setting it.
// Current Connection Parameters Setting: Interaval 60ms & Latency 4 and Interval 15ms & Latency 3
#define BT_LE_DLF_TIMEOUT_SLOTS 1600 // 1600 * 0.625 ms = 1000 ms

#define MAX_DLF_LINK_NUM CONFIG_BLUETOOTH_GATTS_MAX_CONNECTIONS // Must be less than or equal to 16

typedef struct
{
    bool is_enabled;
    bt_address_t peer_addr;
    hci_error_t status;
} cm_dlf_operation_t;

typedef struct
{
    bt_address_t peer_addr;
    uint16_t connection_handle;
    uint16_t timeout_slots;
} cm_dlf_link_t;

typedef struct {
    uint8_t dlf_link_num;
    cm_dlf_link_t dlf_links[MAX_DLF_LINK_NUM];
} cm_dlf_t;

static cm_dlf_t g_cm_dlf;

static cm_dlf_link_t* bt_cm_find_dlf_link(const bt_address_t* peer_addr)
{
    cm_dlf_t* manager = &g_cm_dlf;
    cm_dlf_link_t* dlf_link;
    uint8_t i;

    for (i = 0; i < MAX_DLF_LINK_NUM; i++) {
        dlf_link = &manager->dlf_links[i];

        if (!bt_addr_compare(&dlf_link->peer_addr, peer_addr))
            return dlf_link;
    }

    return NULL;
}

static void bt_cm_remove_dlf_link(cm_dlf_link_t* dlf_link)
{
    cm_dlf_t* manager = &g_cm_dlf;

    manager->dlf_link_num--;
    memset(dlf_link, 0, sizeof(cm_dlf_link_t));
}

static void bt_cm_process_dlf_result(void* context)
{
    cm_dlf_link_t* dlf_link;
    cm_dlf_operation_t* dlf_operation = (cm_dlf_operation_t*)context;

    BT_LOGI("DLF operation type: %d, command status: 0x%x", dlf_operation->is_enabled, dlf_operation->status);

    if (dlf_operation->is_enabled && (dlf_operation->status == HCI_SUCCESS)) {
        free(dlf_operation);
        return;
    }

    dlf_link = bt_cm_find_dlf_link(&dlf_operation->peer_addr);
    if (dlf_link)
        bt_cm_remove_dlf_link(dlf_link);

    free(dlf_operation);
}

static void bt_hci_event_callback(bt_hci_event_t* hci_event, void* context)
{
    cm_dlf_operation_t* dlf_operation = (cm_dlf_operation_t*)context;

    dlf_operation->status = hci_get_result(hci_event);
    do_in_service_loop(bt_cm_process_dlf_result, dlf_operation);
}

static bool bt_cm_build_dlf_command(le_dlf_config_t* dlf_config, uint8_t* data, size_t* size, bool is_enabled)
{
    if (is_enabled)
        return le_dlf_enable_builder(dlf_config, data, size);

    return le_dlf_disable_builder(dlf_config, data, size);
}

static bt_status_t bt_cm_send_dlf_command(cm_dlf_link_t* dlf_link, bool is_enabled)
{
    uint8_t ogf;
    uint16_t ocf;
    uint8_t len;
    uint8_t* payload;
    uint8_t temp_data[CONFIG_DLF_COMMAND_MAX_LEN];
    size_t size;
    le_dlf_config_t dlf_config;
    cm_dlf_operation_t* dlf_operation;

    dlf_operation = (cm_dlf_operation_t*)malloc(sizeof(cm_dlf_operation_t));
    if (!dlf_operation) {
        BT_LOGE("malloc failed");
        return BT_STATUS_FAIL;
    }

    dlf_operation->is_enabled = is_enabled;
    dlf_operation->peer_addr = dlf_link->peer_addr;
    dlf_config.connection_handle = dlf_link->connection_handle;
    dlf_config.dlf_timeout = dlf_link->timeout_slots;

    if (!bt_cm_build_dlf_command(&dlf_config, temp_data, &size, is_enabled)) {
        BT_LOGD("build dlf command %d failed", is_enabled);
        free(dlf_operation);
        return BT_STATUS_NOT_SUPPORTED;
    }

    payload = temp_data;
    len = size - sizeof(ogf) - sizeof(ocf);
    STREAM_TO_UINT8(ogf, payload);
    STREAM_TO_UINT16(ocf, payload);
    return bt_sal_send_hci_command(ogf, ocf, len, payload, bt_hci_event_callback, dlf_operation);
}

void bt_cm_dlf_cleanup(void)
{
    memset(&g_cm_dlf, 0, sizeof(g_cm_dlf));
}

bt_status_t bt_cm_enable_dlf(bt_address_t* peer_addr)
{
    cm_dlf_t* manager = &g_cm_dlf;
    uint16_t connection_handle;
    cm_dlf_link_t* dlf_link;
    bt_address_t empty_addr = { 0 };

    if (manager->dlf_link_num >= MAX_DLF_LINK_NUM)
        return BT_STATUS_NO_RESOURCES;

    dlf_link = bt_cm_find_dlf_link(peer_addr);
    if (dlf_link)
        return BT_STATUS_FAIL;

    dlf_link = bt_cm_find_dlf_link(&empty_addr);
    if (!dlf_link) {
        BT_LOGE("resource not found");
        return BT_STATUS_FAIL;
    }

    connection_handle = bt_sal_get_acl_link_handle(peer_addr, BT_TRANSPORT_BLE);
    if (connection_handle == BT_INVALID_CONNECTION_HANDLE)
        return BT_STATUS_PARM_INVALID;

    manager->dlf_link_num++;
    dlf_link->peer_addr = *peer_addr;
    dlf_link->connection_handle = connection_handle;
    dlf_link->timeout_slots = BT_LE_DLF_TIMEOUT_SLOTS;
    return bt_cm_send_dlf_command(dlf_link, true);
}

bt_status_t bt_cm_disable_dlf(bt_address_t* peer_addr)
{
    cm_dlf_link_t* dlf_link;

    dlf_link = bt_cm_find_dlf_link(peer_addr);
    if (!dlf_link)
        return BT_STATUS_SERVICE_NOT_FOUND;

    return bt_cm_send_dlf_command(dlf_link, false);
}
