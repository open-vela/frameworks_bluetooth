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
#include "bt_le_scan.h"
#include "bt_message_advertiser.h"
#include "bt_message_scan.h"
#include "feature_bluetooth.h"
#include "feature_context.h"
#include "feature_exports.h"
#include "feature_log.h"

#define file_tag "bluetooth_ble"

void system_bluetooth_ble_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    feature_bluetooth_init_bt_ins_async(handle);
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
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_bluetooth_ble_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

static bool adv_userdata_cmp(void* node, void* userdata)
{
    return ((feature_bluetooth_adv_info_t*)node)->start_userdata == userdata;
}

static bool adv_cmp(void* node, void* adv)
{
    return ((feature_bluetooth_adv_info_t*)node)->adv == adv;
}

static bool scan_userdata_cmp(void* node, void* userdata)
{
    return ((feature_bluetooth_scan_info_t*)node)->start_userdata == userdata;
}

static bool scan_cmp(void* node, void* scan)
{
    return ((feature_bluetooth_scan_info_t*)node)->scan == scan;
}

static bool scan_subscribe_info_cmp(void* node, void* id)
{
    return ((scan_subscribe_info_t*)node)->id == *(FtInt*)id;
}

#define FIND_INFO_BY_USERDATA(ins, data, type, ret)                                              \
    do {                                                                                         \
        feature_bluetooth_features_info_t* features_info;                                        \
        bt_list_t* list;                                                                         \
        if (!ins) {                                                                              \
            ret = NULL;                                                                          \
            break;                                                                               \
        }                                                                                        \
        features_info = (feature_bluetooth_features_info_t*)(ins->context);                      \
        list = features_info->feature_ble_##type;                                                \
        if (!list) {                                                                             \
            ret = NULL;                                                                          \
            break;                                                                               \
        }                                                                                        \
        ret = (feature_bluetooth_##type##_info_t*)bt_list_find(list, type##_userdata_cmp, data); \
    } while (0);

#define FIND_INFO_BY_OBJECT(ins, obj, type, ret)                                       \
    do {                                                                               \
        feature_bluetooth_features_info_t* features_info;                              \
        bt_list_t* list;                                                               \
        if (!ins) {                                                                    \
            ret = NULL;                                                                \
            break;                                                                     \
        }                                                                              \
        features_info = (feature_bluetooth_features_info_t*)(ins->context);            \
        list = features_info->feature_ble_##type;                                      \
        if (!list) {                                                                   \
            ret = NULL;                                                                \
            break;                                                                     \
        }                                                                              \
        ret = (feature_bluetooth_##type##_info_t*)bt_list_find(list, type##_cmp, obj); \
    } while (0);

void system_bluetooth_ble_Advertiser_interface_adv_finalize(FeatureInterfaceHandle handle)
{
    feature_bluetooth_adv_info_t* adv_info = (feature_bluetooth_adv_info_t*)FeatureGetObjectData(handle);
    bt_instance_t* bluetooth_instance = adv_info->ins;
    feature_bluetooth_features_info_t* features_info = (feature_bluetooth_features_info_t*)(bluetooth_instance->context);

    if (adv_info->adv) {
        FEATURE_LOG_INFO("%s::%s(), stop advertising\n", file_tag, __FUNCTION__);
        bt_le_stop_advertising_async(bluetooth_instance, adv_info->adv, NULL, NULL);
    }

    if (adv_info->start_userdata) {
        free(adv_info->start_userdata);
        adv_info->start_userdata = NULL;
    }

    bt_list_remove(features_info->feature_ble_adv, adv_info);
}

FeatureInterfaceHandle system_bluetooth_ble_wrap_createAdvertiser(FeatureInstanceHandle feature, AppendData append_data)
{
    bt_instance_t* bluetooth_instance = feature_bluetooth_get_bt_ins(feature);
    feature_bluetooth_features_info_t* features_info = (feature_bluetooth_features_info_t*)(bluetooth_instance->context);
    feature_bluetooth_adv_info_t* adv_info = (feature_bluetooth_adv_info_t*)calloc(1, sizeof(feature_bluetooth_adv_info_t));

    FeatureInterfaceHandle handle = system_bluetooth_ble_createAdvertiser_instance(feature);
    FEATURE_LOG_INFO("%s::%s(), FeatureInstanceHandle: %p, FeatureInterfaceHandle: %p\n", file_tag, __FUNCTION__, feature, handle);

    adv_info->ins = bluetooth_instance;
    adv_info->interface = handle;

    bt_list_add_tail(features_info->feature_ble_adv, adv_info);
    FeatureSetObjectData(handle, adv_info);

    return handle;
}

static void on_advertising_start_cb(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status)
{
    feature_data_t* data;
    feature_bluetooth_adv_info_t* adv_info;
    bt_instance_t* bluetooth_instance;

    bluetooth_instance = ((bt_advertiser_remote_t*)adv)->ins;
    FIND_INFO_BY_OBJECT(bluetooth_instance, adv, adv, adv_info);
    if (!adv_info) {
        FEATURE_LOG_ERROR("%s, adv_info not found", __func__);
        return;
    }

    data = (feature_data_t*)adv_info->start_userdata;

    FEATURE_LOG_INFO("%s, handle:%p, adv_id:%d, status:%d", __func__, adv, adv_id, status);

    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, adv fail", __func__);
        adv_info->adv = NULL;
        adv_info->busy = false;
        FeaturePromiseReject(adv_info->interface, data->pid, status, "start advertising failed!");
    } else {
        FeaturePromiseResolve(adv_info->interface, data->pid);
    }

    free(adv_info->start_userdata);
    adv_info->start_userdata = NULL;
}

static void on_advertising_stopped_cb(bt_advertiser_t* adv, uint8_t adv_id)
{
    feature_bluetooth_adv_info_t* adv_info;
    bt_instance_t* bluetooth_instance;

    bluetooth_instance = ((bt_advertiser_remote_t*)adv)->ins;
    FIND_INFO_BY_OBJECT(bluetooth_instance, adv, adv, adv_info);
    if (!adv_info) {
        FEATURE_LOG_ERROR("%s, adv_info not found", __func__);
        return;
    }

    FEATURE_LOG_INFO("%s, handle:%p, adv_id:%d", __func__, adv, adv_id);

    adv_info->adv = NULL;
    adv_info->busy = false;
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

static bt_status_t feature_get_advertiser_data(system_bluetooth_ble_AdvertiseData* data,
    advertiser_data_t* adv_data, feature_bluetooth_adv_info_t* adv_info)
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
    uint8_t** p_adv_data, uint16_t* adv_len, feature_bluetooth_adv_info_t* adv_info)
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
    uint8_t** p_scan_rsp_data, uint16_t* scan_rsp_len, feature_bluetooth_adv_info_t* adv_info)
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
    feature_bluetooth_adv_info_t* adv_info;

    FIND_INFO_BY_USERDATA(ins, userdata, adv, adv_info);
    if (!adv_info)
        goto error;

    assert(adv_info->adv == NULL);

    if (adv) {
        adv_info->adv = adv;
    } else {
        adv_info->busy = false;
        FeaturePromiseReject(adv_info->interface, data->pid, status, "start advertising failed!");
    }

    return;

