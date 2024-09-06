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
#include <stdlib.h>

#include "utils/btsnoop_filter.h"
#include "utils/btsnoop_log.h"
#include "utils/log.h"

typedef struct {
    uint64_t filter_items;
    bt_list_t* acl_connection_list;
} g_snoop_filter_global_t;

static g_snoop_filter_global_t g_snoop_filter = { 0 };

#define L2CAP_NULL_IDENTIFIER_CID 0x0000
#define L2CAP_SIGNALING_CHANNEL_CID 0x0001
#define L2CAP_LE_SIGNALING_CHANNEL_CID 0x0005

#define GET_HCI_H4_PLAYLOAD(pkt) ((pkt) + 1)
#define GET_HCI_H4_PLAYLOAD_SIZE(pkt_size) ((pkt_size)-1)
#define GET_L2CAP_PACKET_DATA(pkt) ((pkt) + 6)
#define GET_L2CAP_PACKET_PLATLOAD_SIZE(pkt_size) ((pkt_size)-6)
#define GET_HCI_EVENT_PLAYLOAD(pkt) ((pkt) + 2)
#define GET_HCI_EVENT_PLAYLOAD_SIZE(pkt_size) ((pkt_size)-2)
#define GET_L2CAP_COMMAND_DATA(pkt) ((pkt) + 4)
#define GET_L2CAP_COMMAND_DATA_SIZE(pkt_size) ((pkt_size)-4)

#define GET_HCI_TYPE(hci_pkt) ((hci_pkt)[0])
#define GET_HCI_EVENT_CODE(evt) ((evt)[0])
#define GET_ACL_CONNECTION_HANDLE_FROM_ACL_DATA(pkt) (((uint16_t*)(pkt))[0] & 0x0FFF)
#define GET_PB_FLAG_FROM_ACL_DATA(pkt) (((pkt)[1] >> 4) & 0x3)
#define GET_ACL_CONNECTION_HANDLE_FROM_CONNECT_COMPELTE_EVENT(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 3))
#define GET_STATUS_FROM_CONNECT_COMPELTE_EVENT(pkt) (((uint8_t*)(pkt))[2])
#define GET_ACL_CONNECTION_HANDLE_FROM_DISCONNECT_COMPELTE_EVENT(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 3))
#define GET_STATUS_FROM_DISCONNECT_COMPELTE_EVENT(pkt) (((uint8_t*)(pkt))[2])
#define GET_L2CAP_CID(pkt) (*(uint16_t*)(pkt))
#define GET_L2CAP_COMMAND_CODE(pkt) ((pkt)[2])
#define GET_L2CAP_CONNECTION_REQ_COMMAND_PSM(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 6))
#define GET_L2CAP_CONNECTION_REQ_COMMAND_SCID(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 8))
#define GET_L2CAP_CONNECTION_RSP_COMMAND_SCID(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 8))
#define GET_L2CAP_CONNECTION_RSP_COMMAND_DCID(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 6))
#define GET_L2CAP_CONNECTION_RSP_COMMAND_STATUS(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 10))
#define GET_L2CAP_DISCONNECTION_RSP_COMMAND_SCID(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 6))
#define GET_L2CAP_DISCONNECTION_RSP_COMMAND_DCID(pkt) (*(uint16_t*)(((uint8_t*)(pkt)) + 8))

#define GET_L2CAP_RSP_DERIVE_CIDS(pkt, is_receive, local_cid, peer_cid, get_scid, get_dcid) \
    do {                                                                                    \
        if (is_receive) {                                                                   \
            local_cid = get_scid(pkt);                                                      \
            peer_cid = get_dcid(pkt);                                                       \
        } else {                                                                            \
            local_cid = get_dcid(pkt);                                                      \
            peer_cid = get_scid(pkt);                                                       \
            response_cids_info.peer_cid = peer_cid;                                         \
        }                                                                                   \
    } while (0)

static void free_l2cap_cid_item(void* data)
{
    free(data);
}

static btsnoop_filter_acl_info_t* malloc_acl_connection_item(uint16_t acl_connection_handle)
{
    btsnoop_filter_acl_info_t* item;

    item = zalloc(sizeof(btsnoop_filter_acl_info_t));
    if (NULL == item) {
        return NULL;
    }

    item->connection_handle = acl_connection_handle;
    item->filter_cids = bt_list_new(free_l2cap_cid_item);

    if (NULL == item->filter_cids) {
        free(item);
        return NULL;
    }

    return item;
}

