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
#include "bluetooth.h"
#include "bt_adapter.h"
#include "feature_bluetooth.h"
#include "feature_exports.h"
#include "feature_log.h"

#define file_tag "bluetooth"

void system_bluetooth_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    feature_bluetooth_init_bt_ins_async(handle);
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

static void get_addr_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addr, void* userdata)
{
    feature_data_t* data = (feature_data_t*)userdata;

    if (FeatureInstanceIsDetached(data->feature_ins)) {
        FeatureFreeInstanceHandle(data->feature_ins);
        free(data);
        return;
    }

    if (addr != NULL) {
        ft_context_ref ft_ctx = FeatureGetContext(data->feature_ins);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        ft_value_t ret_data = ft_from_string(ft_ctx, addr_str);
        ft_obj_set_property(ft_ctx, ret_obj, "address", ret_data);

        FeaturePromiseResolve(data->feature_ins, data->pid, &ret_obj);
        ft_free_value(ft_ctx, ret_obj);
    } else {
        FeaturePromiseReject(data->feature_ins, data->pid, bt_status_to_feature_error(status), "get address failed!");
    }

    FeatureFreeInstanceHandle(data->feature_ins);
    free(data);
}

void system_bluetooth_wrap_getAddressAsync(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid)
{
    feature_data_t* data;
    bt_status_t status;

    data = (feature_data_t*)malloc(sizeof(feature_data_t));
    if (!data)
        return;

    data->feature_ins = FeatureDupInstanceHandle(feature);
    data->pid = pid;

    status = bt_adapter_get_address_async(feature_bluetooth_get_bt_ins(feature), get_addr_cb, (void*)data);

    if (status == BT_STATUS_SUCCESS)
        return;

    FeaturePromiseReject(feature, pid, bt_status_to_feature_error(status), "get address failed!");
    FeatureFreeInstanceHandle(data->feature_ins);
    free(data);
}