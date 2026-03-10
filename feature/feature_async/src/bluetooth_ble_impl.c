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
#include "bt_auracast_sink.h"
#include "bt_gatt_feature.h"
#include "bt_le_advertiser.h"
#include "bt_le_scan.h"
#include "bt_message_advertiser.h"
#include "bt_message_scan.h"
#include "bt_pa_sync.h"
#include "bt_utils.h"
#include "feature_bluetooth.h"
#include "feature_context.h"
#include "feature_exports.h"
#include "feature_log.h"

#define file_tag "bluetooth_ble"
#define FEATURE_BLE_FT_STRING_MAX (256)
#define FEATURE_BLE_PA_SYNC_SKIP (1)
#define FEATURE_BLE_PA_SYNC_TIMEOUT_MS (1000)
#define FEATURE_BLE_PTR_PENDING ((void*)-1)
#define FEATURE_BLE_FT_CALLBACK_ID_INVALID ((int)-1)
#define FEATURE_BLE_STRCAT(dst, size, src, ...)                 \
    do {                                                        \
        size_t _len = strlen(dst);                              \
        size_t _size = (size);                                  \
        if (_len + 1 >= _size)                                  \
            break;                                              \
        snprintf(dst + _len, _size - _len, src, ##__VA_ARGS__); \
    } while (0)

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

#ifdef CONFIG_BLUETOOTH_BLE_ADV
static bool adv_userdata_cmp(void* node, void* userdata)
{
    return ((feature_bluetooth_adv_info_t*)node)->start_userdata == userdata;
}

static bool adv_cmp(void* node, void* adv)
{
    return ((feature_bluetooth_adv_info_t*)node)->adv == adv;
}
#endif

#ifdef CONFIG_BLUETOOTH_BLE_SCAN
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
#endif

#ifdef CONFIG_BLUETOOTH_GATT
static bool gattc_userdata_cmp(void* node, void* userdata)
{
    return ((gattc_data_t*)node) == userdata;
}

static bool gattc_cmp(void* node, void* handle)
{
    return ((feature_bluetooth_gattc_info_t*)node)->gattc->handle == handle;
}

static bool gattc_userdata_type_cmp(void* node, void* type)
{
    return ((gattc_data_t*)node)->userdata_type == (gattc_userdata_type_t)type;
}
#endif

#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
static bool aurasnk_cmp(void* node, void* data)
{
    return node == data;
}

static bool aurasnk_instance_cmp(void* node, void* data)
{
    feature_bluetooth_aurasnk_info_t* aurasnk_info = node;

    return aurasnk_info->ins == data;
}

static bool aurasnk_scanner_cmp(void* node, void* data)
{
    feature_bluetooth_aurasnk_info_t* aurasnk_info = node;

    return aurasnk_info->scanner == data;
}

static bool aurasnk_work_cmp(void* node, void* data)
{
    feature_bluetooth_aurasnk_pending_work_t* work = node;
    aurasnk_work_type_t* p_type = data;

    return work->type == *p_type;
}
#endif

#define FIND_INFO_BY_USERDATA(ins, data, type, ret)                                              \
    do {                                                                                         \
        feature_bluetooth_features_info_t* features_info;                                        \
        bt_list_t* list;                                                                         \
        if (!ins || !ins->context) {                                                             \
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
        if (!ins || !ins->context) {                                                   \
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

#ifdef CONFIG_BLUETOOTH_GATT
feature_bluetooth_gattc_info_t* find_gattc_info_by_userdata(bt_instance_t* ins, void* data)
{
    feature_bluetooth_features_info_t* features_info;
    bt_list_t* list;
    bt_list_node_t* node;

    if (!ins || !ins->context)
        return NULL;

    features_info = (feature_bluetooth_features_info_t*)(ins->context);

    list = features_info->feature_ble_gattc;
    if (!list)
        return NULL;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        feature_bluetooth_gattc_info_t* gattc_info = bt_list_node(node);
        if (bt_list_find(gattc_info->userdata_list, gattc_userdata_cmp, data))
            return gattc_info;
    }

    return NULL;
}

bt_status_t get_valid_uuid128(uint8_t uuid128[16], const char* in)
{
    int num;
    int ret;
    if (strlen(in) != 36)
        return BT_STATUS_PARM_INVALID;

    if (in[8] != '-' || in[13] != '-' || in[18] != '-' || in[23] != '-')
        return BT_STATUS_PARM_INVALID;

    ret = sscanf(in, "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx"
                     "-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%n",
        &uuid128[15], &uuid128[14], &uuid128[13], &uuid128[12], &uuid128[11], &uuid128[10], &uuid128[9], &uuid128[8],
        &uuid128[7], &uuid128[6], &uuid128[5], &uuid128[4], &uuid128[3], &uuid128[2], &uuid128[1], &uuid128[0], &num);

    if (ret != 16 || num != 36)
        return BT_STATUS_PARM_INVALID;

    return BT_STATUS_SUCCESS;
}

char* bt_uuid_to_feature_string(const bt_uuid_t* bt_uuid)
{
    char uuid[40] = { 0 };
    bt_uuid_to_string(bt_uuid, uuid, 40);
    return StringToFtString(uuid);
}
#endif

#ifdef CONFIG_BLUETOOTH_BLE_ADV
static void feature_adv_destroy(FeatureInterfaceHandle handle)
{
    feature_bluetooth_adv_info_t* adv_info = (feature_bluetooth_adv_info_t*)FeatureGetObjectData(handle);
    if (adv_info == NULL)
        return;

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
#endif

void system_bluetooth_ble_Advertiser_interface_adv_finalize(FeatureInterfaceHandle handle)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    feature_adv_destroy(handle);
#endif
}

FeatureInterfaceHandle system_bluetooth_ble_wrap_createAdvertiser(FeatureInstanceHandle feature, AppendData append_data)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
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
#else
    return NULL;
#endif
}

#ifdef CONFIG_BLUETOOTH_BLE_ADV
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
        FeaturePromiseReject(adv_info->interface, data->pid, bt_status_to_feature_error(status), "start advertising failed!");
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
        adv_params->adv_type = BT_LE_LEGACY_ADV_IND;
    else
        adv_params->adv_type = BT_LE_LEGACY_ADV_NONCONN_IND;

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
    if (!(*adv))
        return BT_STATUS_FAIL;

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
    if (!(*scan_rsp))
        return BT_STATUS_FAIL;

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
        FeaturePromiseReject(adv_info->interface, data->pid, bt_status_to_feature_error(status), "start advertising failed!");
    }

    return;

error:
    if (adv)
        bt_le_stop_advertising_async(ins, adv, NULL, NULL);
}
#endif

void system_bluetooth_ble_Advertiser_interface_adv_startAdvertising(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_StartAdvertisingParams* params)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    bt_status_t status;
    feature_data_t* data = NULL;
    feature_bluetooth_adv_info_t* adv_info = NULL;
    ble_adv_params_t adv_params = { 0 };
    advertiser_data_t *adv = NULL, *scan_rsp = NULL;
    uint8_t *p_adv_data = NULL, *p_scan_rsp_data = NULL;
    uint16_t adv_len = 0;
    uint16_t scan_rsp_len = 0;

    status = BT_STATUS_FAIL;
    adv_info = FeatureGetObjectData(handle);
    if (!adv_info) {
        FEATURE_LOG_ERROR("%s, advertiser has been closed", __func__);
        return;
    }

    if (!params || !params->setting) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    if (adv_info->busy) {
        FEATURE_LOG_ERROR("%s, Repeated Attempt", __func__);
        status = BT_STATUS_DONE;
        goto error;
    }

    // AdvertiseSetting
    if (feature_set_adv_params(params->setting, &adv_params) != BT_STATUS_SUCCESS) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    // AdvertiseData-advData
    if (feature_set_adv_data(params->advData, &adv, &p_adv_data, &adv_len, adv_info) != BT_STATUS_SUCCESS) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    // AdvertiseData-scanRspData
    if (feature_set_scan_rsp_data(params->advResponse, &scan_rsp, &p_scan_rsp_data, &scan_rsp_len, adv_info) != BT_STATUS_SUCCESS) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    data = (feature_data_t*)malloc(sizeof(feature_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

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

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "start advertising failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "advertising is not supported.");
#endif
}

void system_bluetooth_ble_Advertiser_interface_adv_stopAdvertising(FeatureInterfaceHandle handle, AppendData append_data)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    feature_bluetooth_adv_info_t* adv_info = FeatureGetObjectData(handle);
    if (!adv_info) {
        FEATURE_LOG_ERROR("%s, advertiser has been closed", __func__);
        return;
    }

    if (adv_info->adv == NULL)
        return;

    bt_le_stop_advertising_async(adv_info->ins, adv_info->adv, NULL, NULL);
#endif
}

void system_bluetooth_ble_Advertiser_interface_adv_close(FeatureInterfaceHandle handle, AppendData append_data)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    feature_adv_destroy(handle);

    FeatureSetObjectData(handle, NULL);
#endif
}

FeatureInterfaceHandle system_bluetooth_ble_wrap_createScanner(FeatureInstanceHandle feature, AppendData append_data)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
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
#else
    return NULL;
#endif
}

#ifdef CONFIG_BLUETOOTH_BLE_SCAN
static void feature_scan_destroy(FeatureInterfaceHandle handle)
{
    bt_list_node_t* node;
    feature_bluetooth_scan_info_t* scan_info = (feature_bluetooth_scan_info_t*)FeatureGetObjectData(handle);
    if (scan_info == NULL)
        return;

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
#endif

void system_bluetooth_ble_Scanner_interface_scan_finalize(FeatureInterfaceHandle handle)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    feature_scan_destroy(handle);
#endif
}

#ifdef CONFIG_BLUETOOTH_BLE_SCAN
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

    switch (result->addr_type) {
    case BT_LE_ADDR_TYPE_PUBLIC:
        result_data->addressType = StringToFtString("PUBLIC");
        break;
    case BT_LE_ADDR_TYPE_RANDOM:
        result_data->addressType = StringToFtString("RANDOM");
        break;
    case BT_LE_ADDR_TYPE_ANONYMOUS:
        result_data->addressType = StringToFtString("ANONYMOUS");
        break;
    default:
        result_data->addressType = StringToFtString("UNKNOWN");
        break;
    }

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
        FeaturePromiseReject(scan_info->interface, data->pid, bt_status_to_feature_error(status), "start scan failed!");
    } else {
        FeaturePromiseResolve(scan_info->interface, data->pid);
    }

    free(scan_info->start_userdata);
    scan_info->start_userdata = NULL;
}

