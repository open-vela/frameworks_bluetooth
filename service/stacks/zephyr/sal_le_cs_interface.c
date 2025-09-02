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

#include "sal_le_cs_interface.h"
#include <zephyr/bluetooth/cs.h>
#include "sal_interface.h"
bt_status_t bt_sal_cs_read_remote_supported_capabilities(bt_controller_id_t id, bt_address_t* addr)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_remote_supported_capabilities(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_default_settings(bt_controller_id_t id, bt_address_t* addr, cs_bt_le_cs_set_default_settings_param_t *params)
{
	const struct bt_le_cs_set_default_settings_param default_settings = {
		.enable_initiator_role = params->enable_initiator_role,
		.enable_reflector_role = params->enable_reflector_role,
		.cs_sync_antenna_selection = params->cs_sync_antenna_selection,
		.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
	};

    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_default_settings(conn, params), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_read_remote_fae_table(bt_controller_id_t id, bt_address_t* addr)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_remote_fae_table(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
    
}

bt_status_t bt_sal_cs_create_config(bt_controller_id_t id, bt_address_t* addr,
                            struct cs_bt_le_cs_create_config_params_t *params,
			                enum bt_le_cs_create_config_context context)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    bt_le_cs_create_config_params config = {};
    memcpy(&config, params, sizeof(bt_le_cs_create_config_params));
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_create_config(conn, config, (cs_bt_le_cs_create_config_context_t)context), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_remove_config(uint8_t config_id)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_remove_config(conn, config_id), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_security_enable(bt_controller_id_t id, bt_address_t* addr)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_security_enable(), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_procedure_enable(bt_address_t* addr
			      const struct cs_bt_le_cs_procedure_enable_param_t *params)
{
    struct bt_le_cs_procedure_enable_param enable = {};
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    memcpy(&enable, params, sizeof(bt_le_cs_procedure_enable_param));

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_procedure_enable(conn, &enable), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_remove_config(bt_controller_id_t id, bt_address_t* addr, uint8_t config_id)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_remove_config(conn, config_id), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_procedure_parameters(bt_controller_id_t id, bt_address_t* addr,
				      const struct cs_bt_le_cs_set_procedure_parameters_param_t *params)
{
    struct bt_le_cs_set_procedure_parameters_param parameters = {};
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    memcpy(&parameters, params, sizeof(bt_le_cs_set_procedure_parameters_param));

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_procedure_parameters(conn, &parameters), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_channel_classification(uint8_t channel_classification[10])
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_channel_classification(channel_classification), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_read_local_supported_capabilities(struct cs_bt_conn_le_cs_capabilities_t *params)
{
    struct bt_conn_le_cs_capabilities capabilities = {};
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    memcpy(&capabilities, params, sizeof(bt_conn_le_cs_capabilities));
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_local_supported_capabilities(&capabilities), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_write_cached_remote_supported_capabilities(
	const struct cs_bt_conn_le_cs_capabilities_t *params)
{
    struct bt_conn_le_cs_capabilities capabilities = {};
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    memcpy(&capabilities, params, sizeof(bt_conn_le_cs_capabilities));
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_write_cached_remote_supported_capabilities(conn, &capabilities), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}




