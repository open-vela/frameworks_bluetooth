/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef __PBAP_PCE_EVENT_H__
#define __PBAP_PCE_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"
#include "bt_pbap_pce.h"
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef enum {
    PCE_CONNECT_REQ = 1,
    PCE_CONNECTED_EVT,
    PCE_DISCONNECT_REQ,
    PCE_DISCONNECTED_EVT,
    PCE_DISCONNECTING_EVT,
    PCE_CHANGE_DIR_REQ,
    PCE_CHANGE_DIR_EVT,
    PCE_PULL_VCARD_LIST_REQ,
    PCE_PULL_VCARD_LIST_DATA_EVT,
    PCE_PULL_VCARD_LIST_END_EVT,
    PCE_PULL_VCARD_REQ,
    PCE_PULL_VCARD_DATA_EVT,
    PCE_PULL_VCARD_END_EVT,
    PCE_GET_CONNTACT_REQ,
    PCE_GET_CONTACT_END_EVT,
} pbap_pce_event_t;

typedef void (*pce_msg_callback_t)(void* data);

typedef struct {
    bt_pce_get_contact_req_type_t req_type;
    char req_data[0];
} pce_get_contact_req_t;

typedef struct {
    bt_pce_get_contact_req_type_t req_type;
    char req_data[0];
} pce_pull_card_list_req_t;

typedef struct {
    uint64_t filter;
    char name[BT_PBAP_PCE_VCARD_HANDLE_MAX_LEN];
} pce_pull_card_req_t;

typedef struct pbap_pce_msg_data {
    void* ext_data;
    bt_address_t addr;
} pbap_pce_data_t;

typedef struct {
    pbap_pce_event_t event;
    pbap_pce_data_t data;
} pbap_pce_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
pbap_pce_msg_t* pbap_pce_msg_new(pbap_pce_event_t event, bt_address_t* addr,
    void* data);
void pbap_pce_msg_destroy(pbap_pce_msg_t* msg);
pce_pull_card_list_req_t* create_pull_cl_req(bt_pce_get_contact_req_type_t req_type, void* pull_data);
pce_get_contact_req_t* create_query_contact_req(bt_pce_get_contact_req_type_t type, void* req_data);
pce_pull_card_req_t* create_pull_card_req(void* pull_data, uint64_t filter);

#endif /* __PBAP_PCE_EVENT_H__ */
