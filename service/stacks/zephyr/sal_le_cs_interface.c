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

#include "sal_le_cs_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "service_loop.h"
#include "utils/log.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/cs.h>

#if defined(CONFIG_BLUETOOTH_LE_CS) && defined(CONFIG_BT_CHANNEL_SOUNDING)

#define STACK_CALL(func) zblue_##func

typedef bt_status_t (*sal_cs_func_t)(void* args);

typedef union {
    bt_le_srv_cs_set_default_settings_param_t set_default_settings;
    struct {
        bt_le_srv_cs_create_config_params_t param;
        bt_le_srv_cs_create_config_context_t context;

    } create_config;
    bt_le_srv_cs_procedure_enable_param_t enable;
    uint8_t config_id;
    bt_le_srv_cs_set_procedure_parameters_param_t set_procedure_parameters;
    uint8_t channel_classification[10];
    bt_srv_conn_le_cs_capabilities_t capabilities;
} sal_cs_args_t;

typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type;
    sal_cs_func_t func;
    sal_cs_args_t args;
} sal_cs_req_t;

static sal_cs_req_t* sal_cs_req(bt_controller_id_t id, bt_address_t* addr, sal_cs_func_t func)
{
    sal_cs_req_t* req = calloc(sizeof(sal_cs_req_t), 1);

    if (req) {
        req->id = id;
        req->func = func;
        if (addr)
            memcpy(&req->addr, addr, sizeof(bt_address_t));
    }

    return req;
}

static void sal_invoke_async(service_work_t* work, void* userdata)
{
    sal_cs_req_t* req = userdata;

    SAL_ASSERT(req);
    if (req->func(req) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, fail", __func__);
    }

    free(userdata);
}

