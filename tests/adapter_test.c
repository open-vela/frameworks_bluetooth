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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_adapter.h"
#include "utils.h"

static void on_adapter_state_changed_cb(void* cookie, bt_adapter_state_t state)
{
    printf("Context:%p, Adapter state changed: %d\n", cookie, state);
}

static void on_discovery_state_changed_cb(void* cookie, bt_discovery_state_t state)
{
    printf("Discovery state: %s\n", state == BT_DISCOVERY_STATE_STARTED ? "Started" : "Stopped");
}

static void on_discovery_result_cb(void* cookie, bt_discovery_result_t* result)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(&result->addr, addr_str);
    printf("Inquiring: device [%s], name: %s, cod: %08" PRIx32 ", rssi: %d\n", addr_str, result->name, result->cod, result->rssi);
}

static void on_scan_mode_changed_cb(void* cookie, bt_scan_mode_t mode)
{
    printf("Adapter new scan mode: %d\n", mode);
}

static void on_device_name_changed_cb(void* cookie, const char* device_name)
{
    printf("Adapter update device name: %s\n", device_name);
}

const static adapter_callbacks_t g_adapter_cbs = {
    .on_adapter_state_changed = on_adapter_state_changed_cb,
    .on_discovery_state_changed = on_discovery_state_changed_cb,
    .on_discovery_result = on_discovery_result_cb,
    .on_scan_mode_changed = on_scan_mode_changed_cb,
    .on_device_name_changed = on_device_name_changed_cb,
    .on_pair_request = NULL,
    .on_pair_display = NULL,
    .on_connection_state_changed = NULL,
    .on_bond_state_changed = NULL,
    .on_remote_name_changed = NULL,
    .on_remote_alias_changed = NULL,
    .on_remote_cod_changed = NULL,
    .on_remote_uuids_changed = NULL,
};

int main(int argc, char** argv)
{
    bt_instance_t* ins = NULL;
    static void* adapter_callback = NULL;

    ins = bluetooth_create_instance();
    if (ins == NULL) {
        printf("create instance error\n");
        return -1;
    }
    printf("create instance success\n");
    adapter_callback = bt_adapter_register_callback(ins, &g_adapter_cbs);
    if (adapter_callback == NULL) {
        bluetooth_delete_instance(ins);
        printf("register callback error\n");
        return -1;
    }
    printf("register callback success\n");
    bt_adapter_state_t state = bt_adapter_get_state(ins);
    printf("adapter state: %d\n", state);

    sleep(10);
    if (!bt_adapter_unregister_callback(ins, adapter_callback)) {
        printf("unregister callback error\n");
    } else {
        printf("unregister callback success\n");
    }
    bluetooth_delete_instance(ins);
    printf("btclient exited\n");

    return 0;
}
