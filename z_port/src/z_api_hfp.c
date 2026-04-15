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

#include <debug.h>
#include <errno.h>
#include <nuttx/config.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "bluetooth.h"
#include "bt_device.h"
#include "bt_hfp.h"
#include "bt_hfp_hf.h"
#include "utils/log.h"
#include "z_api.h"
#include "z_api_manager.h"

typedef int (*z_api_func_t)(void* arg);
extern int z_api_dispatch(z_api_func_t func, void* arg);
extern bt_instance_t* local_bt_ins;
extern int z_bt_conn_create(const void* peer, void** ret_conn);

static bt_instance_t* get_ins(void)
{
    if (local_bt_ins)
        return local_bt_ins;
    return (bt_instance_t*)z_api(bt_svc_ins_get)();
}

/* ---- Address conversion helpers ---- */

static void zephyr_addr_to_fw(const uint8_t* z_addr, bt_address_t* fw)
{
    memcpy(fw->addr, &z_addr[1], 6);
}

static void fw_addr_to_zephyr(const bt_address_t* fw, uint8_t addr_type,
    uint8_t* z_addr)
{
    z_addr[0] = addr_type;
    memcpy(&z_addr[1], fw->addr, 6);
}

/* ---- HFP state management ---- */

static void* hfp_handle;
static bool hfp_initialized;

/* BTP HFP callback declarations */
extern void btp_hfp_connected_cb(const uint8_t* addr);
extern void btp_hfp_disconnected_cb(const uint8_t* addr);
extern void btp_hfp_audio_state_cb(const uint8_t* addr, uint8_t state);

/* ---- HFP Framework callbacks ---- */

static void hfp_connection_state_cb(void* cookie, bt_address_t* addr,
    profile_connection_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp] connection_state_cb: state=%d\n", state);

    fw_addr_to_zephyr(addr, 0x00, z_addr);

    if (state == PROFILE_STATE_CONNECTED) {
        btp_hfp_connected_cb(z_addr);
    } else if (state == PROFILE_STATE_DISCONNECTED) {
        btp_hfp_disconnected_cb(z_addr);
    }
}

static void hfp_audio_state_cb(void* cookie, bt_address_t* addr,
    hfp_audio_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp] audio_state_cb: state=%d\n", state);

    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_audio_state_cb(z_addr, (uint8_t)state);
}

static const hfp_hf_callbacks_t hfp_cbs = {
    .size = sizeof(hfp_hf_callbacks_t),
    .connection_state_cb = hfp_connection_state_cb,
    .audio_state_cb = hfp_audio_state_cb,
};

/* ---- IPC dispatch functions ---- */

static int hfp_init_in_ipc(void* arg)
{
    (void)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    if (hfp_initialized)
        return 0;

    hfp_handle = bt_hfp_hf_register_callbacks(ins, &hfp_cbs);
    if (!hfp_handle) {
        _info("[z_api_hfp] register_callbacks failed\n");
        return -EIO;
    }

    hfp_initialized = true;
    _info("[z_api_hfp] initialized\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
} hfp_addr_args_t;

static int hfp_connect_in_ipc(void* arg)
{
    hfp_addr_args_t* a = (hfp_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    if (!hfp_initialized) {
        hfp_init_in_ipc(NULL);
    }

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_connect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] connect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] connect initiated\n");
    return 0;
}

static int hfp_disconnect_in_ipc(void* arg)
{
    hfp_addr_args_t* a = (hfp_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_disconnect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] disconnect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] disconnect initiated\n");
    return 0;
}

static int hfp_answer_in_ipc(void* arg)
{
    hfp_addr_args_t* a = (hfp_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_accept_call(ins, &fw_addr, HFP_HF_CALL_ACCEPT_NONE);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] answer failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] answer call\n");
    return 0;
}

static int hfp_reject_in_ipc(void* arg)
{
    hfp_addr_args_t* a = (hfp_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_reject_call(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] reject failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] reject call\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    char number[33];
} hfp_dial_args_t;

static int hfp_dial_in_ipc(void* arg)
{
    hfp_dial_args_t* a = (hfp_dial_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_dial(ins, &fw_addr, a->number);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] dial failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] dial: %s\n", a->number);
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t type;
    uint8_t volume;
} hfp_volume_args_t;