static void free_acl_connection_item(void* data)
{
    btsnoop_filter_acl_info_t* info = (btsnoop_filter_acl_info_t*)data;

    bt_list_free(info->filter_cids);
    free(data);
}

static bool compare_acl_connection_item(void* data, void* context)
{
    return ((btsnoop_filter_acl_info_t*)data)->connection_handle == *(uint16_t*)context;
}

static btsnoop_filter_l2cap_channel_info_t* malloc_filter_cid_item(uint16_t local_cid, uint16_t peer_cid, uint16_t psm, btsnoop_l2cap_state_t state)
{
    btsnoop_filter_l2cap_channel_info_t* new_item;

    new_item = (btsnoop_filter_l2cap_channel_info_t*)zalloc(sizeof(btsnoop_filter_l2cap_channel_info_t));

    if (NULL == new_item)
        return NULL;

    new_item->cids.local_cid = local_cid;
    new_item->cids.peer_cid = peer_cid;
    new_item->psm = psm;
    new_item->state = state;
    return new_item;
}

static bool compare_l2cap_local_and_remote_cid_item(void* data, void* context)
{
    btsnoop_filter_l2cap_channel_info_t* tmp_data = (btsnoop_filter_l2cap_channel_info_t*)data;
    btsnoop_l2cap_channel_cids_t* tmp_context = (btsnoop_l2cap_channel_cids_t*)context;

    return (tmp_data->cids.local_cid == tmp_context->local_cid) && (tmp_data->cids.peer_cid == tmp_context->peer_cid);
}

static bool compare_l2cap_local_or_remote_cid_item(void* data, void* context)
{
    btsnoop_filter_l2cap_channel_info_t* tmp_data = (btsnoop_filter_l2cap_channel_info_t*)data;
    btsnoop_l2cap_channel_cids_t* tmp_context = (btsnoop_l2cap_channel_cids_t*)context;

    return (tmp_data->cids.local_cid == tmp_context->local_cid) || (tmp_data->cids.peer_cid == tmp_context->peer_cid);
}

static bool compare_l2cap_local_cid_item(void* data, void* context)
{
    btsnoop_filter_l2cap_channel_info_t* tmp_data = (btsnoop_filter_l2cap_channel_info_t*)data;
    btsnoop_l2cap_channel_cids_t* tmp_context = (btsnoop_l2cap_channel_cids_t*)context;

    return (tmp_data->cids.local_cid == tmp_context->local_cid);
}

static int handle_hci_command(uint8_t* pkt, uint32_t pkt_size)
{
    // TODO: handle_hci_command
    return 0;
}

static btsnoop_filter_flag_t handle_rfcomm_data(uint8_t* pkt, uint32_t pkt_size)
{
    // TODO: handle_rfcomm_data
    return BTSNOOP_FILTER_UNFILTER;
}

static bool check_channel_need_filtered(btsnoop_filter_acl_info_t* acl_info, uint16_t psm, uint16_t scid, uint8_t is_receive)
{
    assert(acl_info);
    switch (psm) {
    case BTSNOOP_PSM_AVDTP:
        if ((acl_info->avdtp_signal_ch.peer_cid != L2CAP_NULL_IDENTIFIER_CID) || (acl_info->avdtp_signal_ch.local_cid != L2CAP_NULL_IDENTIFIER_CID))
            return true;

        if (is_receive)
            acl_info->avdtp_signal_ch.peer_cid = scid;
        else
            acl_info->avdtp_signal_ch.local_cid = scid;

        break;
    default:
        break;
    }
    return false;
}