static void on_scan_stopped_cb(bt_scanner_t* scanner)
{
    feature_bluetooth_scan_info_t* scan_info;
    bt_instance_t* bluetooth_instance;

    bluetooth_instance = ((bt_scan_remote_t*)scanner)->ins;
    FIND_INFO_BY_OBJECT(bluetooth_instance, scanner, scan, scan_info);
    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scan_info not found", __func__);
        return;
    }

    FEATURE_LOG_ERROR("%s, scanner:%p", __func__, scanner);

    scan_info->scan = NULL;
    scan_info->busy = false;
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

    if (!scan_info) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    assert(scan_info->scan == NULL);

    if (scan) {
        scan_info->scan = scan;
    } else {
        scan_info->busy = false;
        FeaturePromiseReject(scan_info->interface, data->pid, bt_status_to_feature_error(status), "start scan failed!");
    }

    return;

error:
    if (scan)
        bt_le_stop_scan_async(ins, scan, NULL, NULL);
}
#endif

void system_bluetooth_ble_Scanner_interface_scan_startBLEScan(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_StartScanParams* params)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    bt_status_t status;
    feature_data_t* data = NULL;
    feature_bluetooth_scan_info_t* scan_info;
    ble_scan_settings_t settings = { BT_SCAN_MODE_LOW_POWER, 0, BT_LE_SCAN_TYPE_PASSIVE, BT_LE_1M_PHY, { 0 } };

    status = BT_STATUS_FAIL;
    scan_info = FeatureGetObjectData(handle);
    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scanner has been closed", __func__);
        return;
    }

    if (!params) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    if (scan_info->busy) {
        FEATURE_LOG_ERROR("%s, Repeated Attempt", __func__);
        status = BT_STATUS_DONE;
        goto error;
    }

    if (params->options) {
        settings.scan_mode = params->options->dutyMode;
    }

    data = (feature_data_t*)malloc(sizeof(feature_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

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

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "start scan failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "scanner is not supported");
#endif
}

void system_bluetooth_ble_Scanner_interface_scan_stopBLEScan(FeatureInterfaceHandle handle, AppendData append_data)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);
    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scanner has been closed", __func__);
        return;
    }

    if (scan_info->scan == NULL)
        return;

    bt_le_stop_scan_async(scan_info->ins, scan_info->scan, NULL, NULL);
#endif
}

void system_bluetooth_ble_Scanner_interface_scan_getScanState(FeatureInterfaceHandle handle, AppendData append_data, FtPromiseId pid)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);
    system_bluetooth_ble_ScanStateParams* state;
    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scanner has been closed", __func__);
        return;
    }

    state = system_bluetooth_bleMallocScanStateParams();
    if (scan_info->scan)
        state->scanState = STATE_SCANING;
    else
        state->scanState = STATE_NON_SCAN;

    FeaturePromiseResolve(handle, pid, state);
    FeatureFreeValue(state);
#endif
}

FtInt system_bluetooth_ble_Scanner_interface_scan_subscribeBLEDeviceFind(FeatureInterfaceHandle handle, AppendData append_data,
    system_bluetooth_ble_DeviceFindParams* params)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);
    scan_subscribe_info_t* subscribe_info;

    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scanner has been closed", __func__);
        return -1;
    }

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
#else
    return -1;
#endif
}

void system_bluetooth_ble_Scanner_interface_scan_unsubscribeBLEDeviceFind(FeatureInterfaceHandle handle, AppendData append_data, FtInt SubscribeId)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    scan_subscribe_info_t* subscribe_info;
    feature_bluetooth_scan_info_t* scan_info = FeatureGetObjectData(handle);
    if (!scan_info) {
        FEATURE_LOG_ERROR("%s, scanner has been closed", __func__);
        return;
    }

    subscribe_info = (scan_subscribe_info_t*)bt_list_find(scan_info->subscribe_info, scan_subscribe_info_cmp, &SubscribeId);
    if (!subscribe_info)
        return;

    FeatureRemoveCallback(handle, subscribe_info->callback);
    FeatureRemoveCallback(handle, subscribe_info->fail);
    bt_list_remove(scan_info->subscribe_info, subscribe_info);
#endif
}

void system_bluetooth_ble_Scanner_interface_scan_close(FeatureInterfaceHandle handle, AppendData append_data)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    feature_scan_destroy(handle);

    FeatureSetObjectData(handle, NULL);
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
typedef enum {
    FEATURE_GATT_STATE_DISCONNECTED,
    FEATURE_GATT_STATE_CONNECTING,
    FEATURE_GATT_STATE_CONNECTED,
    FEATURE_GATT_STATE_DISCONNECTING
} feature_gatt_state_t;

static void feature_notify_gatt_state_changed(FeatureInterfaceHandle handle, connection_state_t state, bt_address_t* addr)
{
    FtEventId event_id = FeatureGetEventId(handle, "onBLEConnectionStateChange");
    FtInt conn_state;

    if (!(FeatureGetEventCallbackCount(handle, event_id) > 0))
        return;

    switch (state) {
    case CONNECTION_STATE_DISCONNECTED:
        conn_state = FEATURE_GATT_STATE_DISCONNECTED;
        break;
    case CONNECTION_STATE_CONNECTING:
        conn_state = FEATURE_GATT_STATE_CONNECTING;
        break;
    case CONNECTION_STATE_DISCONNECTING:
        conn_state = FEATURE_GATT_STATE_DISCONNECTING;
        break;
    case CONNECTION_STATE_CONNECTED:
        conn_state = FEATURE_GATT_STATE_CONNECTED;
        break;
    default:
        return;
    }
    FeatureEmitEvent(handle, event_id, conn_state);
}

static void gattc_set_conn_state(feature_bluetooth_gattc_info_t* gattc_info, connection_state_t state)
{
    connection_state_t old_state = gattc_info->gattc->conn_state;

    if (old_state == state)
        return;

    FEATURE_LOG_INFO("gattc connect state changed from %d to %d", old_state, state);
    gattc_info->gattc->conn_state = state;
    feature_notify_gatt_state_changed(gattc_info->interface, state, &gattc_info->gattc->remote_address);
}

static void connect_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;

    FEATURE_LOG_ERROR("%s, connect failed, status: %d", __func__, status);

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_CONN);
    if (!data) {
        FEATURE_LOG_INFO("%s, data not found", __func__);

        if (status == GATT_STATUS_SUCCESS)
            gattc_set_conn_state(gattc_info, CONNECTION_STATE_CONNECTED);

        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        if (gattc_info->gattc->conn_state == CONNECTION_STATE_CONNECTING)
            gattc_set_conn_state(gattc_info, CONNECTION_STATE_DISCONNECTED);

        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc connect failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    gattc_set_conn_state(gattc_info, CONNECTION_STATE_CONNECTED);
    FEATURE_LOG_INFO("%s, connect success", __func__);
    FeaturePromiseResolve(data->interface, data->pid);
    bt_list_remove(gattc_info->userdata_list, data);
}

