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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bluetooth.h"
#include "lea_mcpc_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_mcpc_interface.h"
#include "stack_adapter_common.h"
#include "stack_adapter_lea_mcp.h"

#define UNKNOWN_INFO "unknown"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPC

static void adpt_lea_mcc_media_player_name_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t *name);
static void adpt_lea_mcc_media_player_icon_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id);
static void adpt_lea_mcc_media_player_icon_url_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t *url);
static void adpt_lea_mcc_playback_speed_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int8_t speed);
static void adpt_lea_mcc_seeking_speed_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int8_t speed);
static void adpt_lea_mcc_playing_order_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t order);
static void adpt_lea_mcc_playing_orders_supported_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint16_t orders);
static void adpt_lea_mcc_media_control_opcodes_supported_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint32_t opcodes);
static void adpt_lea_mcc_track_changed_callback(BD_ADDR mcs_addr, uint32_t mcs_id);
static void adpt_lea_mcc_track_title_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t *title);
static void adpt_lea_mcc_track_duration_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int32_t duration);
static void adpt_lea_mcc_track_position_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int32_t position);
static void adpt_lea_mcc_media_state_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t state);
static void adpt_lea_mcc_media_control_result_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t opcode, uint8_t result);
static void adpt_lea_mcc_search_control_result_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t result);
static void adpt_lea_mcc_current_track_segments_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id);
static void adpt_lea_mcc_current_track_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id);
static void adpt_lea_mcc_next_track_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id);
static void adpt_lea_mcc_parent_group_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id);
static void adpt_lea_mcc_current_group_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id);
static void adpt_lea_mcc_search_results_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id);
static void adpt_lea_mcc_content_control_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t ccid);

const LEA_MCC_CALLBACK_S adpt_lea_mcp_client_callbacks = {
    .lea_mcc_media_player_name_cb = adpt_lea_mcc_media_player_name_callback,
    .lea_mcc_media_player_icon_object_id_cb = adpt_lea_mcc_media_player_icon_object_id_callback,
    .lea_mcc_media_player_icon_url_cb = adpt_lea_mcc_media_player_icon_url_callback,

    .lea_mcc_playback_speed_cb = adpt_lea_mcc_playback_speed_callback,
    .lea_mcc_seeking_speed_cb = adpt_lea_mcc_seeking_speed_callback,
    .lea_mcc_playing_order_cb = adpt_lea_mcc_playing_order_callback,
    .lea_mcc_playing_orders_supported_cb = adpt_lea_mcc_playing_orders_supported_callback,
    .lea_mcc_media_control_opcodes_supported_cb = adpt_lea_mcc_media_control_opcodes_supported_callback,
    .lea_mcc_content_control_id_cb = adpt_lea_mcc_content_control_id_callback,

    .lea_mcc_track_changed_cb = adpt_lea_mcc_track_changed_callback,
    .lea_mcc_track_title_cb = adpt_lea_mcc_track_title_callback,
    .lea_mcc_track_duration_cb = adpt_lea_mcc_track_duration_callback,
    .lea_mcc_track_position_cb = adpt_lea_mcc_track_position_callback,

    .lea_mcc_media_state_cb = adpt_lea_mcc_media_state_callback,
    .lea_mcc_media_control_result_cb = adpt_lea_mcc_media_control_result_callback,
    .lea_mcc_search_control_result_cb = adpt_lea_mcc_search_control_result_callback,

    .lea_mcc_current_track_segments_object_id_cb = adpt_lea_mcc_current_track_segments_object_id_callback,
    .lea_mcc_current_track_object_id_cb = adpt_lea_mcc_current_track_object_id_callback,
    .lea_mcc_next_track_object_id_cb = adpt_lea_mcc_next_track_object_id_callback,
    .lea_mcc_parent_group_object_id_cb = adpt_lea_mcc_parent_group_object_id_callback,
    .lea_mcc_current_group_object_id_cb = adpt_lea_mcc_current_group_object_id_callback,
    .lea_mcc_search_results_object_id_cb = adpt_lea_mcc_search_results_object_id_callback,
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_mcc_media_player_name_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t *name)
{
    bt_address_t addr = { 0 };
    char *nullname = UNKNOWN_INFO;

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    if (name == NULL) {
        lea_mcpc_on_media_player_name(&addr, mcs_id, strlen(nullname) + 1, nullname);
    } else {
        lea_mcpc_on_media_player_name(&addr, mcs_id, strlen((char *)name) + 1, (char *)name);
    }
}

static void adpt_lea_mcc_media_player_icon_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id,
                                                              LEA_OBJ_ID obj_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_media_player_icon_object_id(&addr, mcs_id, obj_id);
}

static void adpt_lea_mcc_media_player_icon_url_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t *url)
{
    bt_address_t addr = { 0 };
    char *nullurl = UNKNOWN_INFO;

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    if (url == NULL) {
        lea_mcpc_on_media_player_icon_url(&addr, mcs_id, strlen(nullurl) + 1, nullurl);
    } else {
        lea_mcpc_on_media_player_icon_url(&addr, mcs_id, strlen((char *)url) + 1, (char *)url);
    }
}

