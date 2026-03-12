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
#include "bt_avrcp.h"
#include "bt_avrcp_control.h"
#include "bt_device.h"
#include "bluetooth.h"
#include "utils/log.h"

typedef int (*z_api_func_t)(void *arg);
extern int z_api_dispatch(z_api_func_t func, void *arg);
extern bt_instance_t *local_bt_ins;

static bt_instance_t *get_ins(void)
{
    if (local_bt_ins) return local_bt_ins;
    return (bt_instance_t *)z_api(bt_svc_ins_get)();
}

/* ---- Address conversion helpers ---- */

static void zephyr_addr_to_fw(const uint8_t *z_addr, bt_address_t *fw)
{
    memcpy(fw->addr, &z_addr[1], 6);
}

static void fw_addr_to_zephyr(const bt_address_t *fw, uint8_t addr_type,
                               uint8_t *z_addr)
{
    z_addr[0] = addr_type;
    memcpy(&z_addr[1], fw->addr, 6);
}

/* ---- AVRCP state management ---- */

static void *avrcp_handle;
static bool avrcp_initialized;

/* BTP AVRCP callback declarations */
extern void btp_avrcp_connected_cb(const uint8_t *addr);
extern void btp_avrcp_disconnected_cb(const uint8_t *addr);

/* ---- AVRCP Framework callbacks ---- */

static void avrcp_connection_state_cb(void *cookie, bt_address_t *addr,
    profile_connection_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_avrcp] connection_state_cb: state=%d\n", state);

    fw_addr_to_zephyr(addr, 0x00, z_addr);

    if (state == PROFILE_STATE_CONNECTED) {
        btp_avrcp_connected_cb(z_addr);
    } else if (state == PROFILE_STATE_DISCONNECTED) {
        btp_avrcp_disconnected_cb(z_addr);
    }
}

static const avrcp_control_callbacks_t avrcp_cbs = {
    .size = sizeof(avrcp_control_callbacks_t),
    .connection_state_cb = avrcp_connection_state_cb,
};

/* ---- IPC dispatch functions ---- */

static int avrcp_init_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (avrcp_initialized) return 0;

    avrcp_handle = bt_avrcp_control_register_callbacks(ins, &avrcp_cbs);
    if (!avrcp_handle) {
        _info("[z_api_avrcp] register_callbacks failed\n");
        return -EIO;
    }

    avrcp_initialized = true;
    _info("[z_api_avrcp] initialized\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
} avrcp_addr_args_t;

static int avrcp_connect_in_ipc(void *arg)
{
    avrcp_addr_args_t *a = (avrcp_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (!avrcp_initialized) {
        avrcp_init_in_ipc(NULL);
    }

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    /* AVRCP control has no explicit connect API in the Framework;
     * the connection is established implicitly via A2DP or other profiles.
     * Log and return success as a stub. */
    (void)ins;
    (void)fw_addr;
    _info("[z_api_avrcp] connect: no explicit API, stub OK\n");
    return 0;
}

static int avrcp_disconnect_in_ipc(void *arg)
{
    avrcp_addr_args_t *a = (avrcp_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    /* AVRCP control has no explicit disconnect API in the Framework;
     * the disconnection is handled implicitly via A2DP or other profiles.
     * Log and return success as a stub. */
    (void)ins;
    (void)fw_addr;
    _info("[z_api_avrcp] disconnect: no explicit API, stub OK\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t key_id;
    uint8_t key_state;
} avrcp_passthrough_args_t;

static int avrcp_passthrough_in_ipc(void *arg)
{
    avrcp_passthrough_args_t *a = (avrcp_passthrough_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_avrcp_control_send_passthrough_cmd(ins, &fw_addr,
        a->key_id, a->key_state);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_avrcp] passthrough failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_avrcp] passthrough sent\n");
    return 0;
}

static int avrcp_get_element_attrs_in_ipc(void *arg)
{
    avrcp_addr_args_t *a = (avrcp_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_avrcp_control_get_element_attributes(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_avrcp] get_element_attrs failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_avrcp] get_element_attrs requested\n");
    return 0;
}

/* ---- Public z_api AVRCP functions ---- */

int z_bt_avrcp_connect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_avrcp_connect\n");
    if (!addr) return -EINVAL;

    avrcp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(avrcp_connect_in_ipc, &args);
}

int z_bt_avrcp_disconnect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_avrcp_disconnect\n");
    if (!addr) return -EINVAL;

    avrcp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(avrcp_disconnect_in_ipc, &args);
}

int z_bt_avrcp_passthrough(const uint8_t *addr,
                           uint8_t key_id, uint8_t key_state)
{
    _info("[z_api] >>> z_bt_avrcp_passthrough: key=%d state=%d\n",
          key_id, key_state);
    if (!addr) return -EINVAL;

    avrcp_passthrough_args_t args;
    memcpy(args.z_addr, addr, 7);
    args.key_id = key_id;
    args.key_state = key_state;
    return z_api_dispatch(avrcp_passthrough_in_ipc, &args);
}

int z_bt_avrcp_get_element_attrs(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_avrcp_get_element_attrs\n");
    if (!addr) return -EINVAL;

    avrcp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(avrcp_get_element_attrs_in_ipc, &args);
}
