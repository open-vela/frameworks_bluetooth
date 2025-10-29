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
#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "pbap_pce_event.h"
#include "pbap_pce_service.h"
#include "pbap_pce_state_machine.h"
#include "pce_parser.h"
#include "sal_pbap_pce_interface.h"
#include "service_loop.h"
#include "state_machine.h"
#include "utils/log.h"

#define PBAP_TELECOM_PHONE_BOOK_PATH "telecom/pb"

typedef bt_status_t (*vcard_praser_t)(uint16_t data_len, char* data, bt_pce_contact_t* contact);

typedef struct __pbap_pce_state_machine {
    state_machine_t sm;
    void* service;
    bt_address_t addr;
    uint16_t acl_handle;
    bt_list_t* pending_contact_query;
    char* vc_data;
    vcard_praser_t vcard_praser;
    char* vcl_data;
} pce_state_machine_t;

static const char* stack_event_to_string(pbap_pce_event_t event);

#define PBAP_PCE_TRANS_DBG(_sm, _addr, _action)                                                 \
    do {                                                                                        \
        char __addr_str[BT_ADDR_STR_LENGTH] = { 0 };                                            \
        bt_addr_ba2str(_addr, __addr_str);                                                      \
        BT_LOGD("%s State=%s, Peer=[%s]", _action, hsm_get_current_state_name(sm), __addr_str); \
    } while (0);

#define PBAP_PCE_DBG_ENTER(__sm, __addr) PBAP_PCE_TRANS_DBG(__sm, __addr, "Enter")
#define PBAP_PCE_DBG_EXIT(__sm, __addr) PBAP_PCE_TRANS_DBG(__sm, __addr, "Exit ")
#define PBAP_PCE_DBG_EVENT(__sm, __addr, __event)                                              \
    do {                                                                                       \
        char __addr_str[BT_ADDR_STR_LENGTH] = { 0 };                                           \
        bt_addr_ba2str(__addr, __addr_str);                                                    \
        BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm), \
            __addr_str, stack_event_to_string(event));                                         \
    } while (0);

static const char* stack_event_to_string(pbap_pce_event_t event)
{
    switch (event) {
        CASE_RETURN_STR(PCE_CONNECT_REQ)
        CASE_RETURN_STR(PCE_CONNECTED_EVT)
        CASE_RETURN_STR(PCE_DISCONNECT_REQ)
        CASE_RETURN_STR(PCE_DISCONNECTED_EVT)
        CASE_RETURN_STR(PCE_DISCONNECTING_EVT)
        CASE_RETURN_STR(PCE_CHANGE_DIR_REQ)
        CASE_RETURN_STR(PCE_CHANGE_DIR_EVT)
        CASE_RETURN_STR(PCE_PULL_VCARD_LIST_REQ)
        CASE_RETURN_STR(PCE_PULL_VCARD_LIST_DATA_EVT)
        CASE_RETURN_STR(PCE_PULL_VCARD_LIST_END_EVT)
        CASE_RETURN_STR(PCE_PULL_VCARD_REQ)
        CASE_RETURN_STR(PCE_PULL_VCARD_DATA_EVT)
        CASE_RETURN_STR(PCE_PULL_VCARD_END_EVT)
        CASE_RETURN_STR(PCE_GET_CONNTACT_REQ)
        CASE_RETURN_STR(PCE_GET_CONTACT_END_EVT)
    default:
        return "UNKNOWN_PCE_EVENT";
    }
}

static void disconnected_enter(state_machine_t* sm);
static void disconnected_exit(state_machine_t* sm);
static void connecting_enter(state_machine_t* sm);
static void connecting_exit(state_machine_t* sm);
static void connected_enter(state_machine_t* sm);
static void connected_exit(state_machine_t* sm);
static void disconnecting_enter(state_machine_t* sm);
static void disconnecting_exit(state_machine_t* sm);

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data);

