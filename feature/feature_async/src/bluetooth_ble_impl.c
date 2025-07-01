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
#include <ctype.h>

#include "advertiser_data.h"
#include "bluetooth.h"
#include "bluetooth_ble.h"
#include "bt_adapter.h"
#include "bt_le_advertiser.h"
#include "feature_bluetooth.h"
#include "feature_context.h"
#include "feature_exports.h"
#include "feature_log.h"

#define file_tag "bluetooth_ble"

typedef struct {
    bt_instance_t* bluetooth_ins;
    FeatureInterfaceHandle interface;
    bool busy;
    void* adv;
} advertiser_t;

static advertiser_t* advertiser_obj_get(FeatureInterfaceHandle handle)
{
    return (advertiser_t*)FeatureGetObjectData(handle);
}

void system_bluetooth_ble_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    feature_bluetooth_init_bt_ins_async(FEATURE_BLUETOOTH_BLE, handle);
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    feature_bluetooth_uninit_bt_ins_async(FEATURE_BLUETOOTH_BLE, handle);
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_Advertiser_interface_adv_finalize(FeatureInterfaceHandle handle)
{
    advertiser_t* adv_info = advertiser_obj_get(handle);

    if (adv_info->adv) {
        FEATURE_LOG_INFO("%s::%s(), stop advertising\n", file_tag, __FUNCTION__);
        bt_le_stop_advertising_async(adv_info->bluetooth_ins, adv_info->adv, NULL, NULL);
    }

    free(adv_info);
    adv_info = NULL;
    FeatureSetObjectData(handle, NULL);
}

FeatureInterfaceHandle system_bluetooth_ble_wrap_createAdvertiser(FeatureInstanceHandle feature, AppendData append_data)
{
    advertiser_t* adv_info = (advertiser_t*)malloc(sizeof(advertiser_t));

    FeatureInterfaceHandle handle = system_bluetooth_ble_createAdvertiser_instance(feature);
    FEATURE_LOG_INFO("%s::%s(), FeatureInstanceHandle: %p, FeatureInterfaceHandle: %p\n", file_tag, __FUNCTION__, feature, handle);

    adv_info->bluetooth_ins = feature_bluetooth_get_bt_ins(feature);
    adv_info->interface = handle;
    adv_info->adv = NULL;
    adv_info->busy = false;

    FeatureSetObjectData(handle, adv_info);
    return handle;
}

static void on_advertising_start_cb(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status)
{
    FEATURE_LOG_INFO("%s, handle:%p, adv_id:%d, status:%d", __func__, adv, adv_id, status);
}

static void on_advertising_stopped_cb(bt_advertiser_t* adv, uint8_t adv_id)
{
    FEATURE_LOG_INFO("%s, handle:%p, adv_id:%d", __func__, adv, adv_id);
}

static advertiser_callback_t adv_callback = {
    sizeof(adv_callback),
    on_advertising_start_cb,
    on_advertising_stopped_cb
};