error:
    if (adv)
        bt_le_stop_advertising_async(ins, adv, NULL, NULL);
}

void system_bluetooth_ble_Advertiser_interface_adv_startAdvertising(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_StartAdvertisingParams* params)
{
    bt_status_t status;
    feature_data_t* data = NULL;
    feature_bluetooth_adv_info_t* adv_info = NULL;
    ble_adv_params_t adv_params = { 0 };
    advertiser_data_t *adv = NULL, *scan_rsp = NULL;
    uint8_t *p_adv_data = NULL, *p_scan_rsp_data = NULL;
    uint16_t adv_len = 0;
    uint16_t scan_rsp_len = 0;

    adv_info = FeatureGetObjectData(handle);
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

    data->interface = handle;
    data->pid = pid;

    adv_info->start_userdata = (void*)data;

    status = bt_le_start_advertising_async(adv_info->ins, &adv_params,
        p_adv_data, adv_len, p_scan_rsp_data, scan_rsp_len, &adv_callback,
        start_adv_cb, (void*)data);

    if (status != BT_STATUS_SUCCESS) {
        goto error;
    }

    if (adv) {
        advertiser_data_free(adv);
        adv = NULL;
    }

    if (scan_rsp) {
        advertiser_data_free(scan_rsp);
        scan_rsp = NULL;
    }

    adv_info->busy = true;
    return;

error:
    if (data) {
        free(data);
        adv_info->start_userdata = NULL;
    }

    if (adv)
        advertiser_data_free(adv);

    if (scan_rsp)
        advertiser_data_free(scan_rsp);

    FeaturePromiseReject(handle, pid, status, "start advertising failed!");
}

