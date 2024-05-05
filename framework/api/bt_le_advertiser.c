/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "adv"

#include <stdlib.h>

#include "advertising.h"
#include "bluetooth.h"
#include "bt_internal.h"
#include "bt_le_advertiser.h"
#include "bt_list.h"
#include "utils/log.h"

bt_advertiser_t* BTSYMBOLS(bt_le_start_advertising)(bt_instance_t* ins,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    advertiser_callback_t* cbs)
{
    return start_advertising(NULL, params, adv_data, adv_len,
        scan_rsp_data, scan_rsp_len, cbs);
}

void BTSYMBOLS(bt_le_stop_advertising)(bt_instance_t* ins, bt_advertiser_t* adver)
{
    stop_advertising(adver);
}

void BTSYMBOLS(bt_le_stop_advertising_id)(bt_instance_t* ins, uint8_t adv_id)
{
    stop_advertising_id(adv_id);
}

bool BTSYMBOLS(bt_le_advertising_is_supported)(bt_instance_t* ins)
{
    return advertising_is_supported();
}
