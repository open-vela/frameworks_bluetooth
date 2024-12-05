/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#ifndef __SAL_LE_ADVERTISE_INTERFACE_H__
#define __SAL_LE_ADVERTISE_INTERFACE_H__

#include "bt_le_advertiser.h"

#include <stdint.h>
#include <stdio.h>

bt_status_t bt_sal_le_start_adv(bt_controller_id_t id, uint8_t adv_id, ble_adv_params_t* params, uint8_t* adv_data, uint16_t adv_len, uint8_t* scan_rsp_data, uint16_t scan_rsp_len);
bt_status_t bt_sal_le_stop_adv(bt_controller_id_t id, uint8_t adv_id);

#endif