static void disconnect_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_DISCONN);
    if (!data) {
        FEATURE_LOG_INFO("%s, data not found", __func__);

        if (status == GATT_STATUS_SUCCESS)
            gattc_set_conn_state(gattc_info, CONNECTION_STATE_DISCONNECTED);

        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        if (gattc_info->gattc->conn_state == CONNECTION_STATE_DISCONNECTING)
            gattc_set_conn_state(gattc_info, CONNECTION_STATE_CONNECTED);

        FEATURE_LOG_ERROR("%s, disconnect failed, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc disconnect failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    gattc_set_conn_state(gattc_info, CONNECTION_STATE_DISCONNECTED);
    FeaturePromiseResolve(gattc_info->interface, data->pid);
    bt_list_remove(gattc_info->userdata_list, data);
}

static system_bluetooth_ble_BLEDescriptor* feature_get_descriptor_info(ft_context_ref ft_ctx, const gatt_descriptor_t* descriptor)
{
    system_bluetooth_ble_BLEDescriptor* feature_descriptor;

    if (!descriptor) {
        return NULL;
    }

    feature_descriptor = system_bluetooth_bleMallocBLEDescriptor();
    feature_descriptor->serviceUuid = bt_uuid_to_feature_string(&descriptor->service_uuid);
    feature_descriptor->characteristicUuid = bt_uuid_to_feature_string(&descriptor->characteristic_uuid);

    feature_descriptor->descriptorUuid = bt_uuid_to_feature_string(&descriptor->uuid);
    feature_descriptor->descriptorValue = (FtAny)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *feature_descriptor->descriptorValue = ft_from_buffer(ft_ctx, descriptor->value, descriptor->value_len);

    return feature_descriptor;
}

static system_bluetooth_ble_BLECharacteristic* feature_get_characteristic_info(ft_context_ref ft_ctx, const gatt_characteristic_t* characteristic)
{
    system_bluetooth_ble_BLECharacteristic* feature_characteristic;
    system_bluetooth_ble_GattProperties* properties;
    FtArray* descriptor_array;

    feature_characteristic = system_bluetooth_bleMallocBLECharacteristic();
    feature_characteristic->characteristicUuid = bt_uuid_to_feature_string(&characteristic->uuid);
    feature_characteristic->serviceUuid = bt_uuid_to_feature_string(&characteristic->service_uuid);

    feature_characteristic->characteristicValue = (FtAny)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *feature_characteristic->characteristicValue = ft_from_buffer(ft_ctx, characteristic->value, characteristic->value_len);

    descriptor_array = system_bluetooth_ble_malloc_BLEDescriptor_struct_type_array();
    descriptor_array->_size = characteristic->descriptor_count;
    descriptor_array->_element = calloc(characteristic->descriptor_count, sizeof(system_bluetooth_ble_BLEDescriptor*));
    for (int i = 0; i < characteristic->descriptor_count; i++) {
        ((system_bluetooth_ble_BLEDescriptor**)descriptor_array->_element)[i] = feature_get_descriptor_info(ft_ctx, &characteristic->descriptors[i]);
    }

    feature_characteristic->descriptors = descriptor_array;

    properties = system_bluetooth_bleMallocGattProperties();

    properties->read = characteristic->properties & GATT_PROP_READ;
    properties->write = characteristic->properties & GATT_PROP_WRITE;
    properties->writeNoResponse = characteristic->properties & GATT_PROP_WRITE_NR;
    properties->notify = characteristic->properties & GATT_PROP_NOTIFY;
    properties->indicate = characteristic->properties & GATT_PROP_INDICATE;
    feature_characteristic->properties = properties;

    return feature_characteristic;
}

static system_bluetooth_ble_GattService* feature_get_service_info(ft_context_ref ft_ctx, const gatt_service_t* service)
{
    FtArray* characteristics_array;
    system_bluetooth_ble_GattService* feature_service;

    feature_service = system_bluetooth_bleMallocGattService();
    feature_service->serviceUuid = bt_uuid_to_feature_string(&service->uuid);
    feature_service->isPrimary = service->is_primary;

    characteristics_array = system_bluetooth_ble_malloc_BLECharacteristic_struct_type_array();
    characteristics_array->_size = service->characteristic_count;
    characteristics_array->_element = calloc(service->characteristic_count, sizeof(system_bluetooth_ble_BLECharacteristic*));
    for (uint8_t i = 0; i < service->characteristic_count; i++) {
        ((system_bluetooth_ble_BLECharacteristic**)characteristics_array->_element)[i] = feature_get_characteristic_info(ft_ctx, &service->characteristics[i]);
    }

    feature_service->characteristics = characteristics_array;
    feature_service->includeServices = NULL;

    // Recursive nesting of GattService is not handled within this function.

    return feature_service;
}

static system_bluetooth_ble_GattService* feature_get_include_service_info(ft_context_ref ft_ctx, const gatt_include_service_t* include_service)
{
    system_bluetooth_ble_GattService* feature_innclude_service;

    feature_innclude_service = system_bluetooth_bleMallocGattService();
    memset(feature_innclude_service, 0, sizeof(system_bluetooth_ble_GattService));
    feature_innclude_service->serviceUuid = bt_uuid_to_feature_string(&include_service->included_service_uuid);

    return feature_innclude_service;
}

void feature_free_characteristic(ft_context_ref ft_ctx, system_bluetooth_ble_BLECharacteristic* feature_characteristic)
{
    system_bluetooth_ble_BLEDescriptor* feature_descriptor;

    if (!feature_characteristic)
        return;

    // free own characteristicValue
    ft_free_value(ft_ctx, *feature_characteristic->characteristicValue);

    // free descriptorValue of every descriptor
    for (int i = 0; i < feature_characteristic->descriptors->_size; i++) {
        feature_descriptor = ((system_bluetooth_ble_BLEDescriptor**)feature_characteristic->descriptors->_element)[i];
        ft_free_value(ft_ctx, *feature_descriptor->descriptorValue);
    }
}

void feature_free_service(ft_context_ref ft_ctx, system_bluetooth_ble_GattService* feature_service)
{
    if (!feature_service)
        return;

    system_bluetooth_ble_BLECharacteristic* feature_characteristic;
    int characteristic_count = feature_service->characteristics->_size;

    for (int i = 0; i < characteristic_count; i++) {
        feature_characteristic = ((system_bluetooth_ble_BLECharacteristic**)feature_service->characteristics->_element)[i];
        feature_free_characteristic(ft_ctx, feature_characteristic);
    }
}

static void discover_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle,
    const gatt_service_t* service[], size_t count)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;
    FtArray* feature_service_array;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_DISCOVERY);
    if (!data) {
        FEATURE_LOG_ERROR("%s, data not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, get service failed, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc get service failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    ft_context_ref ft_ctx = FeatureGetContext(data->interface);

    feature_service_array = system_bluetooth_ble_malloc_GattService_struct_type_array();
    feature_service_array->_size = count;
    feature_service_array->_element = calloc(feature_service_array->_size, sizeof(system_bluetooth_ble_GattService*));

    // service == NULL indicates the end of reporting
    for (int index = 0; index < count; index++) {
        system_bluetooth_ble_GattService* feature_service = feature_get_service_info(ft_ctx, service[index]);

        FtArray* include_service_array = system_bluetooth_ble_malloc_GattService_struct_type_array();
        include_service_array->_size = service[index]->included_service_count;
        include_service_array->_element = calloc(service[index]->included_service_count, sizeof(system_bluetooth_ble_GattService*));
        for (uint8_t i = 0; i < service[index]->included_service_count; i++) {
            ((system_bluetooth_ble_GattService**)include_service_array->_element)[i] = feature_get_include_service_info(ft_ctx, &service[index]->included_services[i]);
            // GattService nests up to one level, so any inner GattService does not nest further.
            ((system_bluetooth_ble_GattService**)include_service_array->_element)[i]->includeServices = NULL;
        }

        feature_service->includeServices = include_service_array;

        ((system_bluetooth_ble_GattService**)feature_service_array->_element)[index] = feature_service;
    }

    FEATURE_LOG_INFO("%s, get service success", __func__);
    FeaturePromiseResolve(data->interface, data->pid, feature_service_array);

    // for every outer feature_service in feature_service_array
    for (int k = 0; k < feature_service_array->_size; k++) {
        system_bluetooth_ble_GattService* feature_service_element = ((system_bluetooth_ble_GattService**)feature_service_array->_element)[k];
        int included_service_count = feature_service_element->includeServices->_size;

        for (int i = 0; i < included_service_count; i++) {
            system_bluetooth_ble_GattService* feature_include_service;
            feature_include_service = ((system_bluetooth_ble_GattService**)feature_service_element->includeServices->_element)[i];
            feature_free_service(ft_ctx, feature_include_service);
        }

        feature_free_service(ft_ctx, feature_service_element);
    }

    FeatureFreeValue(feature_service_array);
    bt_list_remove(gattc_info->userdata_list, data);

    return;
}

static void read_char_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;
    system_bluetooth_ble_BLECharacteristic* feature_characteristic;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_READ_CHAR);
    if (!data) {
        FEATURE_LOG_ERROR("%s, data not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, read characteristic failed, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc read characteristic failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    ft_context_ref ft_ctx = FeatureGetContext(data->interface);
    feature_characteristic = feature_get_characteristic_info(ft_ctx, characteristic);

    FeaturePromiseResolve(data->interface, data->pid, feature_characteristic);

    feature_free_characteristic(ft_ctx, feature_characteristic);
    FeatureFreeValue(feature_characteristic);
    bt_list_remove(gattc_info->userdata_list, data);
}

static void read_desc_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle,
    const gatt_descriptor_t* descriptor)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;
    system_bluetooth_ble_BLEDescriptor* feature_descriptor;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_READ_DESC);
    if (!data) {
        FEATURE_LOG_ERROR("%s, data not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, read descriptor, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc read descriptor failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    ft_context_ref ft_ctx = FeatureGetContext(data->interface);
    feature_descriptor = feature_get_descriptor_info(ft_ctx, descriptor);

    FeaturePromiseResolve(data->interface, data->pid, feature_descriptor);

    ft_free_value(ft_ctx, *feature_descriptor->descriptorValue);
    FeatureFreeValue(feature_descriptor);
    bt_list_remove(gattc_info->userdata_list, data);
}

static void write_char_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_WRITE_CHAR);
    if (!data) {
        FEATURE_LOG_ERROR("%s, data not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, write characteristic, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc write characteristic failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    FeaturePromiseResolve(gattc_info->interface, data->pid);
    bt_list_remove(gattc_info->userdata_list, data);
}

static void write_desc_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_WRITE_DESC);
    if (!data) {
        FEATURE_LOG_ERROR("%s, data not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, write descriptor, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc write descriptor failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    FeaturePromiseResolve(gattc_info->interface, data->pid);
    bt_list_remove(gattc_info->userdata_list, data);
}

static void subscribe_complete_callback(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle, bool enable)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_SET_NOTIFY);
    if (!data) {
        FEATURE_LOG_ERROR("%s, data not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, subscribe, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc subscribe failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    FeaturePromiseResolve(gattc_info->interface, data->pid);
    bt_list_remove(gattc_info->userdata_list, data);
}

static void notify_received_callback(bt_instance_t* ins, gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    system_bluetooth_ble_BLECharacteristic* feature_characteristic;

    FIND_INFO_BY_OBJECT(ins, conn_handle, gattc, gattc_info);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    FtEventId event_id = FeatureGetEventId(gattc_info->interface, "onBLECharacteristicChange");
    if (!(FeatureGetEventCallbackCount(gattc_info->interface, event_id) > 0))
        return;

    ft_context_ref ft_ctx = FeatureGetContext(gattc_info->interface);
    feature_characteristic = feature_get_characteristic_info(ft_ctx, characteristic);
    FeatureEmitEvent(gattc_info->interface, event_id, feature_characteristic);

    feature_free_characteristic(ft_ctx, feature_characteristic);
    FeatureFreeValue(feature_characteristic);
}

static void mtu_updated_callback(gattc_handle_t conn_handle, gatt_status_t status, uint32_t mtu)
{
    feature_bluetooth_gattc_info_t* gattc_info;
    gattc_data_t* data = NULL;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    bt_instance_t* bluetooth_instance = gattc_remote->ins;

    FIND_INFO_BY_OBJECT(bluetooth_instance, conn_handle, gattc, gattc_info);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc_info not found", __func__);
        return;
    }

    data = (gattc_data_t*)bt_list_find(gattc_info->userdata_list, gattc_userdata_type_cmp, (void*)FEATURE_GATTC_SET_MTU);
    if (!data) {
        FEATURE_LOG_ERROR("%s, data not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, mtu, status: %d", __func__, status);
        FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc mtu failed!");
        bt_list_remove(gattc_info->userdata_list, data);
        return;
    }

    FeaturePromiseResolve(gattc_info->interface, data->pid);
    bt_list_remove(gattc_info->userdata_list, data);
}

