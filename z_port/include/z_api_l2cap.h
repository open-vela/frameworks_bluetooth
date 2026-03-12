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

#ifndef __Z_API_L2CAP_H__
#define __Z_API_L2CAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "z_api.h"

/* Forward declarations */
struct bt_l2cap_server;
struct bt_l2cap_chan;
struct bt_conn;
struct net_buf;

/* L2CAP Server APIs */
int z_api(bt_l2cap_server_register)(struct bt_l2cap_server *server);

/* L2CAP Channel APIs */
int z_api(bt_l2cap_chan_connect)(struct bt_conn *conn,
                                 struct bt_l2cap_chan *chan, uint16_t psm);
int z_api(bt_l2cap_chan_disconnect)(struct bt_l2cap_chan *chan);
int z_api(bt_l2cap_chan_send)(struct bt_l2cap_chan *chan, struct net_buf *buf);
int z_api(bt_l2cap_chan_recv_complete)(struct bt_l2cap_chan *chan,
                                       struct net_buf *buf);

/* L2CAP ECRED APIs */
int z_api(bt_l2cap_ecred_chan_connect)(struct bt_conn *conn,
                                       struct bt_l2cap_chan **chans,
                                       uint16_t psm);
int z_api(bt_l2cap_ecred_chan_reconfigure)(struct bt_l2cap_chan **chans,
                                           uint16_t mtu);

/* EATT APIs */
int z_api(bt_eatt_connect)(struct bt_conn *conn, size_t num_channels);
int z_api(bt_eatt_disconnect_one)(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

#endif /* __Z_API_L2CAP_H__ */