static void adpt_lea_mcc_playback_speed_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int8_t speed)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_playback_speed(&addr, mcs_id, speed);
}

static void adpt_lea_mcc_seeking_speed_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int8_t speed)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_seeking_speed(&addr, mcs_id, speed);
}

static void adpt_lea_mcc_playing_order_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t order)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_playing_order(&addr, mcs_id, order);
}

static void adpt_lea_mcc_playing_orders_supported_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint16_t orders)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_playing_orders_supported(&addr, mcs_id, orders);
}

static void adpt_lea_mcc_media_control_opcodes_supported_callback(BD_ADDR mcs_addr, uint32_t mcs_id,
                                                                  uint32_t opcodes)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_media_control_opcodes_supported(&addr, mcs_id, opcodes);
}

static void adpt_lea_mcc_track_changed_callback(BD_ADDR mcs_addr, uint32_t mcs_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_track_changed(&addr, mcs_id);
}

static void adpt_lea_mcc_track_title_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t *title)
{
    bt_address_t addr = { 0 };
    char *nulltitle = UNKNOWN_INFO;

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    if (title == NULL) {
        lea_mcpc_on_track_title(&addr, mcs_id, strlen(nulltitle) + 1, nulltitle);
    } else {
        lea_mcpc_on_track_title(&addr, mcs_id, strlen((char *)title) + 1, (char *)title);
    }
}

static void adpt_lea_mcc_track_duration_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int32_t duration)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_track_duration(&addr, mcs_id, duration);
}

static void adpt_lea_mcc_track_position_callback(BD_ADDR mcs_addr, uint32_t mcs_id, int32_t position)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_track_position(&addr, mcs_id, position);
}

static void adpt_lea_mcc_media_state_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t state)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_media_state(&addr, mcs_id, state);
}

static void adpt_lea_mcc_media_control_result_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t opcode, uint8_t result)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_media_control_result(&addr, mcs_id, opcode, result);
}

static void adpt_lea_mcc_search_control_result_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t result)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_search_control_result(&addr, mcs_id, result);
}

static void adpt_lea_mcc_current_track_segments_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_current_track_segments_object_id(&addr, mcs_id, obj_id);
}

static void adpt_lea_mcc_current_track_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_current_track_object_id(&addr, mcs_id, obj_id);
}

static void adpt_lea_mcc_next_track_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_next_track_object_id(&addr, mcs_id, obj_id);
}

static void adpt_lea_mcc_parent_group_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_parent_group_object_id(&addr, mcs_id, obj_id);
}

static void adpt_lea_mcc_current_group_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_current_group_object_id(&addr, mcs_id, obj_id);
}

static void adpt_lea_mcc_search_results_object_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, LEA_OBJ_ID obj_id)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_search_results_object_id(&addr, mcs_id, obj_id);
}

static void adpt_lea_mcc_content_control_id_callback(BD_ADDR mcs_addr, uint32_t mcs_id, uint8_t ccid)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mcs_addr, BD_ADDR_SIZE);
    lea_mcpc_on_content_control_id(&addr, mcs_id, ccid);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bt_status_t bt_sal_lea_mcc_read_media_player_name(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_media_player_name(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_media_player_icon_object_id(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_media_player_icon_object_id(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_media_player_icon_url(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_media_player_icon_url(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_playback_speed(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_playback_speed(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_seeking_speed(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_seeking_speed(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_playing_order(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_playing_order(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_playing_orders_supported(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_playing_orders_supported(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_media_control_opcodes_supported(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_media_control_opcodes_supported(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_track_title(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_track_title(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_track_duration(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_track_duration(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_track_position(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_track_position(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_media_state(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_media_state(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_media_control_request(bt_address_t *addr, uint32_t mcs_id,
                                                 LEA_MCC_MEDIA_CONTROL_OPCODE opcode, int32_t n)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_media_control_request(addr->addr, mcs_id, opcode, n), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_search_control_request(bt_address_t *addr, uint32_t mcs_id,
                                                  uint8_t number, LEA_MCC_SEARCH_CONTROL_ITEM_TYPE type, uint8_t *parameter)
{
    SAL_CHECK_PARAM(addr);

    SERVICS_LEA_MCS_SEARCH_CONTROL_ITEM_S *items;
    items = (SERVICS_LEA_MCS_SEARCH_CONTROL_ITEM_S *)malloc(sizeof(SERVICS_LEA_MCS_SEARCH_CONTROL_ITEM_S));
    items->type = type;
    items->parameter = parameter;

    SAL_CHECK_RET(stack_adapter_lea_mcc_search_control_request(addr->addr, mcs_id, number, items), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_current_track_object_id(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_current_track_object_id(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_next_track_object_id(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_next_track_object_id(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_parent_group_object_id(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_parent_group_object_id(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_current_group_object_id(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_current_group_object_id(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_search_results_object_id(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_search_results_object_id(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcc_read_content_control_id(bt_address_t *addr, uint32_t mcs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_mcc_read_content_control_id(addr->addr, mcs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif