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

#ifndef __BT_SNOOP_FILTER_H__
#define __BT_SNOOP_FILTER_H__

#include "bt_list.h"
#include "bt_status.h"
#include "btsnoop_log.h"

#define BTSNOOP_HCI_EVENT_STATUS_SUCCESS 0x00
typedef enum {
    BTSNOOP_HCI_TYPE_HCI_COMMAND = 0x01,
    BTSNOOP_HCI_TYPE_ACL_DATA,
    BTSNOOP_HCI_TYPE_SCO_DATA,
    BTSNOOP_HCI_TYPE_HCI_EVENT,
    BTSNOOP_HCI_TYPE_ISO_DATA
} btsnoop_hci_t;

typedef enum {
    BTSNOOP_PSM_RFCOMM = 0x0003,
    BTSNOOP_PSM_AVDTP = 0x0019,
    BTSNOOP_PSM_AVCTP_BROWSING = 0x001B,
    BTSNOOP_PSM_ATT = 0x001F,
} btsnoop_psms_t;

typedef enum {
    BTSNOOP_CONNECT_COMPLETE = 0x03,
    BTSNOOP_DISCONNECT_COMPLETE = 0x05,
} btsnoop_hci_event_t;

typedef enum {
    BTSNOOP_ACL_PB_NON_FLUSHABLE = 0x00,
    BTSNOOP_ACL_PB_CONTINUING = 0x01,
    BTSNOOP_ACL_PB_FLUSHABLE = 0x02,
} btsnoop_acl_pb_flag_t;

typedef enum {
    BTSNOOP_L2CAP_CODE_CONNECTION_REQUEST = 0x02,
    BTSNOOP_L2CAP_CODE_CONNECTION_RESPONSE = 0x03,
    BTSNOOP_L2CAP_CODE_DISCONNECTION_RESPONSE = 0x07,
} btsnoop_l2cap_code_t;

typedef enum {
    BTSNOOP_L2CAP_STATE_DISCONECTED = 0x00,
    BTSNOOP_L2CAP_STATE_CONNECTING = 0x01,
    BTSNOOP_L2CAP_STATE_CONNECTED = 0x02
} btsnoop_l2cap_state_t;

typedef enum {
    BTSNOOP_L2CAP_RSP_STATUS_SUCCESSFUL = 0x00,
    BTSNOOP_L2CAP_RSP_STATUS_PENDING = 0x01,
} btsnoop_l2cap_rsp_status_t;

typedef struct {
    uint16_t local_cid;
    uint16_t peer_cid;
} btsnoop_l2cap_channel_cids_t;

typedef struct {
    uint16_t connection_handle;
    btsnoop_l2cap_channel_cids_t avdtp_signal_ch;
    uint16_t prev_acl_cid;
    bt_list_t* filter_cids;
} btsnoop_filter_acl_info_t;

typedef struct {
    btsnoop_l2cap_channel_cids_t cids;
    uint16_t psm;
    btsnoop_l2cap_state_t state;
} btsnoop_filter_l2cap_channel_info_t;

int filter_init();
void filter_uninit();
bool filter_can_filter(uint8_t is_recieve, uint8_t* hci_pkt, uint32_t hci_pkt_size);
int filter_set_filter_flag(btsnoop_filter_flag_t filter_flag);
int filter_remove_filter_flag(btsnoop_filter_flag_t filter_flag);
#endif //__SNOOP_FILTER_H__