static const state_t disconnected_state = {
    .state_name = "Disconnected",
    .state_value = PBAP_PCE_STATE_DISCONNECTED,
    .enter = disconnected_enter,
    .exit = disconnected_exit,
    .process_event = disconnected_process_event,
};

static const state_t connecting_state = {
    .state_name = "Connecting",
    .state_value = PBAP_PCE_STATE_CONNECTING,
    .enter = connecting_enter,
    .exit = connecting_exit,
    .process_event = connecting_process_event,
};

static const state_t connected_state = {
    .state_name = "Connected",
    .state_value = PBAP_PCE_STATE_CONNECTED,
    .enter = connected_enter,
    .exit = connected_exit,
    .process_event = connected_process_event,
};

static const state_t disconnecting_state = {
    .state_name = "Disconnecting",
    .state_value = PBAP_PCE_STATE_DISCONNECTING,
    .enter = disconnecting_enter,
    .exit = disconnecting_exit,
    .process_event = disconnecting_process_event,
};

static void disconnected_enter(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;
    const state_t* prev_state = hsm_get_previous_state(sm);

    PBAP_PCE_DBG_ENTER(sm, &pce_sm->addr);

    if (prev_state != NULL) {
        notify_pce_connection_state_changed(&pce_sm->addr, PROFILE_STATE_DISCONNECTED);
    }
}

static void disconnected_exit(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_ENTER(sm, &pce_sm->addr);
}

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    bt_status_t status;
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;
    pbap_pce_data_t* data = (pbap_pce_data_t*)p_data;

    PBAP_PCE_DBG_EVENT(sm, &pce_sm->addr, event);
    switch (event) {
    case PCE_CONNECT_REQ: {
        status = bt_sal_pbap_pce_connect(&data->addr);
        if (status != BT_STATUS_SUCCESS) {
            notify_pce_connection_state_changed(&pce_sm->addr, PROFILE_STATE_DISCONNECTED);
            break;
        }
        hsm_transition_to(sm, &connecting_state);
        break;
    }

    case PCE_CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;

    default:
        break;
    }

    return true;
}

static void connecting_enter(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_ENTER(sm, &pce_sm->addr);

    notify_pce_connection_state_changed(&pce_sm->addr, PROFILE_STATE_CONNECTING);
}

static void connecting_exit(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_EXIT(sm, &pce_sm->addr);
}

static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_EVENT(sm, &pce_sm->addr, event);
    switch (event) {
    case PCE_CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;

    case PCE_DISCONNECT_REQ:
        bt_sal_pbap_pce_disconnect(&pce_sm->addr);
        hsm_transition_to(sm, &disconnecting_state);
        break;
    case PCE_DISCONNECTED_EVT:
        hsm_transition_to(sm, &disconnected_state);
        break;
    default:
        break;
    }

    return true;
}

static void connected_enter(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_ENTER(sm, &pce_sm->addr);

    bt_sal_pbap_pce_change_directory(&pce_sm->addr, PBAP_TELECOM_PHONE_BOOK_PATH);
    notify_pce_connection_state_changed(&pce_sm->addr, PROFILE_STATE_CONNECTED);
}

static void connected_exit(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_EXIT(sm, &pce_sm->addr);
}

static bt_status_t start_get_contact(pce_state_machine_t* pce_sm)
{
    pce_get_contact_req_t* req;
    pce_pull_card_list_req_t* pull_cl_req;

    req = (pce_get_contact_req_t*)bt_list_node(bt_list_head(pce_sm->pending_contact_query));

    pull_cl_req = create_pull_cl_req(req->req_type, req->req_data);

    if (pull_cl_req == NULL) {
        BT_LOGE("%s: create_pull_cl_req failed", __func__);
        return BT_STATUS_NOMEM;
    }

    return do_in_pbap_pce_service(&pce_sm->addr, PCE_PULL_VCARD_LIST_REQ, pull_cl_req);
}

