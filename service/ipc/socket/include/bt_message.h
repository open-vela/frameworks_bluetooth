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
#ifndef _BT_MESSAGE_H__
#define _BT_MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_message_a2dp_sink.h"
#include "bt_message_a2dp_source.h"
#include "bt_message_adapter.h"
#include "bt_message_advertiser.h"
#include "bt_message_avrcp_control.h"
#include "bt_message_avrcp_target.h"
#include "bt_message_device.h"
#include "bt_message_gattc.h"
#include "bt_message_gatts.h"
#include "bt_message_hfp_ag.h"
#include "bt_message_hfp_hf.h"
#include "bt_message_hid_device.h"
#include "bt_message_l2cap.h"
#include "bt_message_log.h"
#include "bt_message_manager.h"
#include "bt_message_pan.h"
#include "bt_message_scan.h"
#include "bt_message_spp.h"

#include "service_loop.h"

typedef enum {
#define __BT_MESSAGE_CODE__
    BT_MESSAGE_START,
#include "bt_message_a2dp_sink.h"
#include "bt_message_a2dp_source.h"
#include "bt_message_adapter.h"
#include "bt_message_advertiser.h"
#include "bt_message_avrcp_control.h"
#include "bt_message_avrcp_target.h"
#include "bt_message_device.h"
#include "bt_message_gattc.h"
#include "bt_message_gatts.h"
#include "bt_message_hfp_ag.h"
#include "bt_message_hfp_hf.h"
#include "bt_message_hid_device.h"
#include "bt_message_l2cap.h"
#include "bt_message_log.h"
#include "bt_message_manager.h"
#include "bt_message_pan.h"
#include "bt_message_scan.h"
#include "bt_message_spp.h"
    BT_MESSAGE_END,
#undef __BT_MESSAGE_CODE__
#define __BT_CALLBACK_CODE__
    BT_CALLBACK_START,
#include "bt_message_a2dp_sink.h"
#include "bt_message_a2dp_source.h"
#include "bt_message_adapter.h"
#include "bt_message_advertiser.h"
#include "bt_message_avrcp_control.h"
#include "bt_message_avrcp_target.h"
#include "bt_message_device.h"
#include "bt_message_gattc.h"
#include "bt_message_gatts.h"
#include "bt_message_hfp_ag.h"
#include "bt_message_hfp_hf.h"
#include "bt_message_hid_device.h"
#include "bt_message_l2cap.h"
#include "bt_message_manager.h"
#include "bt_message_pan.h"
#include "bt_message_scan.h"
#include "bt_message_spp.h"
    BT_CALLBACK_END,
#undef __BT_MESSAGE_CODE__
} bt_message_type_t;

#pragma pack(4)
typedef struct
{
    uint32_t code; /* bt_message_type_t */
    uint64_t context;
    union {
        bt_manager_result_t manager_r;
        bt_adapter_result_t adpt_r;
        bt_device_result_t devs_r;
        bt_a2dp_sink_result_t a2dp_sink_r;
        bt_a2dp_source_result_t a2dp_source_r;
        bt_avrcp_target_result_t avrcp_target_r;
        bt_avrcp_control_result_t avrcp_control_r;
        bt_hfp_ag_result_t hfp_ag_r;
        bt_hfp_hf_result_t hfp_hf_r;
        bt_advertiser_result_t adv_r;
        bt_scan_result_t scan_r;
        bt_gattc_result_t gattc_r;
        bt_gatts_result_t gatts_r;
        bt_spp_result_t spp_r;
        bt_pan_result_t pan_r;
        bt_hid_device_result_t hidd_r;
        bt_l2cap_result_t l2cap_r;
    };
    union {
        bt_message_manager_t manager_pl;

        bt_message_adapter_t adpt_pl;
        bt_message_adapter_callbacks_t adpt_cb;

        bt_message_device_t devs_pl;

        bt_message_a2dp_sink_t a2dp_sink_pl;
        bt_message_a2dp_sink_callbacks_t a2dp_sink_cb;

        bt_message_a2dp_source_t a2dp_source_pl;
        bt_message_a2dp_source_callbacks_t a2dp_source_cb;

        bt_message_avrcp_target_t avrcp_target_pl;
        bt_message_avrcp_target_callbacks_t avrcp_target_cb;
        bt_message_avrcp_control_t avrcp_control_pl;
        bt_message_avrcp_control_callbacks_t avrcp_control_cb;

        bt_message_hfp_ag_t hfp_ag_pl;
        bt_message_hfp_ag_callbacks_t hfp_ag_cb;

        bt_message_hfp_hf_t hfp_hf_pl;
        bt_message_hfp_hf_callbacks_t hfp_hf_cb;

        bt_message_advertiser_t adv_pl;
        bt_message_advertiser_callbacks_t adv_cb;

        bt_message_scan_t scan_pl;
        bt_message_scan_callbacks_t scan_cb;

        bt_message_gattc_t gattc_pl;
        bt_message_gattc_callbacks_t gattc_cb;

        bt_message_gatts_t gatts_pl;
        bt_message_gatts_callbacks_t gatts_cb;

        bt_message_spp_t spp_pl;
        bt_message_spp_callbacks_t spp_cb;

        bt_message_pan_t pan_pl;
        bt_message_pan_callbacks_t pan_cb;

        bt_message_hid_device_t hidd_pl;
        bt_message_hid_device_callbacks_t hidd_cb;

        bt_message_l2cap_t l2cap_pl;
        bt_message_l2cap_callbacks_t l2cap_cb;

        bt_message_log_t log_pl;
    };
} bt_message_packet_t;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_H__ */
