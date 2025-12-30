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

#include "feature_bluetooth.h"
#include "feature_log.h"
#include <kvdb.h>

#define KVDB_USE_FEATURE "persist.using_bluetooth_feature"

char* StringToFtString(const char* str)
{
    if (!str) {
        return NULL;
    }
    int len = strlen(str);
    char* ftStr = (char*)FeatureMalloc(len + 1, FT_CHAR);
    strcpy(ftStr, str);
    return ftStr;
}

static bool feature_bluetooth_using_feature()
{
    static int using_bluetoothd_feature = -1;

    if (using_bluetoothd_feature == -1) {
        using_bluetoothd_feature = property_get_bool(KVDB_USE_FEATURE, 1);
    }

    return using_bluetoothd_feature;
}

void feature_ble_list_free(void* data)
{
    free(data);
}

static void feature_bluetooth_list_init(bt_instance_t* bt_ins)
{
    feature_bluetooth_features_info_t* features_info;
    if (!bt_ins) {
        return;
    }

    features_info = (feature_bluetooth_features_info_t*)calloc(1, sizeof(feature_bluetooth_features_info_t));

    assert(features_info);

    features_info->feature_ble_adv = bt_list_new(feature_ble_list_free);
    features_info->feature_ble_scan = bt_list_new(feature_ble_list_free);
    features_info->feature_ble_gattc = bt_list_new(feature_ble_list_free);
    bt_ins->context = features_info;
}

static void feature_bluetooth_list_uninit(bt_instance_t* bt_ins)
{
    feature_bluetooth_features_info_t* features_info;
    if (!bt_ins) {
        return;
    }

    features_info = (feature_bluetooth_features_info_t*)bt_ins->context;

    bt_list_free(features_info->feature_ble_adv);
    bt_list_free(features_info->feature_ble_scan);
    bt_list_free(features_info->feature_ble_gattc);
    free(bt_ins->context);
    bt_ins->context = NULL;
}

static void ipc_connected(bt_instance_t* ins, void* userdata)
{
    FEATURE_LOG_ERROR("ipc connected");
}

static void ipc_disconnected(bt_instance_t* ins, void* userdata, int status)
{
    FEATURE_LOG_ERROR("ipc disconnected");
}

void feature_bluetooth_init_bt_ins_async(FeatureProtoHandle handle)
{
    uv_loop_t* loop;
    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    void* data = FeatureGetManagerUserData(manager, FEATURE_MANAGER_BLUETOOTH_DATA);
    bt_instance_t* bluetooth_ins = (bt_instance_t*)data;

    if (!feature_bluetooth_using_feature()) {
        FeatureSetProtoData(handle, NULL);
        return;
    }

    if (bluetooth_ins) {
        FeatureSetProtoData(handle, bluetooth_ins);
        return;
    }

    loop = FeatureGetUVLoop(manager);
    bluetooth_ins = bluetooth_create_async_instance(loop, ipc_connected, ipc_disconnected, NULL);
    if (bluetooth_ins == NULL) {
        FEATURE_LOG_ERROR("Failed to get Bluetooth instance.");
        return;
    }

    FeatureSetManagerUserDataWithFreeCallback(manager, FEATURE_MANAGER_BLUETOOTH_DATA, bluetooth_ins, feature_bluetooth_uninit_bt_ins_async);
    feature_bluetooth_list_init(bluetooth_ins);

    FeatureSetProtoData(handle, bluetooth_ins);
}

void feature_bluetooth_uninit_bt_ins_async(void* data)
{
    bt_instance_t* bluetooth_ins = (bt_instance_t*)data;
    feature_bluetooth_features_info_t* features_info;

    if (!feature_bluetooth_using_feature()) {
        return;
    }

    features_info = (feature_bluetooth_features_info_t*)bluetooth_ins->context;

    if (!features_info) {
        FEATURE_LOG_ERROR("Feature context not found.");
        return;
    }

    feature_bluetooth_list_uninit(bluetooth_ins);
    bluetooth_delete_async_instance(bluetooth_ins);
}

bt_instance_t* feature_bluetooth_get_bt_ins(FeatureInstanceHandle feature)
{
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    return FeatureGetProtoData(protoHandle);
}

FeatureErrorCode bt_status_to_feature_error(uint8_t status)
{
    switch (status) {
    case BT_STATUS_FAIL:
        return FT_ERR_GENERAL;
    case BT_STATUS_NOMEM:
        return FT_ERR_GENERAL;
    case BT_STATUS_NOT_ENABLED:
        return FEATURE_BT_NOT_ENABLED;
    case BT_STATUS_DONE:
        return FT_ERR_DUPLICATE_SUBMISSION;
    case BT_STATUS_NOT_SUPPORTED:
        return FT_ERR_NOT_SUPPORTED;
    case BT_STATUS_NO_RESOURCES:
        return FEATURE_BT_NO_RESOURCES;
    case BT_STATUS_IPC_ERROR:
        return FEATURE_BT_IPC_ERROR;
    case BT_STATUS_DEVICE_NOT_FOUND:
        return FEATURE_BT_NOT_FOUND;
    case BT_STATUS_PARM_INVALID:
        return FT_ERR_ARGS;
    case BT_STATUS_NOT_FOUND:
        return FEATURE_BT_NOT_FOUND;
    case BT_STATUS_ERROR_BUT_UNKNOWN:
        return FEATURE_BT_UNKNOWN_ERROR;
    default:
        return FT_ERR_GENERAL;
    }
}