static bt_gattc_feature_callbacks_t gattc_cbs = {
    sizeof(gattc_cbs),
    .on_connected = connect_callback,
    .on_disconnected = disconnect_callback,
    .on_discovered = discover_callback,
    .on_read_char = read_char_callback,
    .on_read_desc = read_desc_callback,
    .on_write_char = write_char_callback,
    .on_write_desc = write_desc_callback,
    .on_subscribed = subscribe_complete_callback,
    .on_notified = notify_received_callback,
    .on_mtu_updated = mtu_updated_callback,
};
#endif

FeatureInterfaceHandle system_bluetooth_ble_wrap_createGattClientDevice(FeatureInstanceHandle feature,
    AppendData append_data, FtString deviceId, FtString addressType)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_instance_t* bluetooth_instance = feature_bluetooth_get_bt_ins(feature);
    feature_bluetooth_features_info_t* features_info = (feature_bluetooth_features_info_t*)(bluetooth_instance->context);
    feature_bluetooth_gattc_info_t* gattc_info = (feature_bluetooth_gattc_info_t*)calloc(1, sizeof(feature_bluetooth_gattc_info_t));

    gattc_info->ins = bluetooth_instance;
    gattc_info->gattc = (gattc_t*)calloc(1, sizeof(gattc_t));

    if (!deviceId || !addressType)
        goto error;

    if (bt_addr_str2ba(deviceId, &gattc_info->gattc->remote_address) < 0)
        goto error;

    if (!strncmp(addressType, "PUBLIC", strlen("PUBLIC")))
        gattc_info->gattc->addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    else if (!strncmp(addressType, "RANDOM", strlen("RANDOM")))
        gattc_info->gattc->addr_type = BT_LE_ADDR_TYPE_RANDOM;
    else if (!strncmp(addressType, "ANONYMOUS", strlen("ANONYMOUS")))
        gattc_info->gattc->addr_type = BT_LE_ADDR_TYPE_ANONYMOUS;
    else
        gattc_info->gattc->addr_type = BT_LE_ADDR_TYPE_UNKNOWN;

    FeatureInterfaceHandle handle = system_bluetooth_ble_createGattClientDevice_instance(feature);
    FEATURE_LOG_INFO("%s::%s(), FeatureInstanceHandle: %p, FeatureInterfaceHandle: %p\n", file_tag, __FUNCTION__, feature, handle);

    gattc_info->interface = handle;
    gattc_info->userdata_list = bt_list_new((bt_list_free_cb_t)feature_ble_list_free);

    bt_list_add_tail(features_info->feature_ble_gattc, gattc_info);
    FeatureSetObjectData(handle, gattc_info);

    return handle;

error:
    free(gattc_info->gattc);
    free(gattc_info);
    return NULL;
#else
    return NULL;
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void feature_gattc_destroy(FeatureInterfaceHandle handle)
{
    feature_bluetooth_gattc_info_t* gattc_info = (feature_bluetooth_gattc_info_t*)FeatureGetObjectData(handle);
    if (gattc_info == NULL)
        return;

    bt_instance_t* bluetooth_instance = gattc_info->ins;
    feature_bluetooth_features_info_t* features_info = (feature_bluetooth_features_info_t*)(bluetooth_instance->context);

    if (gattc_info->gattc->handle) {
        FEATURE_LOG_INFO("%s::%s(), stop advertising\n", file_tag, __FUNCTION__);
        if (gattc_info->gattc->conn_state == CONNECTION_STATE_CONNECTED) {
            bt_gattc_feature_disconnect_async(gattc_info->gattc->handle, NULL, NULL);
        }

        bt_gattc_feature_delete_client_async(bluetooth_instance, gattc_info->gattc->handle, NULL, NULL);
    }

    free(gattc_info->gattc);
    bt_list_free(gattc_info->userdata_list);
    bt_list_remove(features_info->feature_ble_gattc, gattc_info);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_finalize(FeatureInterfaceHandle handle)
{
#ifdef CONFIG_BLUETOOTH_GATT
    feature_gattc_destroy(handle);
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_connect_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    if (gattc_info->gattc->conn_state == CONNECTION_STATE_CONNECTING)
        gattc_set_conn_state(gattc_info, CONNECTION_STATE_DISCONNECTED);

    FEATURE_LOG_ERROR("%s, connect failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc connect failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}

static void gattc_create_cb(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;
    bt_status_t ret;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != GATT_STATUS_SUCCESS || !conn_handle) {
        goto error;
    }

    gattc_info->created = true;
    gattc_info->gattc->handle = conn_handle;
    FEATURE_LOG_INFO("%s, create connect success", __func__);

    ret = bt_gattc_feature_connect_async(gattc_info->gattc->handle, &gattc_info->gattc->remote_address,
        gattc_info->gattc->addr_type, gattc_connect_cb, (void*)data);
    if (ret == BT_STATUS_SUCCESS) {
        return;
    }

    status = ret;

error:
    if (gattc_info->gattc->conn_state == CONNECTION_STATE_CONNECTING)
        gattc_set_conn_state(gattc_info, CONNECTION_STATE_DISCONNECTED);

    FEATURE_LOG_ERROR("%s, create connect failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc create connect failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_connect(FeatureInterfaceHandle handle, AppendData append_data, FtPromiseId pid)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status;
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    status = BT_STATUS_FAIL;
    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_DISCONNECTED) {
        FEATURE_LOG_ERROR("%s, Repeated Attempt", __func__);
        status = BT_STATUS_DONE;
        goto error;
    }

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->userdata_type = FEATURE_GATTC_CONN;
    data->pid = pid;
    bt_list_add_tail(gattc_info->userdata_list, data);

    if (gattc_info->created) {
        if (!gattc_info->gattc->handle) {
            FEATURE_LOG_ERROR("%s, not create connect", __func__);
            status = BT_STATUS_FAIL;
            goto error;
        }
        status = bt_gattc_feature_connect_async(gattc_info->gattc->handle, &gattc_info->gattc->remote_address,
            gattc_info->gattc->addr_type, gattc_connect_cb, (void*)data);
    } else {
        status = bt_gattc_feature_create_client_async(gattc_info->ins, &gattc_info->gattc->remote_address, gattc_create_cb,
            &gattc_cbs, (void*)data);
    }

    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, connect failed, status: %d", __func__, status);
        goto error;
    }

    gattc_set_conn_state(gattc_info, CONNECTION_STATE_CONNECTING);
    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc connect failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_disconnect_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status == BT_STATUS_SUCCESS)
        return;

    if (gattc_info->gattc->conn_state == CONNECTION_STATE_DISCONNECTING)
        gattc_set_conn_state(gattc_info, CONNECTION_STATE_CONNECTED);

    FEATURE_LOG_ERROR("%s, disconnect failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc disconnect failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_disconnect(FeatureInterfaceHandle handle, AppendData append_data, FtPromiseId pid)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status = BT_STATUS_FAIL;
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state == CONNECTION_STATE_DISCONNECTED) {
        FeaturePromiseResolve(handle, pid);
        return;
    } else if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, gattc not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle is NULL", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_DISCONN;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_disconnect_async(gattc_info->gattc->handle, gattc_disconnect_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, disconnect failed, status: %d", __func__, status);
        goto error;
    }

    gattc_set_conn_state(gattc_info, CONNECTION_STATE_DISCONNECTING);
    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc disconnect failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
// continuously reported
static void gattc_get_service_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    FEATURE_LOG_ERROR("%s, get service failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc get service failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_getServices(FeatureInterfaceHandle handle, AppendData append_data, FtPromiseId pid)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status = BT_STATUS_FAIL;
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, gattc not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle is null", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_DISCOVERY;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_get_service_async(gattc_info->gattc->handle, gattc_get_service_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, get service failed, status: %d", __func__, status);
        goto error;
    }

    return;

