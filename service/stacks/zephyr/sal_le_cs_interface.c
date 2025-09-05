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


int bt_sal_cs_read_remote_supported_capabilities(bt_controller_id_t id, bt_address_t* addr,
                                                 ble_addr_type_t addr_type)
{
    struct bt_conn* conn;
    int err;

    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_read_remote_supported_capabilities(conn);

    if (err) {
        BT_LOGW("CS read remote supported capabilities fail.");
    }

    return err;

label_on_error:
    return -1;
}

int bt_sal_cs_set_default_settings(bt_controller_id_t id, bt_address_t* addr,
                                                 ble_addr_type_t addr_type,
				  const struct bt_le_cs_set_default_settings_param *params)
{
    struct bt_conn* conn;
    int err;

    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_set_default_settings(conn, params);

    return err;

label_on_error:
    return -1;
}

int bt_sal_cs_read_remote_fae_table(bt_controller_id_t id, bt_address_t* addr,
                                                 ble_addr_type_t addr_type)
{
    struct bt_conn* conn;
    int err;

    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_read_remote_fae_table(conn);

    return err;

label_on_error:
    return -1;
    
}

int bt_sal_cs_create_config(bt_controller_id_t id, bt_address_t* addr,
                            ble_addr_type_t addr_type,
                            struct bt_le_cs_create_config_params *params,
			                enum bt_le_cs_create_config_context context)
{
    struct bt_conn* conn;
    int err;
    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_create_config(conn, params, context);

    return err;

label_on_error:
    return -1;
}

int bt_sal_cs_remove_config(struct bt_conn *conn, uint8_t config_id)
{
    struct bt_conn* conn;
    int err;
    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_remove_config(conn, config_id);

    return err;

label_on_error:
    return -1;
}

int bt_sal_cs_security_enable(bt_controller_id_t id, bt_address_t* addr,
                            ble_addr_type_t addr_type)
{
    struct bt_conn* conn;
    int err;

    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_security_enable();

label_on_error:
    return -1;
}

int bt_sal_cs_procedure_enable(struct bt_conn *conn,
			      const struct bt_le_cs_procedure_enable_param *params)
{
    struct bt_conn* conn;
    int err;

    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_procedure_enable(conn, params);

label_on_error:
    return -1;
}

int bt_sal_cs_remove_config(bt_controller_id_t id, bt_address_t* addr,
                            ble_addr_type_t addr_type, uint8_t config_id)
{
    struct bt_conn* conn;
    int err;
    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_remove_config(conn, config_id);

label_on_error:
    return -1;
}

int bt_sal_cs_set_procedure_parameters(bt_controller_id_t id, bt_address_t* addr,
                            ble_addr_type_t addr_type,
				      const struct bt_le_cs_set_procedure_parameters_param *params)
{
    struct bt_conn* conn;
    int err;
    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_set_procedure_parameters(conn, params);

label_on_error:
    return -1;
}

int bt_sal_cs_set_channel_classification(uint8_t channel_classification[10])
{
    int err = bt_sal_cs_set_channel_classification(channel_classification);
    return err;
}

int bt_sal_cs_read_local_supported_capabilities(struct bt_conn_le_cs_capabilities *ret)
{
    int err = bt_le_cs_read_local_supported_capabilities(ret);
    return err;
}

int bt_sal_cs_write_cached_remote_supported_capabilities(
	struct bt_conn *conn, const struct bt_conn_le_cs_capabilities *params)
{
    struct bt_conn* conn;
    int err;
    CHECK_CONN_FROM_ADDR(addr, conn, label_on_error);

    err = bt_le_cs_write_cached_remote_supported_capabilities(conn, params);
    return err;

label_on_error:
    return -1;
}




