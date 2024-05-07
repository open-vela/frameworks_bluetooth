/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef __BT_ADVERTISING_H_
#define __BT_ADVERTISING_H_

#include <stdint.h>

#include "bt_le_advertiser.h"

enum advertising_state {
    LE_ADVERTISING_STARTED = 0,
    LE_ADVERTISING_STOPPED
};

void advertising_on_state_changed(uint8_t adv_id, uint8_t state);
bt_advertiser_t* start_advertising(void* remote,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    const advertiser_callback_t* cbs);
void stop_advertising(bt_advertiser_t* adver);
void stop_advertising_id(uint8_t adv_id);
bool advertising_is_supported(void);
void adv_manager_init(void);
void adv_manager_cleanup(void);

#endif /* __BT_ADVERTISING_H_ */