void system_bluetooth_ble_Advertiser_interface_adv_stopAdvertising(FeatureInterfaceHandle handle, AppendData append_data)
{
    feature_bluetooth_adv_info_t* adv_info = FeatureGetObjectData(handle);

    if (adv_info->adv == NULL)
        return;

    bt_le_stop_advertising_async(adv_info->ins, adv_info->adv, NULL, NULL);
}

FeatureInterfaceHandle system_bluetooth_ble_wrap_createScanner(FeatureInstanceHandle feature, AppendData append_data)
{
    bt_instance_t* bluetooth_instance = feature_bluetooth_get_bt_ins(feature);
    feature_bluetooth_features_info_t* features_info = (feature_bluetooth_features_info_t*)(bluetooth_instance->context);
    feature_bluetooth_scan_info_t* scan_info = (feature_bluetooth_scan_info_t*)calloc(1, sizeof(feature_bluetooth_scan_info_t));

    FeatureInterfaceHandle handle = system_bluetooth_ble_createScanner_instance(feature);
    FEATURE_LOG_INFO("%s::%s(), FeatureInstanceHandle: %p, FeatureInterfaceHandle: %p\n", file_tag, __FUNCTION__, feature, handle);

    scan_info->ins = bluetooth_instance;
    scan_info->interface = handle;
    scan_info->subscribe_info = bt_list_new(feature_ble_list_free);
    scan_info->subscribe_id = 1;

    bt_list_add_tail(features_info->feature_ble_scan, scan_info);
    FeatureSetObjectData(handle, scan_info);

    return handle;
}

void system_bluetooth_ble_Scanner_interface_scan_finalize(FeatureInterfaceHandle handle)
{
    bt_list_node_t* node;
    feature_bluetooth_scan_info_t* scan_info = (feature_bluetooth_scan_info_t*)FeatureGetObjectData(handle);
    bt_instance_t* bluetooth_instance = scan_info->ins;
    feature_bluetooth_features_info_t* features_info = (feature_bluetooth_features_info_t*)(bluetooth_instance->context);

    if (scan_info->scan) {
        FEATURE_LOG_INFO("%s::%s(), stop scanning\n", file_tag, __FUNCTION__);
        bt_le_stop_scan_async(bluetooth_instance, scan_info->scan, NULL, NULL);
        scan_info->scan = NULL;
    }

    if (scan_info->start_userdata) {
        free(scan_info->start_userdata);
        scan_info->start_userdata = NULL;
    }

    for (node = bt_list_head(scan_info->subscribe_info); node != NULL; node = bt_list_next(scan_info->subscribe_info, node)) {
        scan_subscribe_info_t* subscribe_info = bt_list_node(node);
        FeatureRemoveCallback(handle, subscribe_info->callback);
        FeatureRemoveCallback(handle, subscribe_info->fail);
    }
    bt_list_free(scan_info->subscribe_info);

    bt_list_remove(features_info->feature_ble_scan, scan_info);
}

