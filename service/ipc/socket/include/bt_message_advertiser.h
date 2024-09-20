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

#ifdef __BT_MESSAGE_CODE__
BT_ADVERTISER_MESSAGE_START,
    BT_LE_START_ADVERTISING,
    BT_LE_STOP_ADVERTISING,
    BT_LE_STOP_ADVERTISING_ID,
    BT_LE_ADVERTISING_IS_SUPPORT,
    BT_ADVERTISER_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_ADVERTISER_CALLBACK_START,
    BT_LE_ON_ADVERTISER_START,
    BT_LE_ON_ADVERTISER_STOPPED,
    BT_ADVERTISER_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_ADVERTISER_H__
#define _BT_MESSAGE_ADVERTISER_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bluetooth.h"
#include "bt_le_advertiser.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t vbool; /* boolean */
        uint8_t pad[2];
        uint64_t remote;
    } bt_advertiser_result_t;

    typedef struct
    {
        bt_instance_t* ins;
        advertiser_callback_t* callback;
        uint64_t remote;
    } bt_advertiser_remote_t;

    typedef union {
        struct {
            uint64_t adver;
            ble_adv_params_t params;
            uint16_t adv_len;
            uint16_t scan_rsp_len;
            uint8_t adv_data[256];
            uint8_t scan_rsp_data[256];
        } _bt_le_start_advertising;

        struct {
            uint64_t adver;
        } _bt_le_stop_advertising;

        struct {
            uint8_t id;
        } _bt_le_stop_advertising_id;

    } bt_message_advertiser_t;

    typedef struct
    {
        struct {
            uint64_t adver;
            uint8_t adv_id;
            uint8_t status;
        } _on_advertising_start;

        struct {
            uint64_t adver;
            uint8_t adv_id;
        } _on_advertising_stopped;
    } bt_message_advertiser_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_ADVERTISER_H__ */
