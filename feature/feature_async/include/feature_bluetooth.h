/*
 * Copyright (C) 2025 Xiaomi Corporation. All rights reserved.
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
 *
 */

#ifndef _FEATURE_BLUETOOTH_H_
#define _FEATURE_BLUETOOTH_H_
#include "bluetooth.h"
#include "bt_device.h"
#include "bt_list.h"
#include "bt_message_gattc.h"
#include "feature_exports.h"

#define FEATURE_MANAGER_BLUETOOTH_DATA "bluetooth"

#define FEATURE_BT_NO_RESOURCES 10013
#define FEATURE_BT_IPC_ERROR 10012
#define FEATURE_BT_UNKNOWN_ERROR 10008
#define FEATURE_BT_NOT_ENABLED 10001
#define FEATURE_BT_NOT_FOUND 10014

typedef enum {
    STATE_NON_SCAN = 0,
    STATE_SCANING = 1,
} ScanState;

typedef struct {
    FtCallbackId feature_callback_id;
    void* feature;
    void* data;
} callback_info_t;

typedef struct {
    FeatureInstanceHandle* feature_ins;

    //  There will be additional events related to subscribing to features in the future.
} feature_bluetooth_ins_t;

typedef struct {
    FeatureInstanceHandle* feature_ins;

    //  There will be additional events related to subscribing to features in the future.
} feature_bluetooth_ble_ins_t;

typedef struct {
    FtPromiseId pid;
    union {
        FeatureInstanceHandle feature_ins;
        FeatureInterfaceHandle interface;
    };
} feature_data_t;

typedef struct {
    bt_instance_t* ins;
    FeatureInterfaceHandle interface;
    void* adv;
    void* start_userdata;
    bool busy;
} feature_bluetooth_adv_info_t;

typedef struct {
    FtInt id;
    FtCallbackId callback;
    FtCallbackId fail;
} scan_subscribe_info_t;

typedef struct {
    bt_instance_t* ins;
    FeatureInterfaceHandle interface;
    void* scan;
    void* start_userdata;
    bool busy;
    bt_list_t* subscribe_info;
    FtInt subscribe_id;
} feature_bluetooth_scan_info_t;

typedef struct {
    gattc_handle_t handle;
    bt_address_t remote_address;
    ble_addr_type_t addr_type;
    connection_state_t conn_state;
    uint16_t gatt_mtu;
} gattc_t;

typedef enum {
    FEATURE_GATTC_CONN,
    FEATURE_GATTC_DISCONN,
    FEATURE_GATTC_DISCOVERY,
    FEATURE_GATTC_READ_CHAR,
    FEATURE_GATTC_READ_DESC,
    FEATURE_GATTC_WRITE_CHAR,
    FEATURE_GATTC_WRITE_DESC,
    FEATURE_GATTC_SET_MTU,
    FEATURE_GATTC_SET_NOTIFY,
} gattc_userdata_type_t;

typedef struct {
    FtPromiseId pid;
    gattc_userdata_type_t userdata_type;
    FeatureInterfaceHandle interface;
} gattc_data_t;

typedef struct {
    bool created;
    bt_instance_t* ins;
    FeatureInterfaceHandle interface;
    gattc_t* gattc;
    bt_list_t* userdata_list;
} feature_bluetooth_gattc_info_t;

typedef struct {
    bt_list_t* feature_ble_adv;
    bt_list_t* feature_ble_scan;
    bt_list_t* feature_ble_gattc;
} feature_bluetooth_features_info_t;

char* StringToFtString(const char* str);
bool js_event_cb_added();
void feature_bluetooth_post_task(FeatureInstanceHandle handle, FtCallbackId callback_id, void* data);
FeatureErrorCode bt_status_to_feature_error(uint8_t status);
void feature_bluetooth_init_bt_ins_async(FeatureProtoHandle handle);
void feature_bluetooth_uninit_bt_ins_async(void* data);
bt_instance_t* feature_bluetooth_get_bt_ins(FeatureInstanceHandle feature);
void feature_ble_list_free(void* data);
#endif // _FEATURE_BLUETOOTH_H_