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

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <debug.h>
#include <syslog.h>

#include "z_api.h"
#include "z_api_manager.h"
#include "bt_l2cap.h"
#include "utils/log.h"

/* L2CAP Server APIs - stub implementations */
int z_api(bt_l2cap_server_register)(void *server)
{
    _info("[z_api] >>> z_bt_l2cap_server_register: ENTRY");
    BT_LOGI("bt_l2cap_server_register called");
    _info("[z_api] >>> z_bt_l2cap_server_register: ret=%d", (int)(0));
    return 0;
}

int z_api(bt_l2cap_chan_connect)(void *conn, void *chan, uint16_t psm)
{
    _info("[z_api] >>> z_bt_l2cap_chan_connect: ENTRY");
    BT_LOGI("bt_l2cap_chan_connect called: psm=%d", psm);
    _info("[z_api] >>> z_bt_l2cap_chan_connect: ret=%d", (int)(-ENOTSUP));
    return -ENOTSUP;
}

int z_api(bt_l2cap_chan_disconnect)(void *chan)
{
    _info("[z_api] >>> z_bt_l2cap_chan_disconnect: ENTRY");
    BT_LOGI("bt_l2cap_chan_disconnect called");
    _info("[z_api] >>> z_bt_l2cap_chan_disconnect: ret=%d", (int)(-ENOTSUP));
    return -ENOTSUP;
}

int z_api(bt_l2cap_chan_send)(void *chan, void *buf)
{
    _info("[z_api] >>> z_bt_l2cap_chan_send: ENTRY");
    BT_LOGD("bt_l2cap_chan_send called");
    _info("[z_api] >>> z_bt_l2cap_chan_send: ret=%d", (int)(-ENOTSUP));
    return -ENOTSUP;
}

int z_api(bt_l2cap_chan_recv_complete)(void *chan, void *buf)
{
    _info("[z_api] >>> z_bt_l2cap_chan_recv_complete: ENTRY");
    return 0;
}

int z_api(bt_l2cap_ecred_chan_connect)(void *conn, void **chans, uint16_t psm)
{
    _info("[z_api] >>> z_bt_l2cap_ecred_chan_connect: ENTRY");
    BT_LOGI("bt_l2cap_ecred_chan_connect called: psm=%d", psm);
    _info("[z_api] >>> z_bt_l2cap_ecred_chan_connect: ret=%d", (int)(-ENOTSUP));
    return -ENOTSUP;
}

int z_api(bt_l2cap_ecred_chan_reconfigure)(void **chans, uint16_t mtu)
{
    _info("[z_api] >>> z_bt_l2cap_ecred_chan_reconfigure: ENTRY");
    BT_LOGW("bt_l2cap_ecred_chan_reconfigure: mtu=%d (not supported)", mtu);
    _info("[z_api] >>> z_bt_l2cap_ecred_chan_reconfigure: ret=%d", (int)(0));
    return 0;
}

int z_api(bt_eatt_connect)(void *conn, size_t num_channels)
{
    _info("[z_api] >>> z_bt_eatt_connect: ENTRY");
    BT_LOGW("bt_eatt_connect: num_channels=%zu (not supported)", num_channels);
    _info("[z_api] >>> z_bt_eatt_connect: ret=%d", (int)(0));
    return 0;
}

int z_api(bt_eatt_disconnect_one)(void *conn)
{
    _info("[z_api] >>> z_bt_eatt_disconnect_one: ENTRY");
    BT_LOGW("bt_eatt_disconnect_one (not supported)");
    _info("[z_api] >>> z_bt_eatt_disconnect_one: ret=%d", (int)(0));
    return 0;
}