static void handle_l2cap_connection_request(btsnoop_filter_acl_info_t* acl_info, uint8_t is_receive, uint8_t* pkt, uint32_t pkt_size)
{
    assert(acl_info);
    uint16_t psm, scid;
    btsnoop_filter_flag_t filter_flag = BTSNOOP_FILTER_MAX;
    btsnoop_filter_l2cap_channel_info_t* data = NULL;

    psm = GET_L2CAP_CONNECTION_REQ_COMMAND_PSM(pkt);

    switch (psm) {
    case BTSNOOP_PSM_RFCOMM:
        filter_flag = handle_rfcomm_data(pkt, pkt_size);
        break;
    case BTSNOOP_PSM_AVDTP:
        filter_flag = BTSNOOP_FILTER_A2DP_AUDIO;
        break;
    case BTSNOOP_PSM_AVCTP_BROWSING:
        filter_flag = BTSNOOP_FILTER_AVCTP_BROWSING;
        break;
    case BTSNOOP_PSM_ATT:
        filter_flag = BTSNOOP_FILTER_ATT;
        break;
    default:
        break;
    }

    if (!(g_snoop_filter.filter_items & (1ULL << filter_flag))) {
        return;
    }

    scid = GET_L2CAP_CONNECTION_REQ_COMMAND_SCID(pkt);

    if (!check_channel_need_filtered(acl_info, psm, scid, is_receive)) {
        return;
    }

    if (is_receive) {
        data = malloc_filter_cid_item(L2CAP_NULL_IDENTIFIER_CID, scid, psm, BTSNOOP_L2CAP_STATE_CONNECTING);
    } else {
        data = malloc_filter_cid_item(scid, L2CAP_NULL_IDENTIFIER_CID, psm, BTSNOOP_L2CAP_STATE_CONNECTING);
    }

    if (NULL == data) {
        BT_LOGE("malloc filter cid item failed!");
        return;
    }

    bt_list_add_tail(acl_info->filter_cids, data);
}

static void handle_acl_info_connection_response(btsnoop_filter_acl_info_t* acl_info, uint16_t local_cid, uint16_t peer_cid)
{
    assert(acl_info);
    if (acl_info->avdtp_signal_ch.local_cid == local_cid) {
        acl_info->avdtp_signal_ch.peer_cid = peer_cid;
    } else if (acl_info->avdtp_signal_ch.peer_cid == peer_cid) {
        acl_info->avdtp_signal_ch.local_cid = local_cid;
    }
}

static void handle_l2cap_connection_response(btsnoop_filter_acl_info_t* acl_info, uint8_t is_receive, uint8_t* pkt, uint32_t pkt_size)
{
    assert(acl_info);
    uint16_t status, local_cid, peer_cid;
    btsnoop_filter_l2cap_channel_info_t* channel_info = NULL;
    btsnoop_l2cap_channel_cids_t response_cids_info = { 0 };

    GET_L2CAP_RSP_DERIVE_CIDS(pkt, is_receive, local_cid, peer_cid, GET_L2CAP_CONNECTION_RSP_COMMAND_SCID, GET_L2CAP_CONNECTION_RSP_COMMAND_DCID);

    if (is_receive) {
        response_cids_info.local_cid = local_cid;
    } else {
        response_cids_info.peer_cid = peer_cid;
    }

    status = GET_L2CAP_CONNECTION_RSP_COMMAND_STATUS(pkt);

    switch (status) {
    case BTSNOOP_L2CAP_RSP_STATUS_SUCCESSFUL:
        handle_acl_info_connection_response(acl_info, local_cid, peer_cid);
        channel_info = bt_list_find(acl_info->filter_cids, compare_l2cap_local_and_remote_cid_item, &response_cids_info);
        if (NULL == channel_info)
            return;

        if (is_receive) {
            channel_info->cids.peer_cid = peer_cid;
        } else {
            channel_info->cids.local_cid = local_cid;
        }

        channel_info->state = BTSNOOP_L2CAP_STATE_CONNECTED;
        break;
    case BTSNOOP_L2CAP_RSP_STATUS_PENDING:
        break;
    default:
        channel_info = bt_list_find(acl_info->filter_cids, compare_l2cap_local_and_remote_cid_item, &response_cids_info);
        if (NULL == channel_info)
            return;

        bt_list_remove(acl_info->filter_cids, channel_info);
        break;
    }
}

static void handle_acl_info_disconnection_response(btsnoop_filter_acl_info_t* acl_info, uint16_t local_cid, uint16_t peer_cid)
{
    assert(acl_info);
    if ((acl_info->avdtp_signal_ch.local_cid == local_cid) && (acl_info->avdtp_signal_ch.peer_cid == peer_cid)) {
        acl_info->avdtp_signal_ch.peer_cid = L2CAP_NULL_IDENTIFIER_CID;
        acl_info->avdtp_signal_ch.local_cid = L2CAP_NULL_IDENTIFIER_CID;
    }
}

