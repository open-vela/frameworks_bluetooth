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
#include "bt_pan.h"
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

/* ---- PAN state management ---- */

static void *pan_handle;
static bool pan_initialized;
static uint8_t pan_local_role = 2; /* PAN_ROLE_PANU */

/* BTP PAN callback declarations */
extern void btp_pan_connected_cb(const uint8_t *addr);
extern void btp_pan_disconnected_cb(const uint8_t *addr);

/* ---- PAN Framework callbacks ---- */

static void pan_connection_state_cb(void *cookie,
    profile_connection_state_t state, bt_address_t *addr,
    uint8_t local_role, uint8_t remote_role)
{
    uint8_t z_addr[7];

    _info("[z_api_pan] connection_state_cb: state=%d\n", state);

    fw_addr_to_zephyr(addr, 0x00, z_addr);

    if (state == PROFILE_STATE_CONNECTED) {
        btp_pan_connected_cb(z_addr);
    } else if (state == PROFILE_STATE_DISCONNECTED) {
        btp_pan_disconnected_cb(z_addr);
    }
}

static const pan_callbacks_t pan_cbs = {
    .size = sizeof(pan_callbacks_t),
    .connection_state_cb = pan_connection_state_cb,
};

/* ---- IPC dispatch functions ---- */

static int pan_init_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (pan_initialized) return 0;

    /* PAN service may not be compiled into the build.
     * Try to register callbacks; if the symbol is not available at link time,
     * we use weak stubs below. */
    pan_handle = bt_pan_register_callbacks(ins, &pan_cbs);
    if (!pan_handle) {
        _info("[z_api_pan] register_callbacks failed\n");
        return -EIO;
    }

    pan_initialized = true;
    _info("[z_api_pan] initialized\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
} pan_addr_args_t;

static int pan_connect_in_ipc(void *arg)
{
    pan_addr_args_t *a = (pan_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (!pan_initialized) {
        pan_init_in_ipc(NULL);
    }

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    /* PANU connects to NAP */
    bt_status_t ret = bt_pan_connect(ins, &fw_addr, PAN_ROLE_NAP, PAN_ROLE_PANU);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_pan] connect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_pan] connect initiated\n");
    return 0;
}

static int pan_disconnect_in_ipc(void *arg)
{
    pan_addr_args_t *a = (pan_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_pan_disconnect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_pan] disconnect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_pan] disconnect initiated\n");
    return 0;
}

/* ---- Public z_api PAN functions ---- */

int z_bt_pan_connect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_pan_connect\n");
    if (!addr) return -EINVAL;

    pan_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(pan_connect_in_ipc, &args);
}

int z_bt_pan_disconnect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_pan_disconnect\n");
    if (!addr) return -EINVAL;

    pan_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(pan_disconnect_in_ipc, &args);
}

int z_bt_pan_set_role(uint8_t role)
{
    _info("[z_api] >>> z_bt_pan_set_role: role=%d\n", role);
    pan_local_role = role;
    return 0;
}

/* ---- Weak stubs for PAN Framework APIs ----
 * The PAN service implementation may not be compiled into the build.
 * Provide weak default implementations so the linker doesn't fail.
 */

__attribute__((weak))
void *bt_pan_register_callbacks(bt_instance_t *ins, const pan_callbacks_t *callbacks)
{
    (void)ins;
    (void)callbacks;
    _info("[z_api_pan] WEAK bt_pan_register_callbacks (PAN service not available)\n");
    return NULL;
}

__attribute__((weak))
bt_status_t bt_pan_connect(bt_instance_t *ins, bt_address_t *addr,
                           uint8_t dst_role, uint8_t src_role)
{
    (void)ins;
    (void)addr;
    (void)dst_role;
    (void)src_role;
    _info("[z_api_pan] WEAK bt_pan_connect (PAN service not available)\n");
    return BT_STATUS_NOT_SUPPORTED;
}

__attribute__((weak))
bt_status_t bt_pan_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    (void)ins;
    (void)addr;
    _info("[z_api_pan] WEAK bt_pan_disconnect (PAN service not available)\n");
    return BT_STATUS_NOT_SUPPORTED;
}
