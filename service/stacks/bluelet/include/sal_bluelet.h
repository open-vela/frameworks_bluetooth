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
#ifndef __BT_SAL_BLUELET_H__
#define __BT_SAL_BLUELET_H__

#include "stack_adapter_common.h"

#include "bt_device.h"
#include "bt_hfp.h"
#include "bt_hid_device.h"

profile_connection_state_t bluelet_profile_connection_state(SERVICE_PROFILE_CONNECTION_STATE state);
profile_connection_reason_t bluelet_profile_connection_reason(SERVICE_PROFILE_CONNECTION_REASON reason);
#if defined(CONFIG_BLUETOOTH_HFP_AG) || defined(CONFIG_BLUETOOTH_HFP_HF)
hfp_audio_state_t bluelet_hf_audio_state(SERVICE_HFP_SCO_STATE state);
#endif

#if defined(CONFIG_BLUETOOTH_HID_DEVICE)
hid_app_state_t bluelet_hid_app_state(SERVICE_BTHD_APP_STATE state);
#endif

#if defined(CONFIG_BLUETOOTH_GATT)
#include "bt_gatt_defs.h"
gatt_status_t bluelet_gatt_status(SERVICE_GATT_STATUS status);
#endif

#endif /* __BT_SAL_BLUELET_H__ */