static bt_status_t handle_get_conntact_msg(pce_state_machine_t* pce_sm, pbap_pce_data_t* p_data)
{
    /* Move the get contact request to the pending list */
    bt_list_add_tail(pce_sm->pending_contact_query, p_data->ext_data);
    p_data->ext_data = NULL;

    if (bt_list_length(pce_sm->pending_contact_query) > 1)
        return BT_STATUS_SUCCESS;

    return start_get_contact(pce_sm);
}

static bt_status_t pce_pull_vcard_listing(bt_address_t* addr, pce_pull_card_list_req_t* req)
{
    return bt_sal_pbap_pce_pull_vcard_listing(addr, PBAP_SEARCH_PROPERTY_NAME, req->req_data);
}

static bt_status_t handle_pull_vcard_list_data_evt(pce_state_machine_t* pce_sm, char* data)
{
    uint32_t original_len = 0;
    uint32_t new_len;
    char* new_data;

    if (pce_sm->vcl_data != NULL) {
        original_len = strlen(pce_sm->vcl_data);
    }

    new_len = strlen(data) + original_len + 1;
    new_data = (char*)realloc(pce_sm->vcl_data, new_len);

    if (new_data == NULL) {
        BT_LOGE("%s: realloc failed", __func__);
        free(pce_sm->vcl_data);
        return BT_STATUS_NOMEM;
    }

    pce_sm->vcl_data = new_data;
    strlcat(pce_sm->vcl_data, data, new_len);

    return BT_STATUS_SUCCESS;
}

