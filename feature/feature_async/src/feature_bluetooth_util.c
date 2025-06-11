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

uint32_t g_created_features;

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

static void ipc_connected(bt_instance_t* ins, void* userdata)
{
    FEATURE_LOG_ERROR("ipc connected");
}

static void ipc_disconnected(bt_instance_t* ins, void* userdata, int status)
{
    FEATURE_LOG_ERROR("ipc disconnected");
}

void feature_bluetooth_init_bt_ins_async(feature_bluetooth_feature_type_t type, FeatureProtoHandle handle)
{
    bt_instance_t* bluetooth_ins;
    uv_loop_t* loop;
    FeatureManagerHandle manager;
    feature_bluetooth_features_info_t* features_info;

    if (!feature_bluetooth_using_feature()) {
        FeatureSetProtoData(handle, NULL);
        return;
    }

    manager = FeatureGetManagerHandleFromProto(handle);
    loop = FeatureGetUVLoop(manager);
    bluetooth_ins = bluetooth_get_async_instance(loop, ipc_connected, ipc_disconnected, NULL);

    if (bluetooth_ins == NULL) {
        FEATURE_LOG_ERROR("Failed to get Bluetooth instance.");
        return;
    }

    if (bluetooth_ins->context == NULL) {
        features_info = (feature_bluetooth_features_info_t*)calloc(1, sizeof(feature_bluetooth_features_info_t));
        assert(features_info);
        bluetooth_ins->context = features_info;
    }

    ((feature_bluetooth_features_info_t*)bluetooth_ins->context)->created_features |= (1UL << type);

    FeatureSetProtoData(handle, bluetooth_ins);
}

void feature_bluetooth_uninit_bt_ins_async(feature_bluetooth_feature_type_t type, FeatureProtoHandle handle)
{
    bt_instance_t* bluetooth_ins;
    feature_bluetooth_features_info_t* features_info;

    if (!feature_bluetooth_using_feature()) {
        return;
    }

    FeatureSetProtoData(handle, NULL);

    bluetooth_ins = bluetooth_find_async_instance(getpid());

    if (bluetooth_ins == NULL) {
        FEATURE_LOG_ERROR("Bluetooth instance not found.");
        return;
    }

    features_info = (feature_bluetooth_features_info_t*)bluetooth_ins->context;

    if (!features_info) {
        FEATURE_LOG_ERROR("Feature context not found.");
        return;
    }

    features_info->created_features &= ~(1UL << type);

    if (features_info->created_features) {
        return;
    }

    free(bluetooth_ins->context);
    bluetooth_ins->context = NULL;
    bluetooth_delete_async_instance(bluetooth_ins);
}

bt_instance_t* feature_bluetooth_get_bt_ins(FeatureInstanceHandle feature)
{
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    return FeatureGetProtoData(protoHandle);
}