bt_status_t get_valid_uuid16(uint16_t* out, const char* in)
{
    static const char uuid_str[] = "0000****-0000-1000-8000-00805f9b34fb";
    char uuid16_str[5];
    char c;
    char* e;

    if (!in)
        return BT_STATUS_PARM_INVALID;

    if (strlen(in) != strlen(uuid_str))
        return BT_STATUS_PARM_INVALID;

    for (int i = 0; i < strlen(uuid_str); i++) {
        c = tolower(in[i]); /**< uppercase to lowercase, remain unchanged otherwise */
        if (c != uuid_str[i] && uuid_str[i] != '*')
            return BT_STATUS_PARM_INVALID;
    }

    strlcpy(uuid16_str, in + 4, sizeof(uuid16_str));
    *out = (uint16_t)strtoul(uuid16_str, &e, 16);
    if (*e != '\0') { /**< unexpected value */
        *out = 0;
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t feature_get_advertiser_data(system_bluetooth_ble_AdvertiseData* data, advertiser_data_t* adv_data, advertiser_t* adv_info)
{
    bt_uuid_t uuid;
    int index = 0;
    size_t length;
    uint16_t manufacture_id;
    uint16_t service_id;

    if (!data)
        return BT_STATUS_SUCCESS;

    for (index = 0; data->manufactureData != NULL && index < data->manufactureData->_size; index++) {
        system_bluetooth_ble_ManufactureData** manufactureData = (system_bluetooth_ble_ManufactureData**)(data->manufactureData->_element);
        if (manufactureData == NULL)
            break;

        if (manufactureData[index] == NULL)
            break;

        if (get_valid_uuid16(&manufacture_id, manufactureData[index]->manufactureId) != BT_STATUS_SUCCESS) {
            FEATURE_LOG_ERROR("%s, Invalid UUID", __func__);
            return BT_STATUS_PARM_INVALID;
        }

        FtAny manufactureValue = manufactureData[index]->manufactureValue;
        ft_context_ref ft_ctx = FeatureGetContext(adv_info->interface);
        uint8_t* value = ft_to_buffer(ft_ctx, &length, *manufactureValue);
        if (length <= 0 || value == NULL) {
            FEATURE_LOG_ERROR("%s, The length and data of manufactureData do not match.", __func__);
            return BT_STATUS_PARM_INVALID;
        }

        advertiser_data_add_manufacture_data(adv_data, manufacture_id, (uint8_t*)value, (uint8_t)length);
    }

    for (index = 0; data->serviceData != NULL && index < data->serviceData->_size; index++) {
        system_bluetooth_ble_ServiceData** serviceData = (system_bluetooth_ble_ServiceData**)(data->serviceData->_element);
        if (serviceData == NULL)
            break;

        if (serviceData[index] == NULL)
            break;

        if (get_valid_uuid16(&service_id, serviceData[index]->serviceUuid) != BT_STATUS_SUCCESS) {
            FEATURE_LOG_ERROR("%s, Invalid UUID", __func__);
            return BT_STATUS_PARM_INVALID;
        }

        FtAny serviceValue = serviceData[index]->serviceValue;
        ft_context_ref ft_ctx = FeatureGetContext(adv_info->interface);
        uint8_t* value = ft_to_buffer(ft_ctx, &length, *serviceValue);
        if (length <= 0 || value == NULL) {
            FEATURE_LOG_ERROR("%s, The length and data of serviceData do not match.", __func__);
            return BT_STATUS_PARM_INVALID;
        }

        bt_uuid16_create(&uuid, service_id);
        if (!advertiser_data_add_service_data(adv_data, &uuid, (uint8_t*)value, (uint8_t)length))
            return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t feature_set_adv_params(system_bluetooth_ble_AdvertiseSetting* setting, ble_adv_params_t* adv_params)
{
    if (setting->txPower < -20 || setting->txPower > 10) {
        FEATURE_LOG_ERROR("%s, Invalid txPower parameter.", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (setting->interval < 0x0020 || setting->interval > 0x4000) {
        FEATURE_LOG_ERROR("%s, Invalid interval parameter.", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (setting->connectable)
        adv_params->adv_type = BT_LE_ADV_IND;
    else
        adv_params->adv_type = BT_LE_ADV_NONCONN_IND;

    bt_addr_set_empty(&adv_params->peer_addr);
    adv_params->peer_addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    bt_addr_set_empty(&adv_params->own_addr);
    adv_params->own_addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    adv_params->tx_power = setting->txPower;
    adv_params->interval = setting->interval;
    adv_params->duration = 0;
    adv_params->channel_map = BT_LE_ADV_CHANNEL_DEFAULT;
    adv_params->filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE;

    return BT_STATUS_SUCCESS;
}

static bt_status_t feature_set_adv_data(system_bluetooth_ble_AdvertiseData* adv_data, advertiser_data_t** adv,
    uint8_t** p_adv_data, uint16_t* adv_len, advertiser_t* adv_info)
{
    bt_uuid_t uuid;
    uint16_t id;

    if (!adv_data)
        return BT_STATUS_FAIL;

    *adv = advertiser_data_new();
    advertiser_data_set_flags(*adv, BT_AD_FLAG_DUAL_MODE | BT_AD_FLAG_GENERAL_DISCOVERABLE); /* set adv flags 0x08 */

    for (int i = 0; adv_data->serviceUuids != NULL && i < adv_data->serviceUuids->_size; i++) {
        char** serviceUuid = (char**)(adv_data->serviceUuids->_element);
        if (!serviceUuid || !serviceUuid[i])
            goto error;

        if (get_valid_uuid16(&id, serviceUuid[i]) != BT_STATUS_SUCCESS) {
            FEATURE_LOG_ERROR("%s, Invalid UUID", __func__);
            goto error;
        }

        bt_uuid16_create(&uuid, id);
        advertiser_data_add_service_uuid(*adv, &uuid);
    }

    if (feature_get_advertiser_data(adv_data, *adv, adv_info) != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("get advertiser data failed!");
        goto error;
    }

    *p_adv_data = advertiser_data_build(*adv, adv_len);
    if (*p_adv_data)
        advertiser_data_dump(*p_adv_data, *adv_len, NULL);

    return BT_STATUS_SUCCESS;

error:
    if (*adv) {
        advertiser_data_free(*adv);
        *adv = NULL;
    }

    return BT_STATUS_FAIL;
}

static bt_status_t feature_set_scan_rsp_data(system_bluetooth_ble_AdvertiseData* scan_rsp_data, advertiser_data_t** scan_rsp,
    uint8_t** p_scan_rsp_data, uint16_t* scan_rsp_len, advertiser_t* adv_info)
{
    if (!scan_rsp_data)
        return BT_STATUS_SUCCESS;

    *scan_rsp = advertiser_data_new();
    if (feature_get_advertiser_data(scan_rsp_data, *scan_rsp, adv_info) != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("get scan response data failed!");
        goto error;
    }

    *p_scan_rsp_data = advertiser_data_build(*scan_rsp, scan_rsp_len);
    if (*p_scan_rsp_data)
        advertiser_data_dump(*p_scan_rsp_data, *scan_rsp_len, NULL);

    return BT_STATUS_SUCCESS;

error:
    if (*scan_rsp) {
        advertiser_data_free(*scan_rsp);
        *scan_rsp = NULL;
    }

    return BT_STATUS_FAIL;
}

static void start_adv_cb(bt_instance_t* ins, bt_status_t status, void* adv, void* userdata)
{
    feature_data_t* data = (feature_data_t*)userdata;
    advertiser_t* adv_info;

    if (FeatureInstanceIsDetached(data->feature_if)) {
        FEATURE_LOG_ERROR("feature instance is detached!");
        goto error;
    }

    adv_info = advertiser_obj_get(data->feature_if);
    assert(adv_info->adv == NULL);

    if (adv) {
        adv_info->adv = adv;
        FeaturePromiseResolve(adv_info->interface, data->pid);
    } else {
        adv_info->busy = false;
        FeaturePromiseReject(adv_info->interface, data->pid, status, "start advertising failed!");
    }

    FeatureFreeInstanceHandle(data->feature_if);
    free(data);
    return;

error:
    if (adv)
        bt_le_stop_advertising_async(bluetooth_find_async_instance(getpid()), adv, NULL, NULL);

    if (!FeatureInstanceIsDetached(data->feature_if))
        FeatureFreeInstanceHandle(data->feature_if);

    free(data);
}

void system_bluetooth_ble_Advertiser_interface_adv_startAdvertising(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_StartAdvertisingParams* params)
{
    bt_status_t status;
    feature_data_t* data;
    advertiser_t* adv_info;
    ble_adv_params_t adv_params = { 0 };
    advertiser_data_t *adv = NULL, *scan_rsp = NULL;
    uint8_t *p_adv_data = NULL, *p_scan_rsp_data = NULL;
    uint16_t adv_len = 0;
    uint16_t scan_rsp_len = 0;

    adv_info = advertiser_obj_get(handle);
    status = BT_STATUS_FAIL;

    if (!params || !params->setting)
        goto error;

    if (adv_info->busy) {
        FEATURE_LOG_ERROR("%s, Repeated Attempt", __func__);
        goto error;
    }

    // AdvertiseSetting
    if (feature_set_adv_params(params->setting, &adv_params) != BT_STATUS_SUCCESS)
        goto error;

    // AdvertiseData-advData
    if (feature_set_adv_data(params->advData, &adv, &p_adv_data, &adv_len, adv_info) != BT_STATUS_SUCCESS)
        goto error;

    // AdvertiseData-scanRspData
    if (feature_set_scan_rsp_data(params->advResponse, &scan_rsp, &p_scan_rsp_data, &scan_rsp_len, adv_info) != BT_STATUS_SUCCESS)
        goto error;

    data = (feature_data_t*)malloc(sizeof(feature_data_t));
    if (!data)
        goto error;

    data->feature_if = FeatureDupInstanceHandle(handle);
    data->pid = pid;

    status = bt_le_start_advertising_async(adv_info->bluetooth_ins, &adv_params,
        p_adv_data, adv_len, p_scan_rsp_data, scan_rsp_len, &adv_callback,
        start_adv_cb, (void*)data);

    if (adv) {
        advertiser_data_free(adv);
        adv = NULL;
    }

    if (scan_rsp) {
        advertiser_data_free(scan_rsp);
        scan_rsp = NULL;
    }

    if (status == BT_STATUS_SUCCESS) {
        adv_info->busy = true;
        return;
    }

    FeatureFreeInstanceHandle(data->feature_if);
    free(data);

error:
    if (adv)
        advertiser_data_free(adv);

    if (scan_rsp)
        advertiser_data_free(scan_rsp);

    FeaturePromiseReject(handle, pid, status, "start advertising failed!");
}

void system_bluetooth_ble_Advertiser_interface_adv_stopAdvertising(FeatureInterfaceHandle handle, AppendData append_data)
{
    advertiser_t* adv_info;

    adv_info = advertiser_obj_get(handle);
    if (adv_info->adv == NULL)
        return;

    bt_le_stop_advertising_async(adv_info->bluetooth_ins, adv_info->adv, NULL, NULL);
    adv_info->adv = NULL;
    adv_info->busy = false;
}