static int hfp_set_volume_in_ipc(void* arg)
{
    hfp_volume_args_t* a = (hfp_volume_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_volume_control(ins, &fw_addr,
        (hfp_volume_type_t)a->type, a->volume);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] set_volume failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] set volume: type=%d vol=%d\n", a->type, a->volume);
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t code;
} hfp_dtmf_args_t;

static int hfp_send_dtmf_in_ipc(void* arg)
{
    hfp_dtmf_args_t* a = (hfp_dtmf_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_send_dtmf(ins, &fw_addr, (char)a->code);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] send_dtmf failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] send DTMF: %c\n", a->code);
    return 0;
}

/* ---- Public z_api HFP functions ---- */

int z_bt_hfp_init(void)
{
    _info("[z_api] >>> z_bt_hfp_init\n");
    return z_api_dispatch(hfp_init_in_ipc, NULL);
}

int z_bt_hfp_connect(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_connect\n");
    if (!addr)
        return -EINVAL;

    hfp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hfp_connect_in_ipc, &args);
}

int z_bt_hfp_disconnect(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_disconnect\n");
    if (!addr)
        return -EINVAL;

    hfp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hfp_disconnect_in_ipc, &args);
}

int z_bt_hfp_answer(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_answer\n");
    if (!addr)
        return -EINVAL;

    hfp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hfp_answer_in_ipc, &args);
}

int z_bt_hfp_reject(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_reject\n");
    if (!addr)
        return -EINVAL;

    hfp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hfp_reject_in_ipc, &args);
}

int z_bt_hfp_dial(const uint8_t* addr, const char* number)
{
    _info("[z_api] >>> z_bt_hfp_dial\n");
    if (!addr || !number)
        return -EINVAL;

    hfp_dial_args_t args;
    memcpy(args.z_addr, addr, 7);
    strncpy(args.number, number, 32);
    args.number[32] = '\0';
    return z_api_dispatch(hfp_dial_in_ipc, &args);
}

int z_bt_hfp_set_volume(const uint8_t* addr, uint8_t type, uint8_t volume)
{
    _info("[z_api] >>> z_bt_hfp_set_volume\n");
    if (!addr)
        return -EINVAL;

    hfp_volume_args_t args;
    memcpy(args.z_addr, addr, 7);
    args.type = type;
    args.volume = volume;
    return z_api_dispatch(hfp_set_volume_in_ipc, &args);
}

int z_bt_hfp_send_dtmf(const uint8_t* addr, uint8_t code)
{
    _info("[z_api] >>> z_bt_hfp_send_dtmf\n");
    if (!addr)
        return -EINVAL;

    hfp_dtmf_args_t args;
    memcpy(args.z_addr, addr, 7);
    args.code = code;
    return z_api_dispatch(hfp_send_dtmf_in_ipc, &args);
}

int z_bt_hfp_connect_acl(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_connect_acl (ACL only)\n");
    if (!addr)
        return -EINVAL;

    if (!hfp_initialized) {
        z_bt_hfp_init();
    }

    /* Use z_bt_conn_create to establish ACL connection only,
     * without initiating RFCOMM/SLC. PTS will initiate RFCOMM. */
    return z_bt_conn_create(addr, NULL);
}

static int hfp_connect_audio_in_ipc(void* arg)
{
    hfp_addr_args_t* a = (hfp_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_connect_audio(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] connect_audio failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] connect_audio initiated\n");
    return 0;
}

static int hfp_disconnect_audio_in_ipc(void* arg)
{
    hfp_addr_args_t* a = (hfp_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_hf_disconnect_audio(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp] disconnect_audio failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp] disconnect_audio initiated\n");
    return 0;
}

int z_bt_hfp_connect_audio(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_connect_audio\n");
    if (!addr)
        return -EINVAL;

    hfp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hfp_connect_audio_in_ipc, &args);
}

int z_bt_hfp_disconnect_audio(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_disconnect_audio\n");
    if (!addr)
        return -EINVAL;

    hfp_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hfp_disconnect_audio_in_ipc, &args);
}