error:
    if (data) {
        bt_list_remove(gattc_info->userdata_list, data);
    }

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc get service failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_read_characteristic_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    FEATURE_LOG_ERROR("%s, read characteristic failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc read characteristic failed!");
    bt_list_remove(gattc_info->userdata_list, data);
    return;
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_readCharacteristicValue(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_ReadCharacteristicValue* params)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status;
    uint8_t uuid128[16];
    bt_uuid_t service_uuid;
    bt_uuid_t characteristic_uuid;
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    status = BT_STATUS_FAIL;
    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, gattc not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle is NULL", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (get_valid_uuid128(uuid128, params->characteristic->characteristicUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Characteristic UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&characteristic_uuid, uuid128);

    if (get_valid_uuid128(uuid128, params->characteristic->serviceUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Service UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&service_uuid, uuid128);

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_READ_CHAR;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_read_characteristic_value_async(gattc_info->gattc->handle, &service_uuid, &characteristic_uuid, gattc_read_characteristic_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, read characteristic failed, status: %d", __func__, status);
        goto error;
    }

    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc write characteristic failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_read_descriptor_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    FEATURE_LOG_ERROR("%s, read descriptor, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc read descriptor failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_readDescriptorValue(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_ReadDescriptorValue* params)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status;
    uint8_t uuid128[16];
    bt_uuid_t service_uuid;
    bt_uuid_t characteristic_uuid;
    bt_uuid_t descriptor_uuid;
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    status = BT_STATUS_FAIL;
    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, gattc not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle is NULL", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (get_valid_uuid128(uuid128, params->descriptor->characteristicUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Descriptor UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&characteristic_uuid, uuid128);

    if (get_valid_uuid128(uuid128, params->descriptor->serviceUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Service UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&service_uuid, uuid128);

    if (get_valid_uuid128(uuid128, params->descriptor->descriptorUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Service UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&descriptor_uuid, uuid128);

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_READ_DESC;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_read_descriptor_value_async(gattc_info->gattc->handle, &service_uuid, &characteristic_uuid, &descriptor_uuid, gattc_read_descriptor_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, read descriptor failed, status: %d", __func__, status);
        goto error;
    }

    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc read descriptor failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_write_char_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    FEATURE_LOG_ERROR("%s, write characteristic failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc write characteristic failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_writeCharacteristicValue(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_WriteCharacteristicValue* params)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status;
    uint8_t uuid128[16];
    gatt_characteristic_t characteristic = { 0 };
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;
    ft_context_ref ft_ctx = FeatureGetContext(handle);

    status = BT_STATUS_FAIL;
    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    FtArray* descriptor_array = params->characteristic->descriptors;
    int descriptor_len = descriptor_array->_size;
    gatt_descriptor_t descriptor[descriptor_len];
    memset(descriptor, 0, sizeof(descriptor) * descriptor_len);

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle is NULL", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (get_valid_uuid128(uuid128, params->characteristic->characteristicUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Characteristic UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&characteristic.uuid, uuid128);

    if (get_valid_uuid128(uuid128, params->characteristic->serviceUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Service UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&characteristic.service_uuid, uuid128);

    characteristic.descriptors = descriptor;
    characteristic.value = ft_to_buffer(ft_ctx, &characteristic.value_len, *params->characteristic->characteristicValue);

    for (int i = 0; i < descriptor_len; i++) {
        if (get_valid_uuid128(uuid128, ((system_bluetooth_ble_BLEDescriptor**)descriptor_array->_element)[i]->descriptorUuid)) {
            FEATURE_LOG_ERROR("%s, Invalid Descriptor UUID", __func__);
            status = BT_STATUS_PARM_INVALID;
            goto error;
        }
        bt_uuid128_create(&descriptor[i].uuid, uuid128);
        memcpy(&descriptor[i].service_uuid, &characteristic.service_uuid, sizeof(bt_uuid_t));
        memcpy(&descriptor[i].characteristic_uuid, &characteristic.uuid, sizeof(bt_uuid_t));

        descriptor[i].value = ft_to_buffer(ft_ctx, &descriptor[i].value_len, *((system_bluetooth_ble_BLEDescriptor**)descriptor_array->_element)[i]->descriptorValue);
    }

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_WRITE_CHAR;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_write_characteristic_value_async(gattc_info->gattc->handle, &characteristic, gattc_write_char_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, write characteristic failed, status: %d", __func__, status);
        goto error;
    }

    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc write characteristic failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_write_desc_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    FEATURE_LOG_ERROR("%s, write descriptor failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc write descriptor failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_writeDescriptorValue(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_WriteDescriptorValue* params)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status;
    uint8_t uuid128[16];
    size_t length;
    gatt_descriptor_t descriptor = { 0 };
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    status = BT_STATUS_FAIL;
    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle is NULL", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (get_valid_uuid128(uuid128, params->descriptor->descriptorUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Descriptor UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&descriptor.uuid, uuid128);

    if (get_valid_uuid128(uuid128, params->descriptor->characteristicUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Characteristic UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&descriptor.characteristic_uuid, uuid128);

    if (get_valid_uuid128(uuid128, params->descriptor->serviceUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid Service UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&descriptor.service_uuid, uuid128);

    ft_context_ref ft_ctx = FeatureGetContext(handle);
    uint8_t* value = ft_to_buffer(ft_ctx, &length, *params->descriptor->descriptorValue);
    descriptor.value = value;
    descriptor.value_len = length;

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_WRITE_DESC;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_write_descriptor_value_async(gattc_info->gattc->handle, &descriptor, gattc_write_desc_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, write descriptor failed, status: %d", __func__, status);
        goto error;
    }

    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc write descriptor failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_set_mtu_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    FEATURE_LOG_ERROR("%s, gattc set mtu failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc set mtu!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_setBLEMtuSize(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_SetBLEMtuSize* params)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status;
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    status = BT_STATUS_FAIL;
    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, gattc not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle is NULL", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_SET_MTU;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_exchange_mtu_async(gattc_info->gattc->handle, (uint32_t)params->mtu, gattc_set_mtu_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, set mtu failed, status: %d", __func__, status);
        goto error;
    }

    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc set mtu failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

#ifdef CONFIG_BLUETOOTH_GATT
static void gattc_set_notify_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gattc_data_t* data = (gattc_data_t*)userdata;
    feature_bluetooth_gattc_info_t* gattc_info;

    gattc_info = find_gattc_info_by_userdata(ins, userdata);
    if (gattc_info == NULL) {
        FEATURE_LOG_ERROR("%s, gattc info not found", __func__);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        goto error;

    return;

error:
    FEATURE_LOG_ERROR("%s, set notify failed, status: %d", __func__, status);
    FeaturePromiseReject(data->interface, data->pid, bt_status_to_feature_error(status), "gattc set notify failed!");
    bt_list_remove(gattc_info->userdata_list, data);
}
#endif

void system_bluetooth_ble_GattClient_interface_gattc_setNotifyCharacteristicChanged(FeatureInterfaceHandle handle, AppendData append_data,
    FtPromiseId pid, system_bluetooth_ble_SetNotifyCharChangedParams* params)
{
#ifdef CONFIG_BLUETOOTH_GATT
    bt_status_t status;
    uint8_t uuid128[16];
    gatt_characteristic_t characteristic = { 0 };
    gattc_data_t* data = NULL;
    feature_bluetooth_gattc_info_t* gattc_info;

    status = BT_STATUS_FAIL;
    gattc_info = FeatureGetObjectData(handle);
    if (!gattc_info) {
        FEATURE_LOG_ERROR("%s, gattc has been closed", __func__);
        return;
    }

    if (gattc_info->gattc->conn_state != CONNECTION_STATE_CONNECTED) {
        FEATURE_LOG_ERROR("%s, gattc not connected", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (!gattc_info->gattc->handle) {
        FEATURE_LOG_ERROR("%s, gattc handle not found", __func__);
        status = BT_STATUS_FAIL;
        goto error;
    }

    if (get_valid_uuid128(uuid128, params->characteristic->serviceUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid service UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&characteristic.service_uuid, uuid128);

    if (get_valid_uuid128(uuid128, params->characteristic->characteristicUuid)) {
        FEATURE_LOG_ERROR("%s, Invalid characteristic UUID", __func__);
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }
    bt_uuid128_create(&characteristic.uuid, uuid128);

    data = (gattc_data_t*)calloc(1, sizeof(gattc_data_t));
    if (!data) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    data->interface = handle;
    data->pid = pid;
    data->userdata_type = FEATURE_GATTC_SET_NOTIFY;
    bt_list_add_tail(gattc_info->userdata_list, data);

    status = bt_gattc_feature_set_notify_characteristic_changed_async(gattc_info->gattc->handle, &characteristic, params->enable, gattc_set_notify_cb, data);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, set notify failed, status: %d", __func__, status);
        goto error;
    }

    return;

error:
    if (data)
        bt_list_remove(gattc_info->userdata_list, data);

    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(status), "gattc set notify failed!");
#else
    FeaturePromiseReject(handle, pid, bt_status_to_feature_error(BT_STATUS_FAIL), "gattc is not supported.");
#endif
}

FtBool system_bluetooth_ble_GattClient_interface_gattc_close(FeatureInterfaceHandle handle, AppendData append_data)
{
#ifdef CONFIG_BLUETOOTH_GATT
    feature_gattc_destroy(handle);
    FeatureSetObjectData(handle, NULL);
    return true;
#else
    return false;
#endif
}

static void aurasnk_pending_work_finished(void* data)
{
    feature_bluetooth_aurasnk_pending_work_t* work = data;

    if (work->status == BT_STATUS_SUCCESS) {
        FeaturePromiseResolve(work->handle, work->pid);
    } else {
        FeaturePromiseReject(work->handle, work->pid, bt_status_to_feature_error(work->status),
            "aurasnk work failed");
    }

    free(work);
}

FeatureInterfaceHandle system_bluetooth_ble_wrap_createAuracastSink(FeatureInstanceHandle feature,
    AppendData adata)
{
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    bt_instance_t* bluetooth_instance = feature_bluetooth_get_bt_ins(feature);
    feature_bluetooth_features_info_t* features_info = bluetooth_instance->context;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = zalloc(
        sizeof(feature_bluetooth_aurasnk_info_t));
    if (!aurasnk_info)
        return NULL;

    FeatureInterfaceHandle handle = system_bluetooth_ble_createAuracastSink_instance(feature);
    FEATURE_LOG_INFO("%s::%s(), FeatureInstanceHandle: %p, FeatureInterfaceHandle: %p\n", file_tag,
        __FUNCTION__, feature, handle);

    aurasnk_info->ins = bluetooth_instance;
    aurasnk_info->handle = handle;
    aurasnk_info->source_found_callback = FEATURE_BLE_FT_CALLBACK_ID_INVALID;
    aurasnk_info->stream_found_callback = FEATURE_BLE_FT_CALLBACK_ID_INVALID;
    aurasnk_info->stream_started_callback = FEATURE_BLE_FT_CALLBACK_ID_INVALID;
    aurasnk_info->stream_stopped_callback = FEATURE_BLE_FT_CALLBACK_ID_INVALID;
    aurasnk_info->pending_work = bt_list_new(aurasnk_pending_work_finished);
    if (!aurasnk_info->pending_work) {
        free(aurasnk_info);
        return NULL;
    }

    bt_list_add_tail(features_info->feature_ble_aurasnk, aurasnk_info);
    FeatureSetObjectData(handle, aurasnk_info);

    return handle;
#else
    return NULL;
#endif
}

#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
static void feature_auracast_sink_destroy(FeatureInterfaceHandle handle)
{
    bt_instance_t* ins;
    feature_bluetooth_features_info_t* features_info;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = FeatureGetObjectData(handle);
    if (aurasnk_info == NULL)
        return;

    ins = aurasnk_info->ins;
    features_info = ins->context;
    if (aurasnk_info->source_found_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID)
        FeatureRemoveCallback(handle, aurasnk_info->source_found_callback);

    if (aurasnk_info->stream_found_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID)
        FeatureRemoveCallback(handle, aurasnk_info->stream_found_callback);

    if (aurasnk_info->stream_started_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID)
        FeatureRemoveCallback(handle, aurasnk_info->stream_started_callback);

    if (aurasnk_info->stream_stopped_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID)
        FeatureRemoveCallback(handle, aurasnk_info->stream_stopped_callback);

    bt_list_free(aurasnk_info->pending_work);
    if (aurasnk_info->scanner && aurasnk_info->scanner != FEATURE_BLE_PTR_PENDING) {
        FEATURE_LOG_INFO("%s::%s(), stop scan", file_tag, __FUNCTION__);
        bt_le_stop_scan_async(ins, aurasnk_info->scanner, NULL, NULL);
        aurasnk_info->scanner = NULL;
    }

    if (aurasnk_info->stream_info) {
        FEATURE_LOG_INFO("%s::%s(), terminate sync", file_tag, __FUNCTION__);
        bt_auracast_sink_terminate_sync_async(ins, &aurasnk_info->stream_info->remote.addr,
            aurasnk_info->stream_info->remote.sid, NULL, NULL);
        bt_pa_sync_terminate_async(ins, &aurasnk_info->stream_info->remote.addr,
            aurasnk_info->stream_info->remote.sid, NULL, NULL);
        free(aurasnk_info->stream_info);
    }

    if (aurasnk_info->auracast_cbs_cookie
        && aurasnk_info->auracast_cbs_cookie != FEATURE_BLE_PTR_PENDING) {
        FEATURE_LOG_INFO("%s::%s(), stop receive", file_tag, __FUNCTION__);
        bt_auracast_sink_unregister_callbacks_async(ins, aurasnk_info->auracast_cbs_cookie, NULL,
            NULL);
        aurasnk_info->auracast_cbs_cookie = NULL;
    }

    bt_list_remove(features_info->feature_ble_aurasnk, aurasnk_info);
}
#endif /** CONFIG_BLUETOOTH_AURACAST_SINK */

void system_bluetooth_ble_AuracastSink_interface_aurasnk_finalize(FeatureInterfaceHandle handle)
{
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    feature_auracast_sink_destroy(handle);
#endif
}

#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
static char* aurasnk_build_id(const bt_address_t* addr, uint8_t addr_type, uint8_t sid)
{
    char* id = zalloc(FEATURE_BLE_FT_STRING_MAX);
    if (!id)
        return NULL;

    snprintf(id, FEATURE_BLE_FT_STRING_MAX, "aurasnk_%02x%02x%02x%02x%02x%02x(t%d)_sid%d",
        addr->addr[5], addr->addr[4], addr->addr[3], addr->addr[2], addr->addr[1], addr->addr[0],
        addr_type, sid);

    return id;
}

static bt_status_t aurasnk_parse_id(bt_le_address_t* addr, uint8_t* sid, const char* id)
{
    int parsed;

    if (!id)
        return BT_STATUS_PARM_INVALID;

    parsed = sscanf(id, "aurasnk_%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx(t%hhd)_sid%hhd",
        &addr->addr[5], &addr->addr[4], &addr->addr[3], &addr->addr[2], &addr->addr[1],
        &addr->addr[0], &addr->addr_type, sid);

    return (parsed == 8) ? BT_STATUS_SUCCESS : BT_STATUS_PARM_INVALID;
}

static char* aurasnk_scan_build_display_name(const ble_scan_result_t* result)
{
    bt_status_t status;
    char* display_name = NULL;
    bt_pa_sync_info_t* info = NULL;

    display_name = zalloc(FEATURE_BLE_FT_STRING_MAX);
    if (!display_name)
        goto error;

    info = malloc(sizeof(bt_pa_sync_info_t));
    if (info == NULL)
        goto error;

    status = bt_pa_sync_parse_adv_data(info, result);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    if (info->broadcast_name[0] != '\0') {
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "%s\n", info->broadcast_name);
    } else if (info->name[0] != '\0') {
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "%s\n", info->name);
    } else {
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "<unknown>\n");
    }

    if (result->sid != BLE_SCAN_SID_NOT_PROVIDED)
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "[%d]", result->sid);

    if (result->rssi != BT_POWER_UNAVAILABLE)
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "[%ddBm]", result->rssi);

    if (info->broadcast_id != BT_INVALID_BROADCAST_ID)
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "[0x%" PRIx32 "]",
            info->broadcast_id);

    free(info);

    return display_name;

error:
    free(display_name);
    free(info);

    return NULL;
}

static void aurasnk_on_scan_result(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    bt_instance_t* ins;
    feature_bluetooth_features_info_t* features_info;
    feature_bluetooth_aurasnk_info_t* aurasnk_info;
    system_bluetooth_ble_AuracastScanResult* result_data = NULL;
    FtArray* result_array = NULL;
    char* display_name = NULL;
    char* id = NULL;

    /** FIXME: scanner might be invalid if aurasnk_info is removed */
    ins = ((bt_scan_remote_t*)scanner)->ins;
    features_info = ins->context;
    if (!features_info || !features_info->feature_ble_aurasnk)
        return;

    aurasnk_info = bt_list_find(features_info->feature_ble_aurasnk, aurasnk_scanner_cmp, scanner);
    if (!aurasnk_info)
        return;

    if (aurasnk_info->source_found_callback == FEATURE_BLE_FT_CALLBACK_ID_INVALID)
        return;

    display_name = aurasnk_scan_build_display_name(result);
    if (!display_name)
        return;

    id = aurasnk_build_id(&result->addr, result->addr_type, result->sid);
    if (!id) {
        free(display_name);
        return;
    }

    result_array = system_bluetooth_ble_malloc_AuracastScanResult_struct_type_array();
    result_array->_size = 1;
    result_array->_element = calloc(result_array->_size,
        sizeof(system_bluetooth_ble_AuracastScanResult*));

    result_data = system_bluetooth_bleMallocAuracastScanResult();
    ((system_bluetooth_ble_AuracastScanResult**)result_array->_element)[0] = result_data;
    result_data->rssi = result->rssi;
    result_data->id = StringToFtString(id);
    result_data->displayName = StringToFtString(display_name);

    FeatureInvokeCallback(aurasnk_info->handle, aurasnk_info->source_found_callback, result_array);

    FeatureFreeValue(result_array);
    free(display_name);
    free(id);
}

static void aurasnk_on_scan_status(bt_scanner_t* scanner, uint8_t status)
{
    bt_instance_t* ins;
    feature_bluetooth_features_info_t* features_info;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = NULL;
    feature_bluetooth_aurasnk_pending_work_t* work = NULL;
    aurasnk_work_type_t type = AURASNK_WORK_TYPE_START_SCAN;

    /** FIXME: scanner might be invalid if aurasnk_info is removed */
    ins = ((bt_scan_remote_t*)scanner)->ins;
    features_info = ins->context;
    if (!features_info || !features_info->feature_ble_aurasnk)
        goto error;

    aurasnk_info = bt_list_find(features_info->feature_ble_aurasnk, aurasnk_scanner_cmp, scanner);
    if (!aurasnk_info)
        goto error;

    if (status != BT_STATUS_SUCCESS)
        aurasnk_info->scanner = NULL;

    work = bt_list_find(aurasnk_info->pending_work, aurasnk_work_cmp, &type);
    if (!work)
        goto error;

    work->status = status;
    bt_list_remove(aurasnk_info->pending_work, work);

    if (status == BT_STATUS_SUCCESS) {
        FEATURE_LOG_DEBUG("%s, scan started", __func__);
    } else {
        FEATURE_LOG_ERROR("%s, failed to start scan, status = %d", __func__, status);
    }

    return;

error:
    if (status == BT_STATUS_SUCCESS) {
        /** scanner is valid but unexpected */
        if (scanner)
            bt_le_stop_scan_async(ins, scanner, NULL, NULL);
    }

    if (aurasnk_info)
        bt_list_remove(aurasnk_info->pending_work, work);
}

static void aurasnk_on_scan_stopped(bt_scanner_t* scanner)
{
    bt_instance_t* ins;
    feature_bluetooth_features_info_t* features_info;
    feature_bluetooth_aurasnk_info_t* aurasnk_info;

    /** FIXME: scanner might be invalid if aurasnk_info is removed */
    ins = ((bt_scan_remote_t*)scanner)->ins;
    features_info = ins->context;
    if (!features_info || !features_info->feature_ble_aurasnk)
        return;

    aurasnk_info = bt_list_find(features_info->feature_ble_aurasnk, aurasnk_scanner_cmp, scanner);
    if (!aurasnk_info)
        return;

    if (aurasnk_info->source_found_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID) {
        FeatureRemoveCallback(aurasnk_info->handle, aurasnk_info->source_found_callback);
        aurasnk_info->source_found_callback = FEATURE_BLE_FT_CALLBACK_ID_INVALID;
    }

    aurasnk_info->scanner = NULL;
}

static const ble_scan_settings_t aurasnk_scan_settings = {
    .scan_mode = BT_SCAN_MODE_LOW_LATENCY,
    .legacy = false,
    .scan_type = BT_LE_SCAN_TYPE_PASSIVE,
    .scan_phy = BT_LE_1M_PHY,
    .policy.policy = 0, /**< Unfiltered */
};

static const scanner_callbacks_t aurasnk_scan_cbs = {
    .size = sizeof(aurasnk_scan_cbs),
    .on_scan_result = aurasnk_on_scan_result,
    .on_scan_start_status = aurasnk_on_scan_status,
    .on_scan_stopped = aurasnk_on_scan_stopped,
};

static void aurasnk_start_scan_cb(bt_instance_t* ins, bt_status_t status, void* scan,
    void* userdata)
{
    feature_bluetooth_aurasnk_info_t* aurasnk_info;
    feature_bluetooth_aurasnk_pending_work_t* work = NULL;
    aurasnk_work_type_t type = AURASNK_WORK_TYPE_START_SCAN;

    FIND_INFO_BY_OBJECT(ins, userdata, aurasnk, aurasnk_info);
    if (!aurasnk_info) {
        FEATURE_LOG_ERROR("%s, aurasnk_info not found", __func__);
        goto error;
    }

    work = bt_list_find(aurasnk_info->pending_work, aurasnk_work_cmp, &type);
    if (!work)
        goto error;

    if (aurasnk_info->scanner != FEATURE_BLE_PTR_PENDING) {
        FEATURE_LOG_ERROR("%s, unexpected scanner", __func__);
        goto error;
    }

    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, failed to start scan, status = %d", __func__, status);
        work->status = status;
        goto error;
    }

    aurasnk_info->scanner = scan;
    if (aurasnk_info->scanner == NULL) {
        FEATURE_LOG_ERROR("%s, scan not started", __func__);
        goto error;
    }

    FEATURE_LOG_DEBUG("%s, scan starting..", __func__);
    return;

error:
    if (aurasnk_info) {
        bt_list_remove(aurasnk_info->pending_work, work);
        if (aurasnk_info->scanner == FEATURE_BLE_PTR_PENDING)
            aurasnk_info->scanner = NULL;
    }

    if (status == BT_STATUS_SUCCESS && scan != NULL)
        bt_le_stop_scan_async(ins, scan, NULL, NULL);
}
#endif /** CONFIG_BLUETOOTH_AURACAST_SINK */

void system_bluetooth_ble_AuracastSink_interface_aurasnk_startScan(FeatureInterfaceHandle handle,
    AppendData adata, FtPromiseId __pid__, system_bluetooth_ble_StartAuracastScanParams* params)
{
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    bt_status_t status;
    ble_scan_settings_t settings;
    feature_bluetooth_aurasnk_pending_work_t* work = NULL;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = FeatureGetObjectData(handle);

    if (params->callback == FEATURE_BLE_FT_CALLBACK_ID_INVALID) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    if (!aurasnk_info) {
        FEATURE_LOG_ERROR("%s, not initialized", __func__);
        status = BT_STATUS_NOT_READY;
        goto error;
    }

    if (aurasnk_info->scanner != NULL
        || aurasnk_info->source_found_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID) {
        FEATURE_LOG_ERROR("%s, repeated attempt", __func__);
        status = BT_STATUS_DONE;
        goto error;
    }

    work = zalloc(sizeof(feature_bluetooth_aurasnk_pending_work_t));
    if (!work) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    memcpy(&settings, &aurasnk_scan_settings, sizeof(ble_scan_settings_t));
    status = bt_le_start_scan_settings_async(aurasnk_info->ins, &settings, &aurasnk_scan_cbs,
        aurasnk_start_scan_cb, aurasnk_info);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    aurasnk_info->scanner = FEATURE_BLE_PTR_PENDING;
    aurasnk_info->source_found_callback = params->callback;
    work->type = AURASNK_WORK_TYPE_START_SCAN;
    work->handle = handle;
    work->pid = __pid__;
    work->status = BT_STATUS_FAIL; /**< Always marked as failed before done */
    bt_list_add_tail(aurasnk_info->pending_work, work);
    FEATURE_LOG_DEBUG("%s, scan starting.", __func__);

    return;

error:
    FeaturePromiseReject(handle, __pid__, bt_status_to_feature_error(status),
        "failed to start scan");
    free(work);
#else
    FeaturePromiseReject(handle, __pid__, bt_status_to_feature_error(BT_STATUS_NOT_SUPPORTED),
        "auracast sink is not supported");
#endif
}

void system_bluetooth_ble_AuracastSink_interface_aurasnk_stopScan(FeatureInterfaceHandle handle,
    AppendData adata)
{
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    bt_status_t status;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = FeatureGetObjectData(handle);

    if (!aurasnk_info) {
        FEATURE_LOG_ERROR("%s, not initialized", __func__);
        return;
    }

    if (aurasnk_info->scanner == NULL) {
        FEATURE_LOG_ERROR("%s, nothing to stop", __func__);
        return;
    }

    if (aurasnk_info->scanner == FEATURE_BLE_PTR_PENDING) {
        FEATURE_LOG_ERROR("%s, scan is starting", __func__);
        return;
    }

    status = bt_le_stop_scan_async(aurasnk_info->ins, aurasnk_info->scanner, NULL, NULL);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, failed to stop scan, status = %d", __func__, status);
        return;
    }

    aurasnk_info->scanner = NULL;
    FEATURE_LOG_DEBUG("%s, scan stopping.", __func__);
#endif
}

#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
static void aurasnk_on_sync_established(const bt_le_address_t* addr, uint8_t sid, void* context)
{
    bt_instance_t* ins = context;
    feature_bluetooth_features_info_t* features_info;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = NULL;
    feature_bluetooth_aurasnk_pending_work_t* work = NULL;
    aurasnk_work_type_t type = AURASNK_WORK_TYPE_CREATE_SYNC;

    features_info = ins->context;
    if (!features_info || !features_info->feature_ble_aurasnk)
        goto error;

    aurasnk_info = bt_list_find(features_info->feature_ble_aurasnk, aurasnk_instance_cmp, ins);
    if (!aurasnk_info)
        goto error;

    work = bt_list_find(aurasnk_info->pending_work, aurasnk_work_cmp, &type);
    if (!work)
        goto error;

    aurasnk_info->stream_info = zalloc(sizeof(feature_bluetooth_aurasnk_stream_info_t));
    if (!aurasnk_info->stream_info) {
        work->status = BT_STATUS_NOMEM;
        goto error;
    }

    FEATURE_LOG_DEBUG("%s, sync created", __func__);
    work->status = BT_STATUS_SUCCESS;
    bt_list_remove(aurasnk_info->pending_work, work);
    memcpy(&aurasnk_info->stream_info->remote.addr, addr, sizeof(bt_le_address_t));
    aurasnk_info->stream_info->remote.sid = sid;
    if (aurasnk_info->scanner)
        bt_le_stop_scan_async(aurasnk_info->ins, aurasnk_info->scanner, NULL, NULL);

    return;

error:
    bt_pa_sync_terminate_async(ins, addr, sid, NULL, NULL);
    if (aurasnk_info)
        bt_list_remove(aurasnk_info->pending_work, work);
}

static feature_bluetooth_aurasnk_info_t* get_aurasnk_info(bt_instance_t* ins,
    const bt_le_address_t* addr, uint8_t sid)
{
    feature_bluetooth_features_info_t* features_info;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = NULL;

    features_info = ins->context;
    if (!features_info || !features_info->feature_ble_aurasnk)
        return NULL;

    aurasnk_info = bt_list_find(features_info->feature_ble_aurasnk, aurasnk_instance_cmp, ins);
    if (!aurasnk_info)
        return NULL;

    if (aurasnk_info->stream_info == NULL)
        return NULL; /**< Sync not established */

    if (memcmp(&aurasnk_info->stream_info->remote.addr, addr, sizeof(bt_le_address_t)) != 0)
        return NULL; /**< Address mismatch */

    if (aurasnk_info->stream_info->remote.sid != sid)
        return NULL; /**< Advertising Set ID mismatch */

    return aurasnk_info;
}

static void aurasnk_on_sync_terminated(const bt_le_address_t* addr, uint8_t sid, void* context)
{
    bt_instance_t* ins = context;
    feature_bluetooth_aurasnk_info_t* aurasnk_info;

    aurasnk_info = get_aurasnk_info(ins, addr, sid);
    if (!aurasnk_info)
        return;

    FEATURE_LOG_DEBUG("%s, sync terminated", __func__);
    if (aurasnk_info->stream_found_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID) {
        FeatureRemoveCallback(aurasnk_info->handle, aurasnk_info->stream_found_callback);
        aurasnk_info->stream_found_callback = FEATURE_BLE_FT_CALLBACK_ID_INVALID;
    }

    free(aurasnk_info->stream_info);
    aurasnk_info->stream_info = NULL;
}

static void aurasnk_on_sync_report(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_report_t* report, void* context)
{
    bt_status_t status;
    bt_instance_t* ins = context;
    bt_auracast_audio_info_t* info = NULL;
    feature_bluetooth_aurasnk_stream_info_t* stream_info = NULL;
    feature_bluetooth_aurasnk_info_t* aurasnk_info;

    aurasnk_info = get_aurasnk_info(ins, addr, sid);
    if (!aurasnk_info)
        return;

    info = malloc(sizeof(bt_auracast_audio_info_t));
    if (info == NULL)
        return;

    status = bt_auracast_sink_parse_adv_data(info, report);
    if (status != BT_STATUS_SUCCESS)
        goto exit;

    stream_info = aurasnk_info->stream_info;
    if (!stream_info)
        goto exit;

    memcpy(&stream_info->audio_info, info, sizeof(bt_auracast_audio_info_t));

exit:
    free(info);
}

static char* aurasnk_build_subid(const bt_le_address_t* addr, uint8_t sid, uint8_t subgroup)
{
    char* subid = zalloc(FEATURE_BLE_FT_STRING_MAX);
    if (!subid)
        return NULL;

    snprintf(subid, FEATURE_BLE_FT_STRING_MAX, "subgroup_%02x%02x%02x%02x%02x%02x(t%d)_sid%d_grp%d",
        addr->addr[5], addr->addr[4], addr->addr[3], addr->addr[2], addr->addr[1], addr->addr[0],
        addr->addr_type, sid, subgroup);

    return subid;
}

static bt_status_t aurasnk_parse_subid(bt_le_address_t* addr, uint8_t* sid, uint8_t* subgroup,
    const char* subid)
{
    int parsed;

    if (!subid)
        return BT_STATUS_PARM_INVALID;

    parsed = sscanf(subid, "subgroup_%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx(t%hhd)_sid%hhd_grp%hhd",
        &addr->addr[5], &addr->addr[4], &addr->addr[3], &addr->addr[2], &addr->addr[1],
        &addr->addr[0], &addr->addr_type, sid, subgroup);

    return (parsed == 9) ? BT_STATUS_SUCCESS : BT_STATUS_PARM_INVALID;
}

static char* get_context_str(uint16_t context)
{
    switch (context) {
    case BT_METADATA_AUDIO_CONTEXT_CONVERSATIONAL:
        return "Conv";
    case BT_METADATA_AUDIO_CONTEXT_MEDIA:
        return "Media";
    case BT_METADATA_AUDIO_CONTEXT_GAME:
        return "Game";
    case BT_METADATA_AUDIO_CONTEXT_INSTRUCTIONAL:
        return "Inst";
    case BT_METADATA_AUDIO_CONTEXT_VOICE_ASSISTANTS:
        return "Assist";
    case BT_METADATA_AUDIO_CONTEXT_LIVE:
        return "Live";
    case BT_METADATA_AUDIO_CONTEXT_SOUND_EFFECTS:
        return "Effect";
    case BT_METADATA_AUDIO_CONTEXT_NOTIFICATIONS:
        return "Notify";
    case BT_METADATA_AUDIO_CONTEXT_RINGTONE:
        return "Ring";
    case BT_METADATA_AUDIO_CONTEXT_ALERTS:
        return "Alert";
    case BT_METADATA_AUDIO_CONTEXT_EMERGENCY_ALARM:
        return "Emerg";
    case BT_METADATA_AUDIO_CONTEXT_UNSPECIFIED:
        return "Unspec";
    default:
        return "";
    }
}

static char* get_codec_str(uint8_t codec_id)
{
    /** Other codecs are not supported now */
    return (codec_id == BT_CODEC_ID_LC3) ? "LC3" : "Unsupported";
}

static char* get_sampling_frequency_str(uint8_t sampling_frequency)
{
    switch (sampling_frequency) {
    case BT_CODEC_CONFIG_FREQUENCY_48000:
        return "48k";
    case BT_CODEC_CONFIG_FREQUENCY_16000:
        return "16k";
    default:
        return "Unsupported";
    }
}

static char* aurasnk_sync_build_display_name(const bt_auracast_audio_subgroup_t* subgroup,
    uint8_t index)
{
    char* display_name = NULL;

    display_name = zalloc(FEATURE_BLE_FT_STRING_MAX);
    if (!display_name)
        return NULL;

    FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "[%d][%s]", index, 
        get_codec_str(subgroup->codec_id.coding_format));

    if (subgroup->codec_id.coding_format == BT_CODEC_ID_LC3) {
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "[ch:%d][%s]",
            bt_utils_count_ones(subgroup->config.lc3.location),
            get_sampling_frequency_str(subgroup->config.lc3.sampling_frequency));
    }

    FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "\n[");
    for (int i = 0; i < 16; i++) {
        if (subgroup->metadata.context & (1UL << i))
            FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "%s",
                get_context_str(1UL << i));
    }

    FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "]");
    if (subgroup->metadata.language[0] != '\0')
        FEATURE_BLE_STRCAT(display_name, FEATURE_BLE_FT_STRING_MAX, "[%s]",
            subgroup->metadata.language);

    return display_name;
}

