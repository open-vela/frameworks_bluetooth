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
#include "utils/log.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/cs.h>

#if defined(CONFIG_BLUETOOTH_LE_CS) && defined(CONFIG_BT_CHANNEL_SOUNDING)

bt_status_t bt_sal_cs_read_remote_supported_capabilities(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGW("sal cs read remote capabilities, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, &le_addr);

    if (!conn) {
        BT_LOGE("cs read remote capabilities, doesn't find connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_remote_supported_capabilities(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_default_settings(bt_controller_id_t id, bt_address_t* addr, bt_le_srv_cs_set_default_settings_param_t* params)
{
    if (!addr || !params) {
        BT_LOGW("sal cs set default settings, invalid params or addr.");
        return BT_STATUS_PARM_INVALID;
    }

    const struct bt_le_cs_set_default_settings_param default_settings = {
        .enable_initiator_role = params->enable_initiator_role,
        .enable_reflector_role = params->enable_reflector_role,
        .cs_sync_antenna_selection = params->cs_sync_antenna_selection,
        .max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
    };

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, &le_addr);

    if (!conn) {
        BT_LOGE("sal cs set default settings, doesn't find connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_default_settings(conn, &default_settings), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_read_remote_fae_table(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs read remote fae table, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, &le_addr);

    if (!conn) {
        BT_LOGE("sal cs read remote fae table, doesn't find connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_remote_fae_table(conn), 0, conn);

    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;
}

static struct bt_le_cs_create_config_params* convert_cs_config_params_to_zblue(bt_le_srv_cs_create_config_params_t* params)
{
    struct bt_le_cs_create_config_params* config = (struct bt_le_cs_create_config_params*)zalloc(sizeof(struct bt_le_cs_create_config_params));

    if (!config) {
        BT_LOGE("cs create config, alloc memory failed.");
        return NULL;
    }

    if (!params) {
        BT_LOGE("cs create config, invalid params.");
        free(config);
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
        break;
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
        break;
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
        break;
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
        break;
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
        break;
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
        break;
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
        break;
    }

    config->ch3c_jump = params->ch3c_jump;
    memcpy(config->channel_map, params->channel_map, sizeof(params->channel_map));
    return config;
}

bt_status_t bt_sal_cs_create_config(bt_controller_id_t id, bt_address_t* addr,
    bt_le_srv_cs_create_config_params_t* params,
    bt_le_srv_cs_create_config_context_t context)
{
    if (!addr) {
        BT_LOGE("cs create config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, &le_addr);

    if (!conn) {
        BT_LOGE("cs create config, doesn't find connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    struct bt_le_cs_create_config_params* config = convert_cs_config_params_to_zblue(params);

    switch (context) {
    case CS_BT_SRV_CONN_LE_CS_SUB_MODE_1:
        SAL_CHECK_RET_WITH_CONN(bt_le_cs_create_config(conn, config, BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_ONLY), 0, conn);
        free(config);
        break;
    case CS_BT_SRV_CONN_LE_CS_SUB_MODE_2:
        SAL_CHECK_RET_WITH_CONN(bt_le_cs_create_config(conn, config, BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE), 0, conn);
        free(config);
        break;
    default:
        BT_LOGE("cs create config, invalid context.");
        bt_conn_unref(conn);
        return BT_STATUS_FAIL;
    }

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_security_enable(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs create config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, &le_addr);

    if (!conn) {
        BT_LOGE("cs security enable, doesn't find connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_security_enable(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_procedure_enable(bt_address_t* addr,
    const bt_le_srv_cs_procedure_enable_param_t* params)
{
    if (!addr) {
        BT_LOGE("cs create config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));
    struct bt_le_cs_procedure_enable_param enable = {};
    struct bt_conn* conn = bt_conn_lookup_addr_le(0, &le_addr);

    if (!conn) {
        BT_LOGE("cs procedure enable, doesn't find the connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    enable.config_id = params->config_id;
    enable.enable = params->enable;
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_procedure_enable(conn, &enable), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_remove_config(bt_controller_id_t id, bt_address_t* addr, uint8_t config_id)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, &le_addr);

    if (!conn) {
        BT_LOGE("cs remove config, doesn't find the connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_remove_config(conn, config_id), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

static struct bt_le_cs_set_procedure_parameters_param* convert_cs_set_procedure_parameters_params_to_zblue(const bt_le_srv_cs_set_procedure_parameters_param_t* params)
{
    struct bt_le_cs_set_procedure_parameters_param* procedure = (struct bt_le_cs_set_procedure_parameters_param*)malloc(sizeof(struct bt_le_cs_set_procedure_parameters_param));
    if (!procedure) {
        BT_LOGE("cs set procedure parameters, malloc failed.");
        return NULL;
    }

    if (!params) {
        BT_LOGE("cs set procedure parameters, invalid params.");
        free(procedure);
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
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_TWO:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_TWO;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_THREE:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_THREE;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SIX:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SIX;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN;
        break;
    case CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_EIGHT:
        procedure->tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_EIGHT;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid tone antenna config selection.");
        break;
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
        break;
    }

    procedure->tx_power_delta = params->tx_power_delta;
    procedure->preferred_peer_antenna = params->preferred_peer_antenna;

    switch (params->snr_control_initiator) {
    case BT_LE_SRV_CS_INITIATOR_SNR_CONTROL_18dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_18dB;
        break;
    case BT_LE_SRV_CS_INITIATOR_SNR_CONTROL_21dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_21dB;
        break;
    case BT_LE_SRV_CS_INITIATOR_SNR_CONTROL_24dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_24dB;
        break;
    case BT_LE_SRV_CS_INITIATOR_SNR_CONTROL_27dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_27dB;
        break;
    case BT_LE_SRV_CS_INITIATOR_SNR_CONTROL_30dB:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_30dB;
        break;
    case BT_LE_SRV_CS_INITIATOR_SNR_CONTROL_NOT_USED:
        procedure->snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_NOT_USED;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid snr control initiator.");
        break;
    }

    switch (params->snr_control_reflector) {
    case BT_LE_SRV_CS_REFLECTOR_SNR_CONTROL_18dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_18dB;
        break;
    case BT_LE_SRV_CS_REFLECTOR_SNR_CONTROL_21dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_21dB;
        break;
    case BT_LE_SRV_CS_REFLECTOR_SNR_CONTROL_24dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_24dB;
        break;
    case BT_LE_SRV_CS_REFLECTOR_SNR_CONTROL_27dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_27dB;
        break;
    case BT_LE_SRV_CS_REFLECTOR_SNR_CONTROL_30dB:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_30dB;
        break;
    case BT_LE_SRV_CS_REFLECTOR_SNR_CONTROL_NOT_USED:
        procedure->snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_NOT_USED;
        break;
    default:
        BT_LOGE("cs set procedure parameters, invalid snr control initiator.");
        break;
    }

    return procedure;
}

bt_status_t bt_sal_cs_set_procedure_parameters(bt_controller_id_t id, bt_address_t* addr,
    const bt_le_srv_cs_set_procedure_parameters_param_t* params)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, &le_addr);

    if (!conn) {
        BT_LOGE("cs set procedure parameters, doesn't find connection for addr:%s",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    struct bt_le_cs_set_procedure_parameters_param* parameters = convert_cs_set_procedure_parameters_params_to_zblue(params);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_procedure_parameters(conn, parameters), 0, conn);

    free(parameters);
    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_channel_classification(uint8_t channel_classification[10], bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(0, &le_addr);

    if (!conn) {
        BT_LOGE("cs set channel classificaition, doesn't find connection for addr:%s.",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_channel_classification(channel_classification), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

static struct bt_conn_le_cs_capabilities* convert_cs_capabilities_to_zblue(bt_srv_conn_le_cs_capabilities_t* params)
{
    struct bt_conn_le_cs_capabilities* capbs = (struct bt_conn_le_cs_capabilities*)zalloc(sizeof(struct bt_conn_le_cs_capabilities));

    if (!capbs) {
        BT_LOGE("cs set procedure parameters, allocate memory failed.");
        return NULL;
    }

    if (!params) {
        BT_LOGE("cs set procedure parameters, invalid params.");
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
        break;
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
        break;
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
        break;
    }

    capbs->rtt_aa_only_n = params->rtt_aa_only_n;
    capbs->rtt_sounding_n = params->rtt_sounding_n;
    capbs->rtt_random_payload_n = params->rtt_random_payload_n;
    capbs->phase_based_nadm_sounding_supported = params->phase_based_nadm_sounding_supported;
    capbs->phase_based_nadm_random_supported = params->phase_based_nadm_random_supported;
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

static void convert_cs_capabilities_to_service(bt_srv_conn_le_cs_capabilities_t* capabilities, struct bt_conn_le_cs_capabilities* params)
{

    bt_srv_conn_le_cs_capabilities_t* capabilities = (bt_srv_conn_le_cs_capabilities_t*)zalloc(sizeof(bt_srv_conn_le_cs_capabilities_t));

    if (!capabilities || !params) {
        BT_LOGE("Invalid cs params.");
        return;
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
        break;
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
        break;
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
        break;
    }

    capabilities->rtt_aa_only_n = params->rtt_aa_only_n;
    capabilities->rtt_sounding_n = params->rtt_sounding_n;
    capabilities->rtt_random_payload_n = params->rtt_random_payload_n;
    capabilities->phase_based_nadm_sounding_supported = params->phase_based_nadm_sounding_supported;
    capabilities->phase_based_nadm_random_supported = params->phase_based_nadm_random_supported;
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
}

bt_status_t bt_sal_cs_read_local_supported_capabilities(bt_srv_conn_le_cs_capabilities_t* params, bt_address_t* addr)
{
    if (!params || !addr) {
        BT_LOGE("cs read local supported capabilities, invalid params or addrs.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(0, &le_addr);

    if (!conn) {
        BT_LOGE("cs read local supported capabilities, doesn't find connection for addr:%s.",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    struct bt_conn_le_cs_capabilities* capabilities = convert_cs_capabilities_to_zblue(params);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_local_supported_capabilities(capabilities), 0, conn);

    convert_cs_capabilities_to_service(params, capabilities);

    free(capabilities);
    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_write_cached_remote_supported_capabilities(
    bt_srv_conn_le_cs_capabilities_t* params, bt_address_t* addr)
{
    if (!params || !addr) {
        BT_LOGE("cs write cached remote supported capabilites, invalid params or addrs.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = { 0 };
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(0, &le_addr);

    if (!conn) {
        BT_LOGE("cs write cached remote supported capabilites, doesn't find connection for addr:%s.",
            bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    struct bt_conn_le_cs_capabilities* capabilities = convert_cs_capabilities_to_zblue(params);
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_write_cached_remote_supported_capabilities(conn, capabilities), 0, conn);

    free(capabilities);
    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_LE_CS && CONFIG_BT_CHANNEL_SOUNDING */
