/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
 ***************************************************************************/
#define LOG_TAG "adv_debug"

#include "advertising_debug.h"

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_time.h"
#include "bt_utils.h"
#include "utils/log.h"

static const char* adv_type_to_string(ble_adv_type_t adv_type)
{
    switch (adv_type) {
        CASE_RETURN_STR(BT_LE_ADV_IND)
        CASE_RETURN_STR(BT_LE_ADV_DIRECT_IND)
        CASE_RETURN_STR(BT_LE_ADV_SCAN_IND)
        CASE_RETURN_STR(BT_LE_ADV_NONCONN_IND)
        CASE_RETURN_STR(BT_LE_SCAN_RSP)
        CASE_RETURN_STR(BT_LE_LEGACY_ADV_IND)
        CASE_RETURN_STR(BT_LE_LEGACY_ADV_DIRECT_IND)
        CASE_RETURN_STR(BT_LE_LEGACY_ADV_SCAN_IND)
        CASE_RETURN_STR(BT_LE_LEGACY_ADV_NONCONN_IND)
        CASE_RETURN_STR(BT_LE_LEGACY_SCAN_RSP)
        CASE_RETURN_STR(BT_LE_EXT_ADV_IND)
        CASE_RETURN_STR(BT_LE_EXT_ADV_DIRECT_IND)
        CASE_RETURN_STR(BT_LE_EXT_ADV_SCAN_IND)
        CASE_RETURN_STR(BT_LE_EXT_ADV_NONCONN_IND)
        CASE_RETURN_STR(BT_LE_EXT_SCAN_RSP)
        DEFAULT_BREAK()
    }

    return "UNKNOWN";
}

static const char* adv_filter_policy_to_string(ble_adv_filter_policy_t filter_policy)
{
    switch (filter_policy) {
        CASE_RETURN_STR(BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE)
        CASE_RETURN_STR(BT_LE_ADV_FILTER_WHITE_LIST_FOR_SCAN)
        CASE_RETURN_STR(BT_LE_ADV_FILTER_WHITE_LIST_FOR_CONNECTION)
        CASE_RETURN_STR(BT_LE_ADV_FILTER_WHITE_LIST_FOR_ALL)
        DEFAULT_BREAK()
    }

    return "UNKNOWN";
}

static const char* adv_channel_map_to_string(ble_adv_channel_t channel_map)
{
    switch (channel_map) {
        CASE_RETURN_STR(BT_LE_ADV_CHANNEL_DEFAULT)
        CASE_RETURN_STR(BT_LE_ADV_CHANNEL_37_ONLY)
        CASE_RETURN_STR(BT_LE_ADV_CHANNEL_38_ONLY)
        CASE_RETURN_STR(BT_LE_ADV_CHANNEL_39_ONLY)
        DEFAULT_BREAK()
    }

    return "UNKNOWN";
}

void adv_dump_info(uint8_t adv_id, const advertising_info_t* adv_info)
{
    const ble_adv_params_t* params;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    char time_str[BT_SLOTS_TO_TIME_STR_LENGTH] = { 0 };

    if (!adv_info)
        return;

    BT_LOGD("Advertising Info[%d]:", adv_id);
    params = &adv_info->params;

    BT_LOGD("\tadv type: %s(%d), tx power: %ddBm, interval: %" PRIu32 " slots (%s), "
            "channel map: %s(%d)",
        adv_type_to_string(params->adv_type), params->adv_type, params->tx_power, params->interval,
        bt_slots_to_time_str(time_str, BT_SLOTS_TO_TIME_STR_LENGTH, (uint16_t)params->interval),
        adv_channel_map_to_string(params->channel_map), params->channel_map);

    /** AdvA */
    if (params->own_addr_type == BT_LE_ADDR_TYPE_RANDOM) {
        /** Own address is valid */
        bt_addr_ba2str(&params->own_addr, addr_str);
        BT_LOGD("\town addr: %s, addr_type: %d", addr_str, params->own_addr_type);
    } else {
        BT_LOGD("\town addr_type: %d", params->own_addr_type);
    }

    /** TargetA */
    if (params->adv_type == BT_LE_ADV_DIRECT_IND
        || params->adv_type == BT_LE_LEGACY_ADV_DIRECT_IND
        || params->adv_type == BT_LE_EXT_ADV_IND
        || params->adv_type == BT_LE_EXT_ADV_DIRECT_IND
        || params->adv_type == BT_LE_EXT_ADV_SCAN_IND
        || params->adv_type == BT_LE_EXT_ADV_NONCONN_IND) {
        /** All extended advertising might be directed and therefore has a TargetA field
         *  TODO: check `Advertising_Event_Properties` to confirm if peer address is valid */
        bt_addr_ba2str(&params->peer_addr, addr_str);
        BT_LOGD("\tpeer addr: %s, addr_type: %d", addr_str, params->peer_addr_type);
    }

    /** Advertising Filter Policy */
    if (params->adv_type != BT_LE_ADV_DIRECT_IND
        && params->adv_type != BT_LE_LEGACY_ADV_DIRECT_IND
        && params->adv_type != BT_LE_EXT_ADV_DIRECT_IND) {
        /** Advertising Filter Policy is valid */
        BT_LOGD("\tfilter_policy: %s(%d)", adv_filter_policy_to_string(params->filter_policy),
            params->filter_policy);
        if (params->filter_policy != BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE) {
            /** Filter Accept List is used */
            adapter_dump_whitelist();
        }
    }

    if (params->duration)
        BT_LOGD("\tduration: %" PRIu32 " ms", params->duration * 10);

    if (adv_info->adv_data && adv_info->adv_len > 0)
        BT_DUMPBUFFER("\tadv data:", adv_info->adv_data, adv_info->adv_len);

    if (adv_info->scan_rsp_data && adv_info->scan_rsp_len > 0)
        BT_DUMPBUFFER("\tscan rsp data:", adv_info->scan_rsp_data, adv_info->scan_rsp_len);
}

void adv_dump_advertiser(void)
{
    int cnt = 0;
    struct list_node* node;
    adv_manager_t* manager = adv_manager_get_interface();

    if (!manager || !manager->started)
        return;

    BT_LOGD("%s", __func__);

    list_for_every(&manager->advertiser_list, node)
    {
        advertiser_t* adver = (advertiser_t*)node;
        BT_LOGD("advertiser[%d]: adv_id:%d, adver:%p", cnt++, adver->adv_id, adver);
    }

    BT_LOGD("%s ends, cnt = %d", __func__, cnt);
}