static void aurasnk_on_auracast_ready(const bt_le_address_t* addr, uint8_t sid, bool encrypted,
    void* context)
{
    bt_instance_t* ins = context;
    feature_bluetooth_aurasnk_info_t* aurasnk_info;
    feature_bluetooth_aurasnk_stream_info_t* stream_info;
    FtArray* result_array = NULL;

    aurasnk_info = get_aurasnk_info(ins, addr, sid);
    if (!aurasnk_info)
        return;

    if (aurasnk_info->stream_found_callback == FEATURE_BLE_FT_CALLBACK_ID_INVALID)
        return;

    stream_info = aurasnk_info->stream_info;
    if (!stream_info)
        return;

    if (stream_info->reported)
        return;

    stream_info->encrypted = encrypted;
    result_array = system_bluetooth_ble_malloc_AuracastBaseResult_struct_type_array();
    result_array->_size = stream_info->audio_info.num_subgroups;
    result_array->_element = calloc(result_array->_size,
        sizeof(system_bluetooth_ble_AuracastScanResult*));

    for (uint8_t i = 0; i < stream_info->audio_info.num_subgroups; i++) {
        bt_auracast_audio_subgroup_t* subgroup = &stream_info->audio_info.subgroup[i];
        system_bluetooth_ble_AuracastBaseResult* result_data;
        char* display_name = NULL;
        char* subid = NULL;

        display_name = aurasnk_sync_build_display_name(subgroup, i);
        if (!display_name)
            continue;

        subid = aurasnk_build_subid(addr, sid, i);
        if (!subid) {
            free(display_name);
            continue;
        }

        result_data = system_bluetooth_bleMallocAuracastBaseResult();
        ((system_bluetooth_ble_AuracastBaseResult**)result_array->_element)[i] = result_data;

        result_data->subId = StringToFtString(subid);
        result_data->displayName = StringToFtString(display_name);
        result_data->encrypted = encrypted;

        free(display_name);
        free(subid);
    }

    FeatureInvokeCallback(aurasnk_info->handle, aurasnk_info->stream_found_callback, result_array);
    FeatureFreeValue(result_array);
    stream_info->reported = true;
}