static void on_scan_result_cb(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    bt_list_node_t* node;
    feature_bluetooth_scan_info_t* scan_info;
    system_bluetooth_ble_ScanResult* result_data = NULL;
    FtArray* result_array = NULL;
    FtAny data = NULL;
    bt_instance_t* bluetooth_instance;

    bluetooth_instance = ((bt_scan_remote_t*)scanner)->ins;
    FIND_INFO_BY_OBJECT(bluetooth_instance, scanner, scan, scan_info);
    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scan_info not found", __func__);
        return;
    }
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    result_array = system_bluetooth_ble_malloc_ScanResult_struct_type_array();
    result_array->_size = 1;
    result_data = system_bluetooth_bleMallocScanResult();
    result_array->_element = calloc(result_array->_size, sizeof(system_bluetooth_ble_ScanResult*));
    ((system_bluetooth_ble_ScanResult**)result_array->_element)[0] = result_data;
    bt_addr_ba2str(&result->addr, addr_str);
    result_data->deviceId = StringToFtString(addr_str);
    result_data->rssi = result->rssi;

    ft_context_ref ft_ctx = FeatureGetContext(scan_info->interface);
    data = (FtAny)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *data = ft_from_buffer(ft_ctx, (uint8_t*)result->adv_data, result->length);
    result_data->data = data;

    for (node = bt_list_head(scan_info->subscribe_info); node != NULL; node = bt_list_next(scan_info->subscribe_info, node)) {
        scan_subscribe_info_t* subscribe_info = bt_list_node(node);
        FeatureInvokeCallback(scan_info->interface, subscribe_info->callback, result_array);
    }

    ft_free_value(ft_ctx, *data);
    FeatureFreeValue(result_array);
}

static void on_scan_start_status_cb(bt_scanner_t* scanner, uint8_t status)
{
    feature_data_t* data;
    feature_bluetooth_scan_info_t* scan_info;
    bt_instance_t* bluetooth_instance;

    bluetooth_instance = ((bt_scan_remote_t*)scanner)->ins;
    FIND_INFO_BY_OBJECT(bluetooth_instance, scanner, scan, scan_info);
    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scan_info not found", __func__);
        return;
    }

    data = (feature_data_t*)scan_info->start_userdata;

    FEATURE_LOG_INFO("%s, scanner:%p, status:%d", __func__, scanner, status);

    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, scan start fail", __func__);
        scan_info->scan = NULL;
        scan_info->busy = false;
        FeaturePromiseReject(scan_info->interface, data->pid, status, "start scan failed!");
    } else {
        FeaturePromiseResolve(scan_info->interface, data->pid);
    }

    free(scan_info->start_userdata);
    scan_info->start_userdata = NULL;
}

static void on_scan_stopped_cb(bt_scanner_t* scanner)
{
    FEATURE_LOG_ERROR("%s, scanner:%p", __func__, scanner);
}

static const scanner_callbacks_t scanner_callbacks = {
    sizeof(scanner_callbacks_t),
    on_scan_result_cb,
    on_scan_start_status_cb,
    on_scan_stopped_cb
};

static void start_scan_cb(bt_instance_t* ins, bt_status_t status, void* scan, void* userdata)
{
    feature_data_t* data = (feature_data_t*)userdata;
    feature_bluetooth_scan_info_t* scan_info;

    FIND_INFO_BY_USERDATA(ins, userdata, scan, scan_info);

    if (!scan_info)
        goto error;

    assert(scan_info->scan == NULL);

    if (scan) {
        scan_info->scan = scan;
    } else {
        scan_info->busy = false;
        FeaturePromiseReject(scan_info->interface, data->pid, status, "start scan failed!");
    }

    return;

error:
    if (scan)
        bt_le_stop_scan_async(ins, scan, NULL, NULL);
}

