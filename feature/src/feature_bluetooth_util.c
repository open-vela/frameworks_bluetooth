/*
 * Copyright (C) 2024 Xiaomi Corporation. All rights reserved.
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

static void free_callback_info(callback_info_t* info)
{
    if (info->data)
        FeatureFreeValue(info->data);

    free(info);
}

void feature_bluetooth_deal_callback(int status, void* data)
{
    callback_info_t* info = (callback_info_t*)data;
    FEATURE_LOG_DEBUG("callback type:%d, feature:%p, callback id: %d", info->callback_id, info->feature, info->feature_callback_id);
    if (!FeatureCheckCallbackId(info->feature, info->feature_callback_id)) {
        goto freeData;
    }

    if (!FeatureInvokeCallback(info->feature, info->feature_callback_id, info->data)) {
        FEATURE_LOG_ERROR("feature:%p, callback id: %d, invoke discoveryresult callback failed!",
            info->feature, info->feature_callback_id);
    }

freeData:
    free_callback_info(info);
}

void feature_bluetooth_remove_callback(int status, void* data)
{
    callback_info_t* info = (callback_info_t*)data;
    FEATURE_LOG_DEBUG("remove callback, feature:%p, callback id: %d", info->feature, info->feature_callback_id);
    FeatureRemoveCallback(info->feature, info->feature_callback_id);

    free_callback_info(info);
}

void feature_bluetooth_post_task(FeatureInstanceHandle handle, FtCallbackId callback_id, void* data)
{
    callback_info_t* callback_info;

    callback_info = (callback_info_t*)calloc(1, sizeof(callback_info_t));
    if (!callback_info) {
        if (data)
            FeatureFreeValue(data);
        return;
    }

    callback_info->feature_callback_id = callback_id;
    callback_info->feature = handle;
    callback_info->data = data;
    if (!FeaturePost(handle, feature_bluetooth_deal_callback, callback_info)) {
        FEATURE_LOG_WARN("feature:%p, callback id: %d, post callback failed!", handle, callback_id);
        free_callback_info(callback_info);
    }
}

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

void feature_bluetooth_init_bt_ins(feature_bluetooth_feature_type_t feature, FeatureProtoHandle handle)
{
    bt_instance_t* bluetooth_ins;

    if (!feature_bluetooth_using_feature()) {
        FeatureSetProtoData(handle, NULL);
        return;
    }

    bluetooth_ins = bluetooth_get_instance();

    if (bluetooth_ins == NULL) {
        FEATURE_LOG_ERROR("Failed to get Bluetooth instance.");
        return;
    }

    if (bluetooth_ins->context == NULL) {
        feature_bluetooth_callback_init(bluetooth_ins);
    }

    ((feature_bluetooth_features_info_t*)bluetooth_ins->context)->created_features |= (1UL << feature);

    FeatureSetProtoData(handle, bluetooth_ins);
}

void feature_bluetooth_uninit_bt_ins(feature_bluetooth_feature_type_t feature, FeatureProtoHandle handle)
{
    bt_instance_t* bluetooth_ins;
    feature_bluetooth_features_info_t* features_info;

    if (!feature_bluetooth_using_feature()) {
        return;
    }

    FeatureSetProtoData(handle, NULL);

    bluetooth_ins = bluetooth_find_instance(getpid());

    if (bluetooth_ins == NULL) {
        FEATURE_LOG_ERROR("Bluetooth instance not found.");
        return;
    }

    features_info = (feature_bluetooth_features_info_t*)bluetooth_ins->context;

    if (!features_info) {
        FEATURE_LOG_ERROR("Feature context not found.");
        return;
    }

    features_info->created_features &= ~(1UL << feature);

    if (features_info->created_features) {
        return;
    }

    feature_bluetooth_callback_uninit(bluetooth_ins);
    bluetooth_delete_instance(bluetooth_ins);
}

void feature_bluetooth_set_bt_ins(FeatureProtoHandle protoHandle)
{
    bt_instance_t* bluetooth_ins = bluetooth_get_instance();
    FeatureSetProtoData(protoHandle, bluetooth_ins);
}

bt_instance_t* feature_bluetooth_get_bt_ins(FeatureInstanceHandle feature)
{
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    return FeatureGetProtoData(protoHandle);
}