static void aurasnk_create_sync_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    feature_bluetooth_aurasnk_info_t* aurasnk_info;
    feature_bluetooth_aurasnk_pending_work_t* work = NULL;
    aurasnk_work_type_t type = AURASNK_WORK_TYPE_CREATE_SYNC;

    FIND_INFO_BY_OBJECT(ins, userdata, aurasnk, aurasnk_info);
    if (!aurasnk_info) {
        FEATURE_LOG_ERROR("%s, aurasnk_info not found", __func__);
        goto error;
    }

    work = bt_list_find(aurasnk_info->pending_work, aurasnk_work_cmp, &type);
    if (!work)
        goto error;

    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, failed to create sync, status = %d", __func__, status);
        work->status = status;
        goto error;
    }

    FEATURE_LOG_DEBUG("%s, sync creating..", __func__);
    return;

error:
    if (aurasnk_info) {
        FeatureRemoveCallback(aurasnk_info->handle, aurasnk_info->stream_found_callback);
        aurasnk_info->stream_found_callback = FEATURE_BLE_FT_CALLBACK_ID_INVALID;
        bt_list_remove(aurasnk_info->pending_work, work);
    }
}

static const bt_pa_sync_create_param_t aurasnk_sync_params = {
    .skip = FEATURE_BLE_PA_SYNC_SKIP,
    .timeout = FEATURE_BLE_PA_SYNC_TIMEOUT_MS / 10,
    .filter = false,
    .no_report = false,
};