static void handle_l2cap_disconnection_response(btsnoop_filter_acl_info_t* acl_info, uint8_t is_receive, uint8_t* pkt, uint32_t pkt_size)
{
    assert(acl_info);
    uint16_t local_cid, peer_cid;
    btsnoop_filter_l2cap_channel_info_t* channel_info = NULL;
    btsnoop_l2cap_channel_cids_t response_cids_info = { 0 };

    GET_L2CAP_RSP_DERIVE_CIDS(pkt, is_receive, local_cid, peer_cid, GET_L2CAP_DISCONNECTION_RSP_COMMAND_SCID, GET_L2CAP_DISCONNECTION_RSP_COMMAND_DCID);
    response_cids_info.local_cid = local_cid;
    response_cids_info.peer_cid = peer_cid;
    channel_info = bt_list_find(acl_info->filter_cids, compare_l2cap_local_or_remote_cid_item, &response_cids_info);

    handle_acl_info_disconnection_response(acl_info, local_cid, peer_cid);
    bt_list_remove(acl_info->filter_cids, channel_info);
}

static void handle_l2cap_signaling_channel_data(btsnoop_filter_acl_info_t* acl_info, uint8_t is_receive, uint8_t* pkt, uint32_t pkt_size)
{
    assert(acl_info);
    uint8_t command_code = GET_L2CAP_COMMAND_CODE(pkt);

    switch (command_code) {
    case BTSNOOP_L2CAP_CODE_CONNECTION_REQUEST:
        handle_l2cap_connection_request(acl_info, is_receive, pkt, pkt_size);
        break;
    case BTSNOOP_L2CAP_CODE_CONNECTION_RESPONSE:
        handle_l2cap_connection_response(acl_info, is_receive, pkt, pkt_size);
        break;
    case BTSNOOP_L2CAP_CODE_DISCONNECTION_RESPONSE:
        handle_l2cap_disconnection_response(acl_info, is_receive, pkt, pkt_size);
        break;
    default:
        break;
    }
}

static bool handle_acl_data(uint8_t is_receive, uint8_t* pkt, uint32_t pkt_size)
{
    btsnoop_l2cap_channel_cids_t acl_cids;
    uint16_t connection_handle;
    btsnoop_filter_acl_info_t* acl_info;
    uint8_t* l2cap_packet;
    uint32_t l2cap_pkt_size;
    uint16_t acl_cid;
    uint8_t pb_flag;

    l2cap_packet = GET_L2CAP_PACKET_DATA(pkt);
    l2cap_pkt_size = GET_L2CAP_PACKET_PLATLOAD_SIZE(pkt_size);
    connection_handle = GET_ACL_CONNECTION_HANDLE_FROM_ACL_DATA(pkt);
    acl_info = bt_list_find(g_snoop_filter.acl_connection_list, compare_acl_connection_item, &connection_handle);

    if (NULL == acl_info) {
        BT_LOGE("The acl connection information does not exist.");
        return false;
    }

    pb_flag = GET_PB_FLAG_FROM_ACL_DATA(pkt);
    if (pb_flag == BTSNOOP_ACL_PB_CONTINUING) {
        acl_cid = acl_info->prev_acl_cid;
    } else {
        acl_cid = GET_L2CAP_CID(l2cap_packet);
        acl_info->prev_acl_cid = acl_cid;
    }

    if (acl_cid == L2CAP_SIGNALING_CHANNEL_CID || acl_cid == L2CAP_LE_SIGNALING_CHANNEL_CID) {
        handle_l2cap_signaling_channel_data(acl_info, is_receive, l2cap_packet, l2cap_pkt_size);
    } else {
        acl_cids.local_cid = acl_cid;
        if (NULL != bt_list_find(acl_info->filter_cids, compare_l2cap_local_cid_item, &acl_cids)) {
            return true;
        }
    }

    return false;
}

static int handle_sco_data(uint8_t* pkt, uint32_t pkt_size)
{
    return 1;
}

static void handle_hci_event_connect_complete(uint8_t* pkt, uint32_t pkt_size)
{
    uint16_t connection_handle;
    btsnoop_filter_acl_info_t* acl_info;

    if (GET_STATUS_FROM_CONNECT_COMPELTE_EVENT(pkt) != BTSNOOP_HCI_EVENT_STATUS_SUCCESS) {
        return;
    }

    connection_handle = GET_ACL_CONNECTION_HANDLE_FROM_CONNECT_COMPELTE_EVENT(pkt);
    if (NULL == g_snoop_filter.acl_connection_list) {
        BT_LOGE("The BTsnoop filter is not initialized.");
        return;
    }

    if (NULL != bt_list_find(g_snoop_filter.acl_connection_list, compare_acl_connection_item, &connection_handle)) {
        BT_LOGE("The acl connection information already exists.");
        return;
    }

    if (NULL == (acl_info = malloc_acl_connection_item(connection_handle))) {
        BT_LOGE("malloc acl connection item failed.");
        return;
    }

    bt_list_add_tail(g_snoop_filter.acl_connection_list, acl_info);

    return;
}

