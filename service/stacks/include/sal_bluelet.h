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
<<<<<<< HEAD
#include "bt_hfp.h"
#include "bt_hid_device.h"
=======
#include "bt_hfp_ag.h"
>>>>>>> bluetooth framework re-implement base
#include "utils/log.h"

static inline profile_connection_state_t bluelet_profile_connection_state(SERVICE_PROFILE_CONNECTION_STATE state)
{
    switch (state) {
    case SERVICE_PROFILE_DISCONNECTED:
        return PROFILE_STATE_DISCONNECTED;
    case SERVICE_PROFILE_CONNECTING:
        return PROFILE_STATE_CONNECTING;
    case SERVICE_PROFILE_CONNECTED:
        return PROFILE_STATE_CONNECTED;
    case SERVICE_PROFILE_DISCONNECTING:
        return PROFILE_STATE_DISCONNECTING;
    default:
        BT_LOGE("Unknow connection state: %d", state);
        return PROFILE_STATE_DISCONNECTED;
    }
}

#if defined(CONFIG_BLUETOOTH_HFP_AG) || defined(CONFIG_BLUETOOTH_HFP_HF)
static inline hfp_audio_state_t bluelet_hf_audio_state(SERVICE_HFP_SCO_STATE state)
{
    switch (state) {
    case SERVICE_HFP_SCO_CONNECTED:
        return HFP_AUDIO_STATE_CONNECTED;
    case SERVICE_HFP_SCO_DISCONNECTED:
        return HFP_AUDIO_STATE_DISCONNECTED;
    case SERVICE_HFP_SCO_UNKNOWN:
<<<<<<< HEAD
=======
        return HFP_AUDIO_STATE_DISCONNECTED;
>>>>>>> bluetooth framework re-implement base
    default:
        BT_LOGE("Unknow audio state: %d", state);
        return HFP_AUDIO_STATE_DISCONNECTED;
    }
}
#endif

<<<<<<< HEAD
#if defined(CONFIG_BLUETOOTH_HID_DEVICE)
static inline hid_app_state_t bluelet_hid_app_state(SERVICE_BTHD_APP_STATE state)
{
    switch (state) {
    case BTHD_APP_STATE_NOT_REGISTERED:
        return HID_APP_STATE_NOT_REGISTERED;
    case BTHD_APP_STATE_REGISTERED:
        return HID_APP_STATE_REGISTERED;
    default:
        BT_LOGE("Unknow hidd app state: %d", state);
        return HID_APP_STATE_NOT_REGISTERED;
    }
}
#endif

#if defined(CONFIG_BLUETOOTH_GATT)
#include "bt_gatt_defs.h"
static inline gatt_status_t bluelet_gatt_status(SERVICE_GATT_STATUS status)
{
    switch (status) {
    case GATT_SUCCESS:
        return GATT_STATUS_SUCCESS;
    case GATT_REQUEST_NOT_SUPPORTED:
        return GATT_STATUS_REQUEST_NOT_SUPPORTED;
    case GATT_INSUFFICIENT_AUTHENTICATION:
        return GATT_STATUS_INSUFFICIENT_AUTHENTICATION;
    case GATT_INSUFFICIENT_ENCRYPTION:
        return GATT_STATUS_INSUFFICIENT_ENCRYPTION;
    case GATT_READ_NOT_PERMITTED:
        return GATT_STATUS_READ_NOT_PERMITTED;
    case GATT_WRITE_NOT_PERMITTED:
        return GATT_STATUS_WRITE_NOT_PERMITTED;
    case GATT_INVALID_ATTRIBUTE_LENGTH:
        return GATT_STATUS_INVALID_ATTRIBUTE_LENGTH;
    case GATT_FAILURE:
    default:
        BT_LOGE("Unknow gatt state: %d", status);
        return GATT_STATUS_FAILURE;
    }
}
#endif

=======
>>>>>>> bluetooth framework re-implement base
#endif /* __BT_SAL_BLUELET_H__ */
