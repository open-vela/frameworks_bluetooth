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
#include "bt_list.h"
#include "feature_exports.h"

typedef enum {
    FEATURE_BLUETOOTH,
    FEATURE_BLUETOOTH_BLE,
} feature_bluetooth_feature_type_t;

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
    void* params;
    union {
        FeatureInstanceHandle feature_ins;
        FeatureInterfaceHandle feature_if;
    };
} feature_data_t;

typedef struct {
    uint32_t created_features;
} feature_bluetooth_features_info_t;

char* StringToFtString(const char* str);
void feature_bluetooth_post_task(FeatureInstanceHandle handle, FtCallbackId callback_id, void* data);

void feature_bluetooth_init_bt_ins_async(feature_bluetooth_feature_type_t feature, FeatureProtoHandle handle);
void feature_bluetooth_uninit_bt_ins_async(feature_bluetooth_feature_type_t feature, FeatureProtoHandle handle);
bt_instance_t* feature_bluetooth_get_bt_ins(FeatureInstanceHandle feature);
#endif // _FEATURE_BLUETOOTH_H_