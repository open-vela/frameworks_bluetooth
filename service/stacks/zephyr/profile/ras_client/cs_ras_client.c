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

#include "cs_ras_client.h"

static struct bt_uuid_16 discover_uuid = BT_UUID_INIT_16(0);

sal_le_ras_cli_env_t *ras_cli;

static bool is_ras_cli_discovery_complete(void)
{
	return (atomic_test_bit(&discovery_state, DISC_OTS_FEATURE) &&
		atomic_test_bit(&discovery_state, DISC_OTS_NAME) &&
		atomic_test_bit(&discovery_state, DISC_OTS_TYPE) &&
		atomic_test_bit(&discovery_state, DISC_OTS_SIZE) &&
		atomic_test_bit(&discovery_state, DISC_OTS_ID) &&
		atomic_test_bit(&discovery_state, DISC_OTS_PROPERTIES) &&
		atomic_test_bit(&discovery_state, DISC_OTS_ACTION_CP) &&
		atomic_test_bit(&discovery_state, DISC_OTS_LIST_CP));
}

static void subevent_result_cb(struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result)
{
	if (result->step_data_buf) {
		if (result->step_data_buf->len <= STEP_DATA_BUF_LEN) {
			memcpy(latest_local_steps, result->step_data_buf->data,
			       result->step_data_buf->len);
		} else {
			LOG_ERR("Not enough memory to store step data. (%d > %d)\n",
			       result->step_data_buf->len, STEP_DATA_BUF_LEN);
		}
	}

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE) {
		// TODO:
	}
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange %s (%u)\n", err == 0U ? "success" : "failed", bt_gatt_get_mtu(conn));
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected to %s (err 0x%02X)\n", addr, err);

	__ASSERT(connection == conn, "Unexpected connected callback");

	if (err) {
		bt_conn_unref(conn);
		connection = NULL;
	}

	connection = bt_conn_ref(conn);

	static struct bt_gatt_exchange_params mtu_exchange_params = {.func = mtu_exchange_cb};

	err = bt_gatt_exchange_mtu(connection, &mtu_exchange_params);
	if (err) {
		LOG_INF("%s: MTU exchange failed (err %d)\n", __func__, err);
	}
}

uint8_t ras_cli_real_time_data_notify_cb(struct bt_conn *conn,
				      struct bt_gatt_subscribe_params *params,
				      const void *data, uint16_t length)
{
    if (!conn || !params || params->value_handle != ras_cli->rang_realtime_data_handle) {
		LOG_ERR("Invalid params. conn(%p), params(%p).", conn, params);
		return 0;
	}

	
}



static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02X)\n", reason);

	bt_conn_unref(conn);
	connection = NULL;
}

static void remote_capabilities_cb(struct bt_conn *conn, struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(params);
	LOG_INF("CS capability exchange completed.\n");
}

static void config_created_cb(struct bt_conn *conn, struct bt_conn_le_cs_config *config)
{
	LOG_INF("CS config creation complete. ID: %d\n", config->id);
}

static void security_enabled_cb(struct bt_conn *conn)
{
	LOG_INF("CS security enabled.\n");
}

static void procedure_enabled_cb(struct bt_conn *conn,
				 struct bt_conn_le_cs_procedure_enable_complete *params)
{
	if (params->state == 1) {
		LOG_INF("CS procedures enabled.\n");
	} else {
		LOG_INF("CS procedures disabled.\n");
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	char str[BT_UUID_STR_LEN];

	LOG_INF("Discovery: attr %p\n", attr);

	if (!attr) {
		LOG_INF("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	bt_uuid_to_str(chrc->uuid, str, sizeof(str));
	LOG_INF("UUID %s\n", str);

	if (bt_uuid_cmp(discover_params.uuid, BT_UUID_RANGING) == 0) {
		(void)memcpy(&discover_uuid, BT_UUID_RANG_RT_DT, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		err = bt_gatt_discover(conn, &discover_params);

		if (err != 0) {
			LOG_ERR("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_RANG_RT_DT) == 0 ) {
		atomic_set_bit(&discovery_state, BT_UUID_RANG_RT_DT);
		otc.feature_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_RANG_ON_DEM_DT, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			LOG_ERR("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_RANG_ON_DEM_DT) == 0) {
		atomic_set_bit(&discovery_state, BT_UUID_RANG_ON_DEM_DT);
		otc.feature_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_RANG_RAS_CTR_POINT, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			LOG_ERR("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_RANG_RAS_CTR_POINT) == 0) {
		atomic_set_bit(&discovery_state, BT_UUID_RANG_RAS_CTR_POINT);
		otc.obj_name_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_RANG_DT_RD, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			LOG_ERR("Discover failed (err %d)\n", err);
		}

	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_RANG_DT_RD) == 0) {
		atomic_set_bit(&discovery_state, BT_UUID_RANG_DT_RD);
		otc.obj_type_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_RANG_DT_OV_WR, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			LOG_ERR("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_RANG_DT_OV_WR) == 0) {
		atomic_set_bit(&discovery_state, BT_UUID_RANG_DT_OV_WR);
		otc.obj_size_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_ID, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			LOG_ERR("Discover failed (err %d)\n", err);
		}

	} else {
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void write_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err) {
		LOG_ERR("Write failed (err %d)\n", err);
		return;
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.le_cs_remote_capabilities_available = remote_capabilities_cb,
	.le_cs_config_created = config_created_cb,
	.le_cs_security_enabled = security_enabled_cb,
	.le_cs_procedure_enabled = procedure_enabled_cb,
	.le_cs_subevent_data_available = subevent_result_cb,
};

void ras_client_init(void)
{
	ras_cli = (sal_le_ras_cli_env_t *)malloc(sizeof(sal_le_ras_cli_env_t));
	__ASSERT(ras_cli != NULL);
	return;
}

