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

void feature_bluetooth_deal_callback(int status, void* data)
{
    callback_info_t* info = (callback_info_t*)data;
    FEATURE_LOG_INFO("callback type:%d, feature:%p, callback id: %d", info->callback_id, info->feature, info->feature_callback_id);
    if (!FeatureInvokeCallback(info->feature,
            info->feature_callback_id, info->data)) {
        FEATURE_LOG_ERROR("callback type:%d, feature:%p, callback id: %d, invoke discoveryresult callback failed!",
            info->callback_id, info->feature, info->feature_callback_id);
    }

    free(data);
}

char* StringToFtString(const char* str)
{
    int len = strlen(str);
    char* ftStr = (char*)FeatureMalloc(len + 1, FT_CHAR);
    strcpy(ftStr, str);
    return ftStr;
}

void feature_bluetooth_init_bt_ins(feature_bluetooth_feature_type_t feature)
{
    bt_instance_t* bluetooth_ins = bluetooth_get_instance();

    if (bluetooth_ins == NULL) {
        FEATURE_LOG_ERROR("Failed to get Bluetooth instance.");
        return;
    }

    if (bluetooth_ins->context == NULL) {
        feature_bluetooth_callback_init(bluetooth_ins);
    }

    ((feature_bluetooth_features_info_t*)bluetooth_ins->context)->created_features |= (1UL << feature);
}

void feature_bluetooth_uninit_bt_ins(feature_bluetooth_feature_type_t feature)
{
    bt_instance_t* bluetooth_ins;
    feature_bluetooth_features_info_t* features_info;

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

void feature_bluetooth_clean_bt_ins(FeatureProtoHandle protoHandle)
{
    FeatureSetProtoData(protoHandle, NULL);
}

bt_instance_t* feature_bluetooth_get_bt_ins(FeatureInstanceHandle feature)
{
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    return FeatureGetProtoData(protoHandle);
}