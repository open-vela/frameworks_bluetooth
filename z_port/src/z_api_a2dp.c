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
#include "bt_a2dp.h"
#include "bt_a2dp_sink.h"
#include "bt_a2dp_source.h"
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

/* ---- A2DP state management ---- */

static void *a2dp_sink_handle;
static bool a2dp_initialized;

/* BTP A2DP callback declarations (defined in btp_a2dp.c) */
extern void btp_a2dp_connected_cb(const uint8_t *addr);
extern void btp_a2dp_disconnected_cb(const uint8_t *addr);
extern void btp_a2dp_audio_state_cb(const uint8_t *addr, uint8_t state);

/* ---- A2DP Framework callbacks ---- */

static void a2dp_connection_state_cb(void *cookie, bt_address_t *addr,
    profile_connection_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_a2dp] connection_state_cb: state=%d\n", state);

    fw_addr_to_zephyr(addr, 0x00, z_addr);

    if (state == PROFILE_STATE_CONNECTED) {
        btp_a2dp_connected_cb(z_addr);
    } else if (state == PROFILE_STATE_DISCONNECTED) {
        btp_a2dp_disconnected_cb(z_addr);
    }
}

static void a2dp_audio_state_cb(void *cookie, bt_address_t *addr,
    a2dp_audio_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_a2dp] audio_state_cb: state=%d\n", state);

    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_a2dp_audio_state_cb(z_addr, (uint8_t)state);
}

static const a2dp_sink_callbacks_t a2dp_sink_cbs = {
    .size = sizeof(a2dp_sink_callbacks_t),
    .connection_state_cb = a2dp_connection_state_cb,
    .audio_state_cb = a2dp_audio_state_cb,
};

/* ---- IPC dispatch functions ---- */

static int a2dp_init_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (a2dp_initialized) return 0;

    a2dp_sink_handle = bt_a2dp_sink_register_callbacks(ins, &a2dp_sink_cbs);
    if (!a2dp_sink_handle) {
        _info("[z_api_a2dp] register_callbacks failed\n");
        return -EIO;
    }

    a2dp_initialized = true;
    _info("[z_api_a2dp] initialized\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
} a2dp_addr_args_t;

static int a2dp_connect_in_ipc(void *arg)
{
    a2dp_addr_args_t *a = (a2dp_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (!a2dp_initialized) {
        a2dp_init_in_ipc(NULL);
    }

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_a2dp_sink_connect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_a2dp] connect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_a2dp] connect initiated\n");
    return 0;
}

static int a2dp_disconnect_in_ipc(void *arg)
{
    a2dp_addr_args_t *a = (a2dp_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_a2dp_sink_disconnect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_a2dp] disconnect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_a2dp] disconnect initiated\n");
    return 0;
}

static int a2dp_start_in_ipc(void *arg)
{
    (void)arg;
    /* bt_a2dp_source_start_stream does not exist in the Framework;
     * streaming is managed internally by the A2DP service.
     * Stub: log and return success. */
    _info("[z_api_a2dp] start stream: no explicit API, stub OK\n");
    return 0;
}

static int a2dp_stop_in_ipc(void *arg)
{
    (void)arg;
    /* bt_a2dp_source_stop_stream does not exist in the Framework;
     * streaming is managed internally by the A2DP service.
     * Stub: log and return success. */
    _info("[z_api_a2dp] stop stream: no explicit API, stub OK\n");
    return 0;
}

/* ---- Public z_api A2DP functions ---- */

int z_bt_a2dp_connect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_a2dp_connect\n");
    if (!addr) return -EINVAL;

    a2dp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(a2dp_connect_in_ipc, &args);
}

int z_bt_a2dp_disconnect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_a2dp_disconnect\n");
    if (!addr) return -EINVAL;

    a2dp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(a2dp_disconnect_in_ipc, &args);
}

int z_bt_a2dp_start(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_a2dp_start\n");
    if (!addr) return -EINVAL;

    a2dp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(a2dp_start_in_ipc, &args);
}

int z_bt_a2dp_stop(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_a2dp_stop\n");
    if (!addr) return -EINVAL;

    a2dp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(a2dp_stop_in_ipc, &args);
}
