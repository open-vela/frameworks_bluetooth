/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#ifndef __Z_API_GATT_H__
#define __Z_API_GATT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "z_api.h"

/* Forward declarations */
struct bt_gatt_service;
struct bt_gatt_attr;
struct bt_gatt_notify_params;
struct bt_gatt_indicate_params;
struct bt_gatt_discover_params;
struct bt_gatt_read_params;
struct bt_gatt_write_params;
struct bt_gatt_subscribe_params;
struct bt_gatt_exchange_params;
struct bt_conn;

typedef uint8_t (*bt_gatt_attr_func_t)(const struct bt_gatt_attr *attr,
                                       uint16_t handle, void *user_data);

/* GATT Server APIs */
int z_api(bt_gatt_service_register)(struct bt_gatt_service *svc);
int z_api(bt_gatt_service_unregister)(struct bt_gatt_service *svc);

/* GATT Notification/Indication APIs */
int z_api(bt_gatt_notify)(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          const void *data, uint16_t len);
int z_api(bt_gatt_notify_cb)(struct bt_conn *conn, struct bt_gatt_notify_params *params);
int z_api(bt_gatt_indicate)(struct bt_conn *conn, struct bt_gatt_indicate_params *params);

/* GATT Client APIs */
int z_api(bt_gatt_discover)(struct bt_conn *conn, struct bt_gatt_discover_params *params);
int z_api(bt_gatt_read)(struct bt_conn *conn, struct bt_gatt_read_params *params);
int z_api(bt_gatt_write)(struct bt_conn *conn, struct bt_gatt_write_params *params);
int z_api(bt_gatt_write_without_response)(struct bt_conn *conn, uint16_t handle,
                                          const void *data, uint16_t length, bool sign);
int z_api(bt_gatt_subscribe)(struct bt_conn *conn, struct bt_gatt_subscribe_params *params);
int z_api(bt_gatt_unsubscribe)(struct bt_conn *conn, struct bt_gatt_subscribe_params *params);

/* GATT MTU Exchange */
int z_api(bt_gatt_exchange_mtu)(struct bt_conn *conn, struct bt_gatt_exchange_params *params);

/* GATT Attribute Read Helper */
ssize_t z_api(bt_gatt_attr_read)(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t buf_len, uint16_t offset,
                                 const void *value, uint16_t value_len);

/* GATT Foreach Attribute */
void z_api(bt_gatt_foreach_attr)(uint16_t start_handle, uint16_t end_handle,
                                 bt_gatt_attr_func_t func, void *user_data);

/* GATT Notify Multiple */
int z_api(bt_gatt_notify_multiple)(struct bt_conn *conn, uint16_t num_params,
                                   struct bt_gatt_notify_params params[]);

#ifdef __cplusplus
}
#endif

#endif /* __Z_API_GATT_H__ */