static void handle_hci_event_disconnect_complete(uint8_t* pkt, uint32_t pkt_size)
{
    btsnoop_filter_acl_info_t* acl_info;
    uint16_t connection_handle;

    if (GET_STATUS_FROM_DISCONNECT_COMPELTE_EVENT(pkt) != BTSNOOP_HCI_EVENT_STATUS_SUCCESS) {
        return;
    }

    connection_handle = GET_ACL_CONNECTION_HANDLE_FROM_DISCONNECT_COMPELTE_EVENT(pkt);
    acl_info = (btsnoop_filter_acl_info_t*)bt_list_find(g_snoop_filter.acl_connection_list, compare_acl_connection_item, &connection_handle);

    if (NULL == acl_info) {
        BT_LOGE("The acl connection information does not exist!");
        return;
    }

    bt_list_remove(g_snoop_filter.acl_connection_list, acl_info);

    return;
}

static bool handle_hci_event(uint8_t* pkt, uint32_t pkt_size)
{
    uint16_t event_code = GET_HCI_EVENT_CODE(pkt);

    switch (event_code) {
    case BTSNOOP_CONNECT_COMPLETE:
        handle_hci_event_connect_complete(pkt, pkt_size);
        break;
    case BTSNOOP_DISCONNECT_COMPLETE:
        handle_hci_event_disconnect_complete(pkt, pkt_size);
        break;
    default:
        break;
    }
    return 0;
}

static int handle_iso_data(uint8_t* hci_pkt, uint32_t hci_pkt_size)
{
    return 1;
}

bool filter_can_filter(uint8_t is_receive, uint8_t* hci_pkt, uint32_t hci_pkt_size)
{
    uint8_t* pkt_data;
    uint32_t pkt_size;
    uint8_t hci_type;

    hci_type = GET_HCI_TYPE(hci_pkt);
    pkt_data = GET_HCI_H4_PLAYLOAD(hci_pkt);
    pkt_size = GET_HCI_H4_PLAYLOAD_SIZE(hci_pkt_size);

    switch (hci_type) {
    case BTSNOOP_HCI_TYPE_HCI_COMMAND:
        return handle_hci_command(pkt_data, pkt_size);
    case BTSNOOP_HCI_TYPE_ACL_DATA:
        return handle_acl_data(is_receive, pkt_data, pkt_size);
    case BTSNOOP_HCI_TYPE_SCO_DATA:
        return handle_sco_data(pkt_data, pkt_size);
    case BTSNOOP_HCI_TYPE_HCI_EVENT:
        return handle_hci_event(pkt_data, pkt_size);
    case BTSNOOP_HCI_TYPE_ISO_DATA:
        return handle_iso_data(pkt_data, pkt_size);
    default:
        return 0;
    }

    return 0;
}

int filter_init()
{
    g_snoop_filter.acl_connection_list = bt_list_new(free_acl_connection_item);

    if (NULL == g_snoop_filter.acl_connection_list)
        return BT_STATUS_NOMEM;

    return BT_STATUS_SUCCESS;
}

void filter_uninit()
{
    bt_list_free(g_snoop_filter.acl_connection_list);
    g_snoop_filter.acl_connection_list = NULL;
}

int filter_set_filter_flag(btsnoop_filter_flag_t filter_flag)
{
    if (filter_flag < 0 || filter_flag >= BTSNOOP_FILTER_MAX) {
        return BT_STATUS_PARM_INVALID;
    }

    g_snoop_filter.filter_items |= 1ULL << filter_flag;

    return BT_STATUS_SUCCESS;
}

int filter_remove_filter_flag(btsnoop_filter_flag_t filter_flag)
{
    if (filter_flag < 0 || filter_flag >= BTSNOOP_FILTER_MAX) {
        return BT_STATUS_PARM_INVALID;
    }

    g_snoop_filter.filter_items &= ~(1ULL << filter_flag);

    return BT_STATUS_SUCCESS;
}