static const bt_pa_sync_callbacks_t aurasnk_sync_cbs = {
    .on_sync_established = aurasnk_on_sync_established,
    .on_sync_terminated = aurasnk_on_sync_terminated,
    .on_sync_report = aurasnk_on_sync_report,
    .on_auracast_ready = aurasnk_on_auracast_ready,
};
#endif /** CONFIG_BLUETOOTH_AURACAST_SINK */

void system_bluetooth_ble_AuracastSink_interface_aurasnk_createSync(FeatureInterfaceHandle handle,
    AppendData adata, FtPromiseId __pid__, system_bluetooth_ble_CreateAuracastSyncParams* params)
{
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    bt_status_t status;
    feature_bluetooth_aurasnk_pending_work_t* work = NULL;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = FeatureGetObjectData(handle);
    bt_le_address_t addr;
    uint8_t sid;

    if (params->callback == FEATURE_BLE_FT_CALLBACK_ID_INVALID) {
        status = BT_STATUS_PARM_INVALID;
        goto error;
    }

    if (!aurasnk_info) {
        FEATURE_LOG_ERROR("%s, not initialized", __func__);
        status = BT_STATUS_NOT_READY;
        goto error;
    }

    status = aurasnk_parse_id(&addr, &sid, params->id);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, invalid id %s", __func__, params->id);
        goto error;
    }

    if (aurasnk_info->stream_found_callback != FEATURE_BLE_FT_CALLBACK_ID_INVALID) {
        FEATURE_LOG_ERROR("%s, repeated attempt", __func__);
        status = BT_STATUS_DONE;
        goto error;
    }

    if (aurasnk_info->stream_info != NULL) {
        FEATURE_LOG_ERROR("%s, not terminated", __func__);
        status = BT_STATUS_BUSY;
        goto error;
    }

    work = zalloc(sizeof(feature_bluetooth_aurasnk_pending_work_t));
    if (!work) {
        status = BT_STATUS_NOMEM;
        goto error;
    }

    status = bt_pa_sync_create_async(aurasnk_info->ins, &addr, sid, &aurasnk_sync_params,
        &aurasnk_sync_cbs, aurasnk_info->ins, aurasnk_create_sync_cb, aurasnk_info);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, failed to create sync, status = %d", __func__, status);
        goto error;
    }

    aurasnk_info->stream_found_callback = params->callback;
    work->type = AURASNK_WORK_TYPE_CREATE_SYNC;
    work->handle = handle;
    work->pid = __pid__;
    work->status = BT_STATUS_FAIL; /**< Always marked as failed before done */
    bt_list_add_tail(aurasnk_info->pending_work, work);
    FEATURE_LOG_DEBUG("%s, sync creating.", __func__);

    return;

error:
    FeaturePromiseReject(handle, __pid__, bt_status_to_feature_error(status),
        "failed to create sync");

    free(work);
#else
    FeaturePromiseReject(handle, __pid__, bt_status_to_feature_error(BT_STATUS_NOT_SUPPORTED),
        "auracast sink is not supported");
#endif
}

void system_bluetooth_ble_AuracastSink_interface_aurasnk_terminateSync(
    FeatureInterfaceHandle handle, AppendData adata)
{
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    bt_status_t status;
    feature_bluetooth_aurasnk_info_t* aurasnk_info = FeatureGetObjectData(handle);

    if (!aurasnk_info) {
        FEATURE_LOG_ERROR("%s, not initialized", __func__);
        return;
    }

    if (aurasnk_info->stream_found_callback == FEATURE_BLE_FT_CALLBACK_ID_INVALID) {
        FEATURE_LOG_ERROR("%s, nothing to terminate", __func__);
        return;
    }

    if (aurasnk_info->stream_info == NULL) {
        FEATURE_LOG_ERROR("%s, sync establishing", __func__);
        return;
    }

    status = bt_pa_sync_terminate_async(aurasnk_info->ins, &aurasnk_info->stream_info->remote.addr,
        aurasnk_info->stream_info->remote.sid, NULL, NULL);
    if (status != BT_STATUS_SUCCESS) {
        FEATURE_LOG_ERROR("%s, failed to terminate sync, status = %d", __func__, status);
        return;
    }

    FEATURE_LOG_DEBUG("%s, sync terminating.", __func__);

    free(aurasnk_info->stream_info);
    aurasnk_info->stream_info = NULL;
#endif
}
}
