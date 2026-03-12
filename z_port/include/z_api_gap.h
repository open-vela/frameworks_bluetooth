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

#ifndef __Z_API_GAP_H__
#define __Z_API_GAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "z_api.h"

/*
 * This header is for z_port internal use only.
 * The actual Zephyr types are included in the .c files which are compiled
 * as part of the zblue build environment.
 *
 * For autopts tester, use z_api_port.h which provides the extern declarations.
 */

/* Forward declarations - actual types come from zephyr headers in .c files */
struct bt_le_adv_param;
struct bt_data;
struct bt_le_scan_param;
struct bt_conn;
struct bt_conn_le_create_param;
struct bt_le_conn_param;
struct bt_conn_info;
struct bt_conn_cb;
struct bt_conn_auth_cb;
struct bt_conn_auth_info_cb;
struct bt_le_oob;
struct bt_le_oob_sc_data;

typedef void (*bt_ready_cb_t)(int err);
typedef void (*bt_le_scan_cb_t)(const void *addr, int8_t rssi,
                                uint8_t adv_type, const void *buf);

/* GAP initialization/deinitialization */
int z_api(bt_gap_init)(void);
int z_api(bt_gap_deinit)(void);

/* Bluetooth enable/disable */
int z_api(bt_enable)(bt_ready_cb_t cb);
int z_api(bt_disable)(void);

/* Advertising APIs */
int z_api(bt_le_adv_start)(const struct bt_le_adv_param *param,
                           const struct bt_data *ad, size_t ad_len,
                           const struct bt_data *sd, size_t sd_len);
int z_api(bt_le_adv_stop)(void);

/* Scanning APIs */
int z_api(bt_le_scan_start)(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb);
int z_api(bt_le_scan_stop)(void);

/* Connection APIs */
int z_api(bt_conn_le_create)(const void *peer,
                             const struct bt_conn_le_create_param *create_param,
                             const struct bt_le_conn_param *conn_param,
                             struct bt_conn **ret_conn);
int z_api(bt_conn_le_create_auto)(const struct bt_conn_le_create_param *create_param,
                                  const struct bt_le_conn_param *conn_param);
int z_api(bt_conn_disconnect)(struct bt_conn *conn, uint8_t reason);
struct bt_conn *z_api(bt_conn_lookup_addr_le)(uint8_t id, const void *peer);
void z_api(bt_conn_unref)(struct bt_conn *conn);
struct bt_conn *z_api(bt_conn_ref)(struct bt_conn *conn);

/* Connection info APIs */
int z_api(bt_conn_get_info)(struct bt_conn *conn, struct bt_conn_info *info);
const void *z_api(bt_conn_get_dst)(const struct bt_conn *conn);
int z_api(bt_conn_get_security)(struct bt_conn *conn);

/* Connection callback APIs */
void z_api(bt_conn_cb_register)(struct bt_conn_cb *cb);
void z_api(bt_conn_cb_unregister)(struct bt_conn_cb *cb);

/* Connection parameter update */
int z_api(bt_conn_le_param_update)(struct bt_conn *conn,
                                   const struct bt_le_conn_param *param);

/* Security APIs */
int z_api(bt_conn_set_security)(struct bt_conn *conn, int sec);
uint8_t z_api(bt_conn_enc_key_size)(struct bt_conn *conn);

/* Authentication callback APIs */
int z_api(bt_conn_auth_cb_register)(const struct bt_conn_auth_cb *cb);
int z_api(bt_conn_auth_info_cb_register)(struct bt_conn_auth_info_cb *cb);
int z_api(bt_conn_auth_passkey_entry)(struct bt_conn *conn, unsigned int passkey);
int z_api(bt_conn_auth_passkey_confirm)(struct bt_conn *conn);
int z_api(bt_conn_auth_cancel)(struct bt_conn *conn);

/* Bonding APIs */
int z_api(bt_set_bondable)(bool enable);
int z_api(bt_unpair)(uint8_t id, const void *addr);
bool z_api(bt_addr_le_is_bonded)(uint8_t id, const void *addr);

/* OOB APIs */
int z_api(bt_le_oob_get_local)(uint8_t id, struct bt_le_oob *oob);
int z_api(bt_le_oob_set_sc_data)(struct bt_conn *conn,
                                 const struct bt_le_oob_sc_data *oobd_local,
                                 const struct bt_le_oob_sc_data *oobd_remote);
int z_api(bt_le_oob_set_legacy_tk)(struct bt_conn *conn, const uint8_t *tk);
int z_api(bt_le_oob_set_sc_flag)(bool enable);
int z_api(bt_le_oob_set_legacy_flag)(bool enable);

/* Filter accept list APIs */
int z_api(bt_le_filter_accept_list_clear)(void);
int z_api(bt_le_filter_accept_list_add)(const void *addr);

#ifdef __cplusplus
}
#endif

#endif /* __Z_API_GAP_H__ */