void system_bluetooth_ble_Scanner_interface_scan_startBLEScan(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_StartScanParams* params)
{
    bt_status_t status;
    feature_data_t* data = NULL;
    feature_bluetooth_scan_info_t* scan_info;
    ble_scan_settings_t settings = { BT_SCAN_MODE_LOW_POWER, 0, BT_LE_SCAN_TYPE_PASSIVE, BT_LE_1M_PHY, { 0 } };

    scan_info = FeatureGetObjectData(handle);
    status = BT_STATUS_FAIL;

    if (!params)
        goto error;

    if (scan_info->busy) {
        FEATURE_LOG_ERROR("%s, Repeated Attempt", __func__);
        goto error;
    }

    if (params->options) {
        settings.scan_mode = params->options->dutyMode;
    }

    data = (feature_data_t*)malloc(sizeof(feature_data_t));
    if (!data)
        goto error;

    data->interface = handle;
    data->pid = pid;

    scan_info->start_userdata = (void*)data;

    status = bt_le_start_scan_settings_async(scan_info->ins, &settings, &scanner_callbacks, start_scan_cb, (void*)data);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    scan_info->busy = true;
    return;

error:
    if (data) {
        free(data);
        scan_info->start_userdata = NULL;
    }

    FeaturePromiseReject(handle, pid, status, "start scan failed!");
}

void system_bluetooth_ble_Scanner_interface_scan_stopBLEScan(FeatureInterfaceHandle handle, AppendData append_data)
{
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);

    if (scan_info->scan == NULL)
        return;

    bt_le_stop_scan_async(scan_info->ins, scan_info->scan, NULL, NULL);
    scan_info->scan = NULL;
    scan_info->busy = false;
}

void system_bluetooth_ble_Scanner_interface_scan_getScanState(FeatureInterfaceHandle handle, AppendData append_data, FtPromiseId pid)
{
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);

    if (scan_info->scan == NULL) {
        FeaturePromiseReject(handle, pid, BT_STATUS_FAIL, "scanner not found");
        return;
    }

    if (scan_info->scan) {
        FeaturePromiseResolve(handle, pid, STATE_SCANING);
    } else {
        FeaturePromiseResolve(handle, pid, STATE_NON_SCAN);
    }
}

FtInt system_bluetooth_ble_Scanner_interface_scan_subscribeBLEDeviceFind(FeatureInterfaceHandle handle, AppendData append_data,
    system_bluetooth_ble_DeviceFindParams* params)
{
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);
    scan_subscribe_info_t* subscribe_info;

    if (!(params->callback > 0)) {
        if (params->fail > 0) {
            FeatureInvokeCallback(handle, params->fail);
            FeatureRemoveCallback(handle, params->fail);
        }
        FEATURE_LOG_ERROR("%s, callback is not set", __func__);
        return -1;
    }

    subscribe_info = (scan_subscribe_info_t*)malloc(sizeof(scan_subscribe_info_t));
    subscribe_info->callback = params->callback;
    // actually not used
    subscribe_info->fail = params->fail;
    subscribe_info->id = scan_info->subscribe_id++;

    bt_list_add_tail(scan_info->subscribe_info, subscribe_info);

    return subscribe_info->id;
}

void system_bluetooth_ble_Scanner_interface_scan_unsubscribeBLEDeviceFind(FeatureInterfaceHandle handle, AppendData append_data, FtInt SubscribeId)
{
    scan_subscribe_info_t* subscribe_info;
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);

    subscribe_info = (scan_subscribe_info_t*)bt_list_find(scan_info->subscribe_info, scan_subscribe_info_cmp, &SubscribeId);
    if (!subscribe_info)
        return;

    FeatureRemoveCallback(handle, subscribe_info->callback);
    FeatureRemoveCallback(handle, subscribe_info->fail);
    bt_list_remove(scan_info->subscribe_info, subscribe_info);
}