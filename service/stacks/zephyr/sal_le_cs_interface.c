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
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include "sal_interface.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LE_CS

bt_status_t bt_sal_cs_read_remote_supported_capabilities(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGW("sal cs read remote capabilities, invalid addr.");
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs read remote capabilities, doesn't find connection for addr:%s",
            bt_fw_addr_str(addr));
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_remote_supported_capabilities(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_default_settings(bt_controller_id_t id, bt_address_t* addr, bt_le_srv_cs_set_default_settings_param_t *params)
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

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("sal cs set default settings, doesn't find connection for addr:%s",
            bt_fw_addr_str(addr));
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

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("sal cs read remote fae table, doesn't find connection for addr:%s",
            bt_fw_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_remote_fae_table(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
    
}

bt_status_t bt_sal_cs_create_config(bt_controller_id_t id, bt_address_t* addr,
                            bt_le_srv_cs_create_config_params_t *params,
			                bt_le_srv_cs_create_config_context_t context)
{
    if (!addr) {
        BT_LOGE("cs create config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs create config, doesn't find connection for addr:%s",
            bt_fw_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    struct bt_le_cs_create_config_params config = {};
    memcpy(&config, params, sizeof(bt_le_srv_cs_create_config_params_t));
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_create_config(conn, &config, (bt_le_srv_cs_create_config_context_t)context), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_security_enable(bt_controller_id_t id, bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs create config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs security enable, doesn't find connection for addr:%s",
            bt_fw_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_security_enable(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_procedure_enable(bt_address_t* addr,
			      const bt_le_srv_cs_procedure_enable_param_t *params)
{
    if (!addr) {
        BT_LOGE("cs create config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));
    struct bt_le_cs_procedure_enable_param enable = {};
    struct bt_conn* conn = bt_conn_lookup_addr_le(0, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs procedure enable, doesn't find the connection for addr:%s",
            bt_fw_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    memcpy(&enable, params, sizeof(struct bt_le_cs_procedure_enable_param));

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

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(id, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs remove config, doesn't find the connection for addr:%s",
            bt_fw_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_remove_config(conn, config_id), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_procedure_parameters(bt_controller_id_t id, bt_address_t* addr,
				      const bt_le_srv_cs_set_procedure_parameters_param_t *params)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_le_cs_set_procedure_parameters_param parameters = {};
    struct bt_conn* conn = bt_conn_lookup_addr_le(id, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs set procedure parameters, doesn't find connection for addr:%s",
            bt_fw_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    memcpy(&parameters, params, sizeof(struct bt_le_cs_set_procedure_parameters_param));

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_procedure_parameters(conn, &parameters), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_set_channel_classification(uint8_t channel_classification[10], bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("cs remove config, invalid addr.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn* conn = bt_conn_lookup_addr_le(0, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs set channel classificaition, doesn't find connection for addr:%s.",
            bt_fw_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET_WITH_CONN(bt_le_cs_set_channel_classification(channel_classification), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_read_local_supported_capabilities(bt_srv_conn_le_cs_capabilities_t *params, bt_address_t* addr)
{
    if (!params || !addr) {
        BT_LOGE("cs read local supported capabilities, invalid params or addrs.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn_le_cs_capabilities capabilities = {};
    struct bt_conn* conn = bt_conn_lookup_addr_le(0, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs read local supported capabilities, doesn't find connection for addr:%s.",
            bt_fw_addr_str(addr));
    }

    memcpy(&capabilities, params, sizeof(struct bt_conn_le_cs_capabilities));
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_read_local_supported_capabilities(&capabilities), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_cs_write_cached_remote_supported_capabilities(
	bt_srv_conn_le_cs_capabilities_t *params, bt_address_t* addr)
{
   if (!params || !addr) {
        BT_LOGE("cs write cached remote supported capabilites, invalid params or addrs.");
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_le_t le_addr = {0};
    memcpy(le_addr.a.val, addr->addr, sizeof(addr->addr));

    struct bt_conn_le_cs_capabilities capabilities = {};
    struct bt_conn* conn = bt_conn_lookup_addr_le(0, (const bt_addr_le_t *)&le_addr);

    if (!conn) {
        BT_LOGE("cs write cached remote supported capabilites, doesn't find connection for addr:%s.", 
            bt_fw_addr_str(addr));
    }

    memcpy(&capabilities, params, sizeof(struct bt_conn_le_cs_capabilities));
    SAL_CHECK_RET_WITH_CONN(bt_le_cs_write_cached_remote_supported_capabilities(conn, &capabilities), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