static bt_status_t sal_send_req(sal_cs_req_t* req)
{
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t STACK_CALL(read_remote_supported_capabilities)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs read remote capabilities, doesn't find connection for addr");
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_read_remote_supported_capabilities(info->conn);
    if (err) {
        BT_LOGE("err: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_read_remote_supported_capabilities(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGW("sal cs read remote capabilities, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(read_remote_supported_capabilities));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

static bt_status_t STACK_CALL(set_default_settings)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("sal cs set default settings, doesn't find connection");
        return BT_STATUS_FAIL;
    }

    const struct bt_le_cs_set_default_settings_param default_settings = {
        .enable_initiator_role = req->args.set_default_settings.enable_initiator_role,
        .enable_reflector_role = req->args.set_default_settings.enable_reflector_role,
        .cs_sync_antenna_selection = req->args.set_default_settings.cs_sync_antenna_selection,
        .max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
    };

    int err = bt_le_cs_set_default_settings(info->conn, &default_settings);
    if (err) {
        BT_LOGE("err: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_default_settings(bt_controller_id_t id, bt_address_t* addr, bt_le_srv_cs_set_default_settings_param_t* params)
{
    if (!addr || !params) {
        BT_LOGW("sal cs set default settings, invalid params or addr.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(set_default_settings));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&req->args.set_default_settings, params, sizeof(*params));

    return sal_send_req(req);
}

static bt_status_t STACK_CALL(read_remote_fae_table)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("sal cs read remote fae table, doesn't find connection");
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_read_remote_fae_table(info->conn);
    if (err) {
        BT_LOGE("err: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_read_remote_fae_table(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs read remote fae table, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(read_remote_fae_table));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

static struct bt_le_cs_create_config_params* convert_cs_config_params_to_zblue(bt_le_srv_cs_create_config_params_t* params)
{
    struct bt_le_cs_create_config_params* config;

    if (!params) {
        BT_LOGE("cs create config, invalid params.");
        return NULL;
    }

    config = (struct bt_le_cs_create_config_params*)zalloc(sizeof(struct bt_le_cs_create_config_params));

    if (!config) {
        BT_LOGE("cs create config, alloc memory failed.");
        return NULL;
    }

    config->id = params->id;
    switch (params->main_mode_type) {
    case CS_BT_SRV_CONN_LE_CS_MAIN_MODE_1:
        config->main_mode_type = BT_CONN_LE_CS_MAIN_MODE_1;
        break;
    case CS_BT_SRV_CONN_LE_CS_MAIN_MODE_2:
        config->main_mode_type = BT_CONN_LE_CS_MAIN_MODE_2;
        break;
    case CS_BT_SRV_CONN_LE_CS_MAIN_MODE_3:
        config->main_mode_type = BT_CONN_LE_CS_MAIN_MODE_3;
        break;
    default:
        BT_LOGE("cs create config, invalid main mode type.");
        free(config);
        return NULL;
    }

    switch (params->sub_mode_type) {
    case CS_BT_SRV_CONN_LE_CS_SUB_MODE_1:
        config->sub_mode_type = BT_CONN_LE_CS_SUB_MODE_1;
        break;
    case CS_BT_SRV_CONN_LE_CS_SUB_MODE_2:
        config->sub_mode_type = BT_CONN_LE_CS_SUB_MODE_2;
        break;
    case CS_BT_SRV_CONN_LE_CS_SUB_MODE_3:
        config->sub_mode_type = BT_CONN_LE_CS_SUB_MODE_3;
        break;
    case CS_BT_SRV_CONN_LE_CS_SUB_MODE_UNUSED:
        config->sub_mode_type = BT_CONN_LE_CS_SUB_MODE_UNUSED;
        break;
    default:
        BT_LOGE("cs create config, invalid sub mode type.");
        free(config);
        return NULL;
    }

    config->min_main_mode_steps = params->min_main_mode_steps;
    config->max_main_mode_steps = params->max_main_mode_steps;
    config->main_mode_repetition = params->main_mode_repetition;
    config->mode_0_steps = params->mode_0_steps;
    switch (params->role) {
    case CS_BT_SRV_CONN_LE_CS_ROLE_INITIATOR:
        config->role = BT_CONN_LE_CS_ROLE_INITIATOR;
        break;
    case CS_BT_SRV_CONN_LE_CS_ROLE_REFLECTOR:
        config->role = BT_CONN_LE_CS_ROLE_REFLECTOR;
        break;
    default:
        BT_LOGE("cs create config, invalid role.");
        free(config);
        return NULL;
    }

    switch (params->rtt_type) {
    case CS_BT_SRV_CONN_LE_CS_RTT_TYPE_AA_ONLY:
        config->rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_TYPE_32_BIT_SOUNDING:
        config->rtt_type = BT_CONN_LE_CS_RTT_TYPE_32_BIT_SOUNDING;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_TYPE_96_BIT_SOUNDING:
        config->rtt_type = BT_CONN_LE_CS_RTT_TYPE_96_BIT_SOUNDING;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_TYPE_32_BIT_RANDOM:
        config->rtt_type = BT_CONN_LE_CS_RTT_TYPE_32_BIT_RANDOM;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_TYPE_64_BIT_RANDOM:
        config->rtt_type = BT_CONN_LE_CS_RTT_TYPE_64_BIT_RANDOM;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_TYPE_96_BIT_RANDOM:
        config->rtt_type = BT_CONN_LE_CS_RTT_TYPE_96_BIT_RANDOM;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_TYPE_128_BIT_RANDOM:
        config->rtt_type = BT_CONN_LE_CS_RTT_TYPE_128_BIT_RANDOM;
        break;
    default:
        BT_LOGE("cs create config, invalid rtt type.");
        free(config);
        return NULL;
    }

    switch (params->cs_sync_phy) {
    case CS_BT_SRV_CONN_LE_CS_SYNC_1M_PHY:
        config->cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY;
        break;
    case CS_BT_SRV_CONN_LE_CS_SYNC_2M_PHY:
        config->cs_sync_phy = BT_CONN_LE_CS_SYNC_2M_PHY;
        break;
    case CS_BT_SRV_CONN_LE_CS_SYNC_2M_2BT_PHY:
        config->cs_sync_phy = BT_CONN_LE_CS_SYNC_2M_2BT_PHY;
        break;
    default:
        BT_LOGE("cs create config, invalid cs sync phy.");
        free(config);
        return NULL;
    }

    config->channel_map_repetition = params->channel_map_repetition;
    switch (params->channel_selection_type) {
    case CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3B:
        config->channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B;
        break;
    case CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3C:
        config->channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3C;
        break;
    default:
        BT_LOGE("cs create config, invalid channel selection type.");
        free(config);
        return NULL;
    }

    switch (params->ch3c_shape) {
    case CS_BT_SRV_CONN_LE_CS_CH3C_SHAPE_HAT:
        config->ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT;
        break;
    case CS_BT_SRV_CONN_LE_CS_CH3C_SHAPE_X:
        config->ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_X;
        break;
    default:
        BT_LOGE("cs create config, invalid ch3c shape.");
        free(config);
        return NULL;
    }

    config->ch3c_jump = params->ch3c_jump;
    memcpy(config->channel_map, params->channel_map, sizeof(params->channel_map));
    return config;
}

static bt_status_t STACK_CALL(create_config)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs create config, doesn't find connection");
        return BT_STATUS_FAIL;
    }

    struct bt_le_cs_create_config_params* config = convert_cs_config_params_to_zblue(&req->args.create_config.param);
    if (config == NULL) {
        BT_LOGE("cs create config, failed to convert params.");
        return BT_STATUS_FAIL;
    }

    int err = 0;

    switch (req->args.create_config.context) {
    case BT_LE_SRV_CS_CREATE_CONFIG_CONTEXT_LOCAL_ONLY:
        err = bt_le_cs_create_config(info->conn, config, BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_ONLY);
        break;
    case BT_LE_SRV_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE:
        err = bt_le_cs_create_config(info->conn, config, BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE);
        break;
    default:
        BT_LOGE("cs create config, invalid context.");
        free(config);
        return BT_STATUS_FAIL;
    }

    if (err) {
        BT_LOGE("err: %d", err);
        free(config);
        return BT_STATUS_FAIL;
    }

    free(config);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_create_config(bt_controller_id_t id, bt_address_t* addr,
    bt_le_srv_cs_create_config_params_t* params,
    bt_le_srv_cs_create_config_context_t context)
{
    if (!addr) {
        BT_LOGE("cs create config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(create_config));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&req->args.create_config.param, params, sizeof(*params));
    req->args.create_config.context = context;

    return sal_send_req(req);
}

static bt_status_t STACK_CALL(security_enable)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs security enable, doesn't find connection");
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_security_enable(info->conn);
    if (err) {
        BT_LOGE("err: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_security_enable(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs security enable, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(security_enable));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

static bt_status_t STACK_CALL(procedure_enable)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs procedure enable, doesn't find the connection");
        return BT_STATUS_FAIL;
    }

    struct bt_le_cs_procedure_enable_param enable = { 0 };

    enable.config_id = req->args.enable.config_id;
    enable.enable = req->args.enable.enable;
    int err = bt_le_cs_procedure_enable(info->conn, &enable);
    if (err) {
        BT_LOGE("err: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_procedure_enable(bt_controller_id_t id, bt_address_t* addr,
    const bt_le_srv_cs_procedure_enable_param_t* params)
{
    if (!addr) {
        BT_LOGE("cs procedure enable, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    if (!params) {
        BT_LOGE("cs procedure enable, invalid params.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(procedure_enable));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&req->args.enable, params, sizeof(*params));

    return sal_send_req(req);
}

static bt_status_t STACK_CALL(remove_config)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs remove config, doesn't find the connection");
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_remove_config(info->conn, req->args.config_id);
    if (err) {
        BT_LOGE("err: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_remove_config(bt_controller_id_t id, bt_address_t* addr, uint8_t config_id)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(remove_config));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->args.config_id = config_id;

    return sal_send_req(req);
}

static struct bt_le_cs_set_procedure_parameters_param* convert_cs_set_procedure_parameters_params_to_zblue(const bt_le_srv_cs_set_procedure_parameters_param_t* params)
{
    struct bt_le_cs_set_procedure_parameters_param* procedure;

    if (!params) {
        BT_LOGE("cs set procedure parameters, invalid params.");
        return NULL;
    }

    procedure = (struct bt_le_cs_set_procedure_parameters_param*)malloc(sizeof(struct bt_le_cs_set_procedure_parameters_param));

    if (!procedure) {
        BT_LOGE("cs set procedure parameters, malloc failed.");
        return NULL;
    }

    procedure->config_id = params->config_id;
    procedure->max_procedure_len = params->max_procedure_len;
    procedure->min_procedure_interval = params->min_procedure_interval;
    procedure->max_procedure_interval = params->max_procedure_interval;
    procedure->max_procedure_count = params->max_procedure_count;
    procedure->min_subevent_len = params->min_subevent_len;
    procedure->max_subevent_len = params->max_subevent_len;

    switch (params->tone_antenna_config_selection) {
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ZERO:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_TWO;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_TWO:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_THREE;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_THREE:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SIX;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SIX:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_EIGHT;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid tone antenna config selection.");
        free(procedure);
        return NULL;
    }

    switch (params->phy) {
    case BT_LE_SRV_CS_PROCEDURE_PHY_1M:
        procedure->phy = BT_LE_CS_PROCEDURE_PHY_1M;
        break;
    case BT_LE_SRV_CS_PROCEDURE_PHY_2M:
        procedure->phy = BT_LE_CS_PROCEDURE_PHY_2M;
        break;
    case BT_LE_SRV_CS_PROCEDURE_PHY_CODED_S8:
        procedure->phy = BT_LE_CS_PROCEDURE_PHY_CODED_S8;
        break;
    case BT_LE_SRV_CS_PROCEDURE_PHY_CODED_S2:
        procedure->phy = BT_LE_CS_PROCEDURE_PHY_CODED_S2;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid phy.");
        free(procedure);
        return NULL;
    }

    procedure->tx_power_delta = params->tx_power_delta;
    procedure->preferred_peer_antenna = params->preferred_peer_antenna;

    switch (params->snr_control_initiator) {
    case BT_LE_SRV_CS_SNR_CONTROL_18dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_18dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_21dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_21dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_24dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_24dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_27dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_27dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_30dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_30dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_NOT_USED:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_NOT_USED;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid snr control initiator.");
        free(procedure);
        return NULL;
    }

    switch (params->snr_control_reflector) {
    case BT_LE_SRV_CS_SNR_CONTROL_18dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_18dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_21dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_21dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_24dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_24dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_27dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_27dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_30dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_30dB;
        break;
    case BT_LE_SRV_CS_SNR_CONTROL_NOT_USED:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_NOT_USED;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid snr control initiator.");
        free(procedure);
        return NULL;
    }

    return procedure;
}

static bt_status_t STACK_CALL(set_procedure_parameters)(void* args)
{
    sal_cs_req_t* req = args;
    struct bt_le_cs_set_procedure_parameters_param* parameters;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs set procedure parameters, doesn't find connection");
        return BT_STATUS_FAIL;
    }

    parameters = convert_cs_set_procedure_parameters_params_to_zblue(&req->args.set_procedure_parameters);
    if (!parameters) {
        BT_LOGE("cs set procedure parameters, convert params failed.");
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_set_procedure_parameters(info->conn, parameters);
    if (err) {
        BT_LOGE("err: %d", err);
        free(parameters);
        return BT_STATUS_FAIL;
    }

    free(parameters);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_procedure_parameters(bt_controller_id_t id, bt_address_t* addr,
    const bt_le_srv_cs_set_procedure_parameters_param_t* params)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    if (!params) {
        BT_LOGE("cs set procedure, invalid params.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(set_procedure_parameters));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&req->args.set_procedure_parameters, params, sizeof(*params));

    return sal_send_req(req);
}

static bt_status_t STACK_CALL(set_channel_classification)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs set channel classificaition, doesn't find connection.");
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_set_channel_classification(req->args.channel_classification);
    if (err) {
        BT_LOGE("err: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_channel_classification(bt_controller_id_t id, uint8_t channel_classification[10], bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(set_channel_classification));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(req->args.channel_classification, channel_classification, 10 * sizeof(uint8_t));

    return sal_send_req(req);
}

static struct bt_conn_le_cs_capabilities* convert_cs_capabilities_to_zblue(bt_srv_conn_le_cs_capabilities_t* params)
{
    struct bt_conn_le_cs_capabilities* capbs;

    if (!params) {
        BT_LOGE("cs set procedure parameters, invalid params.");
        return NULL;
    }

    capbs = (struct bt_conn_le_cs_capabilities*)zalloc(sizeof(struct bt_conn_le_cs_capabilities));

    if (!capbs) {
        BT_LOGE("cs set procedure parameters, allocate memory failed.");
        return NULL;
    }

    capbs->num_config_supported = params->num_config_supported;
    capbs->max_consecutive_procedures_supported = params->max_consecutive_procedures_supported;
    capbs->num_antennas_supported = params->num_antennas_supported;
    capbs->max_antenna_paths_supported = params->max_antenna_paths_supported;
    capbs->initiator_supported = params->initiator_supported;
    capbs->reflector_supported = params->reflector_supported;
    capbs->mode_3_supported = params->mode_3_supported;

    switch (params->rtt_aa_only_precision) {
    case CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP:
        capbs->rtt_aa_only_precision = BT_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_10NS:
        capbs->rtt_aa_only_precision = BT_CONN_LE_CS_RTT_AA_ONLY_10NS;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_150NS:
        capbs->rtt_aa_only_precision = BT_CONN_LE_CS_RTT_AA_ONLY_150NS;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid rtt aa only precision.");
        free(capbs);
        return NULL;
    }

    switch (params->rtt_sounding_precision) {
    case CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP:
        capbs->rtt_sounding_precision = BT_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_10NS:
        capbs->rtt_sounding_precision = BT_CONN_LE_CS_RTT_SOUNDING_10NS;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_150NS:
        capbs->rtt_sounding_precision = BT_CONN_LE_CS_RTT_SOUNDING_150NS;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid rtt sounding precision.");
        free(capbs);
        return NULL;
    }

    switch (params->rtt_random_payload_precision) {
    case CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP:
        capbs->rtt_random_payload_precision = BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS:
        capbs->rtt_random_payload_precision = BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS;
        break;
    case CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS:
        capbs->rtt_random_payload_precision = BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid rtt random payload precision.");
        free(capbs);
        return NULL;
    }

    capbs->rtt_aa_only_n = params->rtt_aa_only_n;
    capbs->rtt_sounding_n = params->rtt_sounding_n;
    capbs->rtt_random_payload_n = params->rtt_random_payload_n;
    capbs->phase_based_nadm_sounding_supported = params->amplitude_based_nadm_sounding_supported;
    capbs->phase_based_nadm_random_supported = params->amplitude_based_nadm_random_supported;
    capbs->cs_sync_2m_phy_supported = params->cs_sync_2m_phy_supported;
    capbs->cs_sync_2m_2bt_phy_supported = params->cs_sync_2m_2bt_phy_supported;
    capbs->cs_without_fae_supported = params->cs_without_fae_supported;
    capbs->chsel_alg_3c_supported = params->chsel_alg_3c_supported;
    capbs->pbr_from_rtt_sounding_seq_supported = params->pbr_from_rtt_sounding_seq_supported;
    capbs->t_ip1_times_supported = params->t_ip1_times_supported;
    capbs->t_ip2_times_supported = params->t_ip2_times_supported;
    capbs->t_fcs_times_supported = params->t_fcs_times_supported;
    capbs->t_pm_times_supported = params->t_pm_times_supported;
    capbs->t_sw_time = params->t_sw_time;
    capbs->tx_snr_capability = params->tx_snr_capability;

    return capbs;
}

static bt_status_t convert_cs_capabilities_to_service(bt_srv_conn_le_cs_capabilities_t* capabilities, struct bt_conn_le_cs_capabilities* params)
{
    if (!params) {
        BT_LOGE("cs get procedure parameters, invalid params.");
        return BT_STATUS_PARM_INVALID;
    }

    if (!capabilities) {
        BT_LOGE("cs get procedure parameters, invalid capabilities.");
        return BT_STATUS_PARM_INVALID;
    }

    capabilities->num_config_supported = params->num_config_supported;
    capabilities->max_consecutive_procedures_supported = params->max_consecutive_procedures_supported;
    capabilities->num_antennas_supported = params->num_antennas_supported;
    capabilities->max_antenna_paths_supported = params->max_antenna_paths_supported;
    capabilities->initiator_supported = params->initiator_supported;
    capabilities->reflector_supported = params->reflector_supported;
    capabilities->mode_3_supported = params->mode_3_supported;

    switch (params->rtt_aa_only_precision) {
    case BT_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP:
        capabilities->rtt_aa_only_precision = CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP;
        break;
    case BT_CONN_LE_CS_RTT_AA_ONLY_10NS:
        capabilities->rtt_aa_only_precision = CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_10NS;
        break;
    case BT_CONN_LE_CS_RTT_AA_ONLY_150NS:
        capabilities->rtt_aa_only_precision = CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_150NS;
        break;
    default:
        BT_LOGE("Invalid rtt aa only precision.");
        return BT_STATUS_FAIL;
    }

    switch (params->rtt_sounding_precision) {
    case BT_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP:
        capabilities->rtt_sounding_precision = CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP;
        break;
    case BT_CONN_LE_CS_RTT_SOUNDING_10NS:
        capabilities->rtt_sounding_precision = CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_10NS;
        break;
    case BT_CONN_LE_CS_RTT_SOUNDING_150NS:
        capabilities->rtt_sounding_precision = CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_150NS;
        break;
    default:
        BT_LOGE("Invalid rtt sounding precision.");
        return BT_STATUS_FAIL;
    }

    switch (params->rtt_random_payload_precision) {
    case BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP:
        capabilities->rtt_random_payload_precision = CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP;
        break;
    case BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS:
        capabilities->rtt_random_payload_precision = CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS;
        break;
    case BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS:
        capabilities->rtt_random_payload_precision = CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS;
        break;
    default:
        BT_LOGE("Invalid rtt random payload precision.");
        return BT_STATUS_FAIL;
    }

    capabilities->rtt_aa_only_n = params->rtt_aa_only_n;
    capabilities->rtt_sounding_n = params->rtt_sounding_n;
    capabilities->rtt_random_payload_n = params->rtt_random_payload_n;
    capabilities->amplitude_based_nadm_sounding_supported = params->phase_based_nadm_sounding_supported;
    capabilities->amplitude_based_nadm_random_supported = params->phase_based_nadm_random_supported;
    capabilities->cs_sync_2m_phy_supported = params->cs_sync_2m_phy_supported;
    capabilities->cs_sync_2m_2bt_phy_supported = params->cs_sync_2m_2bt_phy_supported;
    capabilities->cs_without_fae_supported = params->cs_without_fae_supported;
    capabilities->chsel_alg_3c_supported = params->chsel_alg_3c_supported;
    capabilities->pbr_from_rtt_sounding_seq_supported = params->pbr_from_rtt_sounding_seq_supported;
    capabilities->t_ip1_times_supported = params->t_ip1_times_supported;
    capabilities->t_ip2_times_supported = params->t_ip2_times_supported;
    capabilities->t_fcs_times_supported = params->t_fcs_times_supported;
    capabilities->t_pm_times_supported = params->t_pm_times_supported;
    capabilities->t_sw_time = params->t_sw_time;
    capabilities->tx_snr_capability = params->tx_snr_capability;

    return BT_STATUS_SUCCESS;
}

static bt_status_t STACK_CALL(read_local_supported_capabilities)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs read local supported capabilities, doesn't find connection.");
        return BT_STATUS_FAIL;
    }

    struct bt_conn_le_cs_capabilities* capabilities = convert_cs_capabilities_to_zblue(&req->args.capabilities);
    if (!capabilities) {
        BT_LOGE("cs read local supported capabilities, convert cs capabilities to zblue failed.");
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_read_local_supported_capabilities(capabilities);
    if (err) {
        BT_LOGE("err: %d", err);
        free(capabilities);
        return BT_STATUS_FAIL;
    }

    bt_srv_conn_le_cs_capabilities_t* local_capabilities;
    cs_msg_t* msg = cs_msg_new(LOCAL_SUPPORTED_CAPABILITIES_EVT, &req->addr);
    if (!msg) {
        free(capabilities);
        return BT_STATUS_FAIL;
    }

    local_capabilities = (bt_srv_conn_le_cs_capabilities_t*)zalloc(sizeof(bt_srv_conn_le_cs_capabilities_t));
    if (!local_capabilities) {
        free(capabilities);
        cs_msg_destroy(msg);
        return BT_STATUS_FAIL;
    }

    if (convert_cs_capabilities_to_service(local_capabilities, capabilities) != BT_STATUS_SUCCESS) {
        BT_LOGE("cs convert capabilities to service failed.");
        free(local_capabilities);
        free(capabilities);
        cs_msg_destroy(msg);
        return BT_STATUS_FAIL;
    }
    msg->cs_data.data = (void*)local_capabilities;
    bt_sal_cs_event_callback(msg);

    free(capabilities);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_read_local_supported_capabilities(bt_controller_id_t id,
    bt_srv_conn_le_cs_capabilities_t* params, bt_address_t* addr)
{
    if (!params || !addr) {
        BT_LOGE("cs read local supported capabilities, invalid params or addrs.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(read_local_supported_capabilities));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&req->args.capabilities, params, sizeof(*params));

    return sal_send_req(req);
}

static bt_status_t STACK_CALL(write_cached_remote_supported_capabilities)(void* args)
{
    sal_cs_req_t* req = args;
    bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BLE);
    if (!info->conn) {
        BT_LOGE("cs write cached remote supported capabilites, doesn't find connection.");
        return BT_STATUS_FAIL;
    }

    struct bt_conn_le_cs_capabilities* capabilities = convert_cs_capabilities_to_zblue(&req->args.capabilities);
    if (!capabilities) {
        return BT_STATUS_FAIL;
    }

    int err = bt_le_cs_write_cached_remote_supported_capabilities(info->conn, capabilities);
    if (err) {
        BT_LOGE("err: %d", err);
        free(capabilities);
        return BT_STATUS_FAIL;
    }

    free(capabilities);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_write_cached_remote_supported_capabilities(bt_controller_id_t id,
    const bt_srv_conn_le_cs_capabilities_t* params, bt_address_t* addr)
{
    if (!params || !addr) {
        BT_LOGE("cs write cached remote supported capabilites, invalid params or addrs.");
        return BT_STATUS_PARM_INVALID;
    }

    sal_cs_req_t* req = sal_cs_req(id, addr, STACK_CALL(write_cached_remote_supported_capabilities));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&req->args.capabilities, params, sizeof(*params));

    return sal_send_req(req);
}

#endif /* CONFIG_BLUETOOTH_LE_CS && CONFIG_BT_CHANNEL_SOUNDING */