static bt_status_t handle_pull_vcard_list_end_evt(pce_state_machine_t* pce_sm, uint16_t status)
{
    bt_list_t* card_list;
    pce_vcard_entry_t* vcard_entry;
    bt_list_node_t* node;
    pce_pull_card_req_t* pull_card_req;

    if (status != BT_STATUS_SUCCESS) {
        goto exit;
    }

    pce_parse_card_list(strlen(pce_sm->vcl_data), pce_sm->vcl_data, &card_list);

    node = bt_list_head(card_list);
    vcard_entry = (pce_vcard_entry_t*)bt_list_node(node);

    pull_card_req = create_pull_card_req(vcard_entry->card_handle, PCE_PROPERTY_MASK_TEL | PCE_PROPERTY_MASK_FN | PCE_PROPERTY_MASK_N);

    if (pull_card_req == NULL) {
        BT_LOGE("%s: create_pull_card_req failed", __func__);
        status = BT_STATUS_FAIL;
        goto exit;
    }

    do_in_pbap_pce_service(&pce_sm->addr, PCE_PULL_VCARD_REQ, pull_card_req);

exit:
    free(pce_sm->vcl_data);
    pce_sm->vcl_data = NULL;
    if (card_list != NULL)
        bt_list_free(card_list);

    if (status != BT_STATUS_SUCCESS) {
        bt_list_remove_node(pce_sm->pending_contact_query, bt_list_head(pce_sm->pending_contact_query));
        if (bt_list_is_empty(pce_sm->pending_contact_query) == false)
            start_get_contact(pce_sm);
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t pce_pull_vcard(bt_address_t* addr, pce_pull_card_req_t* req)
{
    bt_status_t status;

    status = bt_sal_pbap_pce_pull_vcard(addr, req->name, req->filter);
    if (status != BT_STATUS_SUCCESS)
        BT_LOGE("%s: pull vcard failed %d", __func__, status);

    return status;
}

static bt_status_t handle_pce_pull_vc_data(pce_state_machine_t* pce_sm, char* data)
{
    uint32_t original_len;
    uint32_t new_len;
    char* new_data;

    if (pce_sm->vc_data != NULL) {
        original_len = strlen(pce_sm->vc_data);
    }

    new_len = strlen(data) + original_len + 1;
    new_data = (char*)realloc(pce_sm->vc_data, new_len);

    if (new_data == NULL) {
        BT_LOGE("%s: realloc failed", __func__);
        free(pce_sm->vc_data);
        return BT_STATUS_NOMEM;
    }

    pce_sm->vc_data = new_data;

    return BT_STATUS_SUCCESS;
}

static bt_status_t handle_pce_pull_vcard_end(pce_state_machine_t* pce_sm, uint16_t* pull_status)
{
    bt_status_t status;
    pce_get_contact_end_evt_t end_evt = {};
    pce_get_contact_req_t* req;

    if (*pull_status != BT_STATUS_SUCCESS) {
        BT_LOGE("pce pull vcard failed.");
        return BT_STATUS_FAIL;
    }

    req = bt_list_node(bt_list_head(pce_sm->pending_contact_query));

    assert(req != NULL);

    end_evt.contact = (bt_pce_contact_t*)zalloc(sizeof(bt_pce_contact_t));
    if (end_evt.contact == NULL) {
        BT_LOGE("%s: zalloc failed", __func__);
        return BT_STATUS_NOMEM;
    }

    status = pce_sm->vcard_praser(strlen(pce_sm->vc_data), pce_sm->vc_data, end_evt.contact);

    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: pce_parse_card_v2_1 failed", __func__);
        return status;
    }

    end_evt.status = BT_STATUS_SUCCESS;
    end_evt.type = req->req_type;
    end_evt.req_data = req->req_data;
    notify_get_contact_end(&pce_sm->addr, &end_evt);

    bt_list_remove_node(pce_sm->pending_contact_query, bt_list_head(pce_sm->pending_contact_query));
    free(end_evt.contact);
    free(pce_sm->vc_data);
    pce_sm->vc_data = NULL;

    if (bt_list_is_empty(pce_sm->pending_contact_query) == false)
       return start_get_contact(pce_sm);

    return BT_STATUS_SUCCESS;
}

static void cleanup_current_query(pce_state_machine_t* pce_sm)
{
    bt_list_remove_node(pce_sm->pending_contact_query, bt_list_head(pce_sm->pending_contact_query));

    free(pce_sm->vc_data);
    pce_sm->vc_data = NULL;
    free(pce_sm->vcl_data);
    pce_sm->vcl_data = NULL;
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    bt_status_t status = BT_STATUS_SUCCESS;
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;
    pbap_pce_data_t* data = (pbap_pce_data_t*)p_data;
    pce_get_contact_req_t* req;
    pce_get_contact_end_evt_t end_evt = {};

    PBAP_PCE_DBG_EVENT(sm, &pce_sm->addr, event);
    switch (event) {
    case PCE_DISCONNECT_REQ:
        bt_sal_pbap_pce_disconnect(&pce_sm->addr);
        hsm_transition_to(sm, &disconnecting_state);
        break;
    case PCE_GET_CONNTACT_REQ:
        status = handle_get_conntact_msg(pce_sm, data);
        break;
    case PCE_PULL_VCARD_LIST_REQ:
        status = pce_pull_vcard_listing(&pce_sm->addr, data->ext_data);
        break;
    case PCE_PULL_VCARD_LIST_DATA_EVT:
        status = handle_pull_vcard_list_data_evt(pce_sm, data->ext_data);
        break;
    case PCE_PULL_VCARD_LIST_END_EVT:
        status = handle_pull_vcard_list_end_evt(pce_sm, *(uint16_t*)(data->ext_data));
        break;
    case PCE_PULL_VCARD_REQ:
        status = pce_pull_vcard(&pce_sm->addr, data->ext_data);
        break;
    case PCE_PULL_VCARD_DATA_EVT:
        status = handle_pce_pull_vc_data(pce_sm, data->ext_data);
        break;
    case PCE_PULL_VCARD_END_EVT:
        status = handle_pce_pull_vcard_end(pce_sm, data->ext_data);
        break;
    default:
        break;
    }

    if (status != BT_STATUS_SUCCESS) {
        req = bt_list_node(bt_list_head(pce_sm->pending_contact_query));
        end_evt.status = status;
        end_evt.type = req->req_type;
        end_evt.req_data = req->req_data;
        notify_get_contact_end(&pce_sm->addr, &end_evt);
        cleanup_current_query(pce_sm);

        if (!bt_list_is_empty(pce_sm->pending_contact_query))
            return start_get_contact(pce_sm);
    }

    return true;
}

static void disconnecting_enter(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_ENTER(sm, &pce_sm->addr);

    notify_pce_connection_state_changed(&pce_sm->addr, PROFILE_STATE_DISCONNECTING);
}

static void disconnecting_exit(state_machine_t* sm)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_EXIT(sm, &pce_sm->addr);
}

static bool disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    pce_state_machine_t* pce_sm = (pce_state_machine_t*)sm;

    PBAP_PCE_DBG_EVENT(sm, &pce_sm->addr, event);
    switch (event) {
    case PCE_DISCONNECTED_EVT:
        hsm_transition_to(sm, &disconnected_state);
    default:
        break;
    }

    return true;
}

static void after_query_contact_req(void* data)
{

    free(data);
}

pce_state_machine_t* pce_state_machine_new(void* context, bt_address_t* bd_addr)
{
    pce_state_machine_t* pce_sm;

    pce_sm = (pce_state_machine_t*)zalloc(sizeof(pce_state_machine_t));
    if (!pce_sm)
        return NULL;

    pce_sm->service = context;
    hsm_ctor(&pce_sm->sm, (state_t*)&disconnected_state);
    memcpy(&pce_sm->addr, bd_addr, sizeof(bt_address_t));
    pce_sm->pending_contact_query = bt_list_new(after_query_contact_req);
    pce_sm->vcard_praser = pce_parse_card_v2_1;

    return pce_sm;
}

void pce_state_machine_destory(pce_state_machine_t* pce_sm)
{
    if (!pce_sm)
        return;

    if (pce_state_machine_get_state(pce_sm) != PBAP_PCE_STATE_DISCONNECTED) {
        bt_sal_pbap_pce_disconnect(&pce_sm->addr);
        notify_pce_connection_state_changed(&pce_sm->addr, PROFILE_STATE_DISCONNECTED);
    }

    free(pce_sm->vc_data);
    free(pce_sm->vcl_data);
    bt_list_free(pce_sm->pending_contact_query);
    hsm_dtor(&pce_sm->sm);
    free((void*)pce_sm);
}

static void pce_state_machine_event_dispatch(pce_state_machine_t* pce_sm, pbap_pce_msg_t* pce_event)
{
    if (!pce_event || !pce_sm)
        return;

    hsm_dispatch_event(&pce_sm->sm, pce_event->event, &pce_event->data);
}

void pce_state_machine_handle_event(pce_state_machine_t* sm, pbap_pce_msg_t* pce_event)
{
    pce_state_machine_event_dispatch(sm, pce_event);
}

pbap_pce_state_t pce_state_machine_get_state(pce_state_machine_t* sm)
{
    const state_t* cur_state = hsm_get_current_state(&sm->sm);

    if (!cur_state)
        return PBAP_PCE_STATE_DISCONNECTED;

    return cur_state->state_value;
}

const char* pce_state_machine_current_state(pce_state_machine_t* sm)
{
    return hsm_get_current_state_name(&sm->sm);
}

profile_connection_state_t pce_state_machine_get_connection_state(pce_state_machine_t* sm)
{
    pbap_pce_state_t state = pce_state_machine_get_state(sm);

    if (state == PBAP_PCE_STATE_DISCONNECTED) {
        return PROFILE_STATE_DISCONNECTED;
    } else if (state == PBAP_PCE_STATE_CONNECTING) {
        return PROFILE_STATE_CONNECTING;
    } else if (state == PBAP_PCE_STATE_CONNECTED) {
        return PROFILE_STATE_CONNECTED;
    } else if (state == PBAP_PCE_STATE_DISCONNECTING) {
        return PROFILE_STATE_DISCONNECTING;
    }

    return PROFILE_STATE_DISCONNECTED;
}