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
#include "bt_hfp_ag.h"
#include "utils/log.h"
#include "z_api.h"
#include "z_api_manager.h"

typedef int (*z_api_func_t)(void* arg);
extern int z_api_dispatch(z_api_func_t func, void* arg);
extern bt_instance_t* local_bt_ins;

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

/* ---- HFP AG state management ---- */

static void* ag_handle;
static bool ag_initialized;

/* BTP HFP AG callback declarations */
extern void btp_hfp_ag_connected_cb(const uint8_t* addr);
extern void btp_hfp_ag_disconnected_cb(const uint8_t* addr);
extern void btp_hfp_ag_audio_state_cb(const uint8_t* addr, uint8_t state);
extern void btp_hfp_ag_vr_state_cb(const uint8_t* addr, bool started);
extern void btp_hfp_ag_battery_update_cb(const uint8_t* addr, uint8_t level);
extern void btp_hfp_ag_volume_cb(const uint8_t* addr, uint8_t type, uint8_t vol);
extern void btp_hfp_ag_answer_call_cb(const uint8_t* addr);
extern void btp_hfp_ag_reject_call_cb(const uint8_t* addr);
extern void btp_hfp_ag_hangup_call_cb(const uint8_t* addr);
extern void btp_hfp_ag_dial_cb(const uint8_t* addr, const char* number);
extern void btp_hfp_ag_at_cmd_cb(const uint8_t* addr, const char* cmd);
extern void btp_hfp_ag_clcc_request_cb(const uint8_t* addr);
extern void btp_hfp_ag_cind_request_cb(const uint8_t* addr);

/* ---- HFP AG Framework callbacks ---- */

static void ag_connection_state_cb(void* cookie, bt_address_t* addr,
    profile_connection_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] connection_state_cb: state=%d\n", state);
    fw_addr_to_zephyr(addr, 0x00, z_addr);

    if (state == PROFILE_STATE_CONNECTED) {
        btp_hfp_ag_connected_cb(z_addr);
    } else if (state == PROFILE_STATE_DISCONNECTED) {
        btp_hfp_ag_disconnected_cb(z_addr);
    }
}

static void ag_audio_state_cb(void* cookie, bt_address_t* addr,
    hfp_audio_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] audio_state_cb: state=%d\n", state);
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_audio_state_cb(z_addr, (uint8_t)state);
}

static void ag_vr_cmd_cb(void* cookie, bt_address_t* addr, bool started)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] vr_cmd_cb: started=%d\n", started);
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_vr_state_cb(z_addr, started);
}

static void ag_battery_update_cb(void* cookie, bt_address_t* addr,
    uint8_t value)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] battery_update_cb: value=%d\n", value);
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_battery_update_cb(z_addr, value);
}

static void ag_volume_control_cb(void* cookie, bt_address_t* addr,
    hfp_volume_type_t type, uint8_t volume)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] volume_control_cb: type=%d vol=%d\n", type, volume);
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_volume_cb(z_addr, (uint8_t)type, volume);
}

static void ag_answer_call_cb(void* cookie, bt_address_t* addr)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] answer_call_cb\n");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_answer_call_cb(z_addr);
}

static void ag_reject_call_cb(void* cookie, bt_address_t* addr)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] reject_call_cb\n");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_reject_call_cb(z_addr);
}

static void ag_hangup_call_cb(void* cookie, bt_address_t* addr)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] hangup_call_cb\n");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_hangup_call_cb(z_addr);
}

static void ag_dial_call_cb(void* cookie, bt_address_t* addr,
    const char* number)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] dial_call_cb: number=%s\n",
        number ? number : "(redial)");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_dial_cb(z_addr, number);
}

static void ag_at_cmd_cb(void* cookie, bt_address_t* addr,
    const char* at_command)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] at_cmd_cb: %s\n", at_command ? at_command : "");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_at_cmd_cb(z_addr, at_command);
}

static void ag_clcc_cmd_cb(void* cookie, bt_address_t* addr)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] clcc_cmd_cb\n");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_clcc_request_cb(z_addr);
}

static void ag_cind_cmd_cb(void* cookie, bt_address_t* addr)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] cind_cmd_cb\n");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_cind_request_cb(z_addr);
}

/* New: call-control / dtmf / nrec / cops callbacks for LOCAL_TELEPHONY=n.
 * These BTP events exist only when AutoPTS drives the telephony state from
 * outside; with local telephony enabled the AG service handles the AT
 * commands internally and never invokes these callbacks.
 */
#ifndef CONFIG_BLUETOOTH_HFP_AG_LOCAL_TELEPHONY
extern void btp_hfp_ag_call_control_cb(const uint8_t* addr, uint8_t chld);
extern void btp_hfp_ag_dtmf_cb(const uint8_t* addr, uint8_t code);
extern void btp_hfp_ag_nrec_cb(const uint8_t* addr, uint8_t enable);
extern void btp_hfp_ag_cops_request_cb(const uint8_t* addr);

static void ag_call_control_cb(void* cookie, bt_address_t* addr, uint8_t chld)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] call_control_cb: chld=%u\n", chld);
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_call_control_cb(z_addr, chld);
}

static void ag_dtmf_cb(void* cookie, bt_address_t* addr, uint8_t code)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] dtmf_cb: code=0x%02x\n", code);
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_dtmf_cb(z_addr, code);
}

static void ag_nrec_req_cb(void* cookie, bt_address_t* addr, bool enable)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] nrec_req_cb: enable=%d\n", (int)enable);
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_nrec_cb(z_addr, enable ? 1 : 0);
}

static void ag_cops_req_cb(void* cookie, bt_address_t* addr)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] cops_req_cb\n");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_cops_request_cb(z_addr);
}

extern void btp_hfp_ag_redial_req_cb(const uint8_t* addr);

static void ag_redial_req_cb(void* cookie, bt_address_t* addr)
{
    uint8_t z_addr[7];

    _info("[z_api_hfp_ag] redial_req_cb\n");
    fw_addr_to_zephyr(addr, 0x00, z_addr);
    btp_hfp_ag_redial_req_cb(z_addr);
}
#endif /* !CONFIG_BLUETOOTH_HFP_AG_LOCAL_TELEPHONY */

static const hfp_ag_callbacks_t ag_cbs = {
    .size = sizeof(hfp_ag_callbacks_t),
    .connection_state_cb = ag_connection_state_cb,
    .audio_state_cb = ag_audio_state_cb,
    .vr_cmd_cb = ag_vr_cmd_cb,
    .hf_battery_update_cb = ag_battery_update_cb,
    .volume_control_cb = ag_volume_control_cb,
    .answer_call_cb = ag_answer_call_cb,
    .reject_call_cb = ag_reject_call_cb,
    .hangup_call_cb = ag_hangup_call_cb,
    .dial_call_cb = ag_dial_call_cb,
    .at_cmd_cb = ag_at_cmd_cb,
    .vender_specific_at_cmd_cb = NULL,
    .clcc_cmd_cb = ag_clcc_cmd_cb,
    .cind_cmd_cb = ag_cind_cmd_cb,
#ifndef CONFIG_BLUETOOTH_HFP_AG_LOCAL_TELEPHONY
    .call_control_cb = ag_call_control_cb,
    .dtmf_cb = ag_dtmf_cb,
    .nrec_req_cb = ag_nrec_req_cb,
    .cops_req_cb = ag_cops_req_cb,
    .redial_req_cb = ag_redial_req_cb,
#endif
};

/* ---- IPC dispatch functions ---- */

static int ag_init_in_ipc(void* arg)
{
    (void)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    if (ag_initialized)
        return 0;

    ag_handle = bt_hfp_ag_register_callbacks(ins, &ag_cbs);
    if (!ag_handle) {
        _info("[z_api_hfp_ag] register_callbacks failed\n");
        return -EIO;
    }

    ag_initialized = true;
    _info("[z_api_hfp_ag] initialized\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
} ag_addr_args_t;

static int ag_connect_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    if (!ag_initialized)
        ag_init_in_ipc(NULL);

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_connect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] connect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] connect initiated\n");
    return 0;
}

static int ag_disconnect_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_disconnect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] disconnect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] disconnect initiated\n");
    return 0;
}

static int ag_connect_audio_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_connect_audio(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] connect_audio failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] connect_audio initiated\n");
    return 0;
}

static int ag_disconnect_audio_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_disconnect_audio(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] disconnect_audio failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] disconnect_audio initiated\n");
    return 0;
}

static int ag_start_virtual_call_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_start_virtual_call(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] start_virtual_call failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] start_virtual_call\n");
    return 0;
}

static int ag_stop_virtual_call_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_stop_virtual_call(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] stop_virtual_call failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] stop_virtual_call\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t num_active;
    uint8_t num_held;
    uint8_t call_state;
    uint8_t addr_type;
    char number[HFP_PHONE_NUMBER_MAX + 1];
    char name[64];
} ag_phone_state_args_t;

static int ag_phone_state_change_in_ipc(void* arg)
{
    ag_phone_state_args_t* a = (ag_phone_state_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_phone_state_change(ins, &fw_addr,
        a->num_active, a->num_held,
        (hfp_ag_call_state_t)a->call_state,
        (hfp_call_addrtype_t)a->addr_type,
        a->number[0] ? a->number : NULL,
        a->name[0] ? a->name : NULL);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] phone_state_change failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] phone_state_change: active=%d held=%d state=%d\n",
        a->num_active, a->num_held, a->call_state);
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t type;
    uint8_t volume;
} ag_volume_args_t;

static int ag_volume_control_in_ipc(void* arg)
{
    ag_volume_args_t* a = (ag_volume_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_volume_control(ins, &fw_addr,
        (hfp_volume_type_t)a->type, a->volume);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] volume_control failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] volume_control: type=%d vol=%d\n",
        a->type, a->volume);
    return 0;
}

static int ag_start_vr_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_start_voice_recognition(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] start_vr failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] start_voice_recognition\n");
    return 0;
}

static int ag_stop_vr_in_ipc(void* arg)
{
    ag_addr_args_t* a = (ag_addr_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_stop_voice_recognition(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] stop_vr failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] stop_voice_recognition\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t network;
    uint8_t roam;
    uint8_t signal;
    uint8_t battery;
} ag_device_status_args_t;

static int ag_device_status_in_ipc(void* arg)
{
    ag_device_status_args_t* a = (ag_device_status_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_notify_device_status(ins, &fw_addr,
        (hfp_network_state_t)a->network,
        (hfp_roaming_state_t)a->roam,
        a->signal, a->battery);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] device_status failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] device_status: net=%d roam=%d sig=%d bat=%d\n",
        a->network, a->roam, a->signal, a->battery);
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    char cmd[256];
} ag_at_cmd_args_t;

static int ag_send_at_cmd_in_ipc(void* arg)
{
    ag_at_cmd_args_t* a = (ag_at_cmd_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_send_at_command(ins, &fw_addr, a->cmd);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] send_at_cmd failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hfp_ag] send_at_cmd: %s\n", a->cmd);
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint32_t index;
    uint8_t dir;
    uint8_t call_state;
    uint8_t mode;
    uint8_t mpty;
    uint8_t addr_type;
    char number[HFP_PHONE_NUMBER_MAX + 1];
} ag_clcc_args_t;

static int ag_clcc_response_in_ipc(void* arg)
{
    ag_clcc_args_t* a = (ag_clcc_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_send_clcc_response(ins, &fw_addr,
        a->index, (hfp_call_direction_t)a->dir,
        (hfp_ag_call_state_t)a->call_state,
        (hfp_call_mode_t)a->mode,
        (hfp_call_mpty_type_t)a->mpty,
        (hfp_call_addrtype_t)a->addr_type,
        a->number[0] ? a->number : NULL);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] clcc_response failed: %d\n", ret);
        return -EIO;
    }

    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t network;
    uint8_t call;
    uint8_t callsetup;
    uint8_t callheld;
    uint8_t signal;
    uint8_t roam;
    uint8_t battery;
} ag_cind_args_t;

static int ag_cind_response_in_ipc(void* arg)
{
    ag_cind_args_t* a = (ag_cind_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hfp_ag_send_cind_response(ins, &fw_addr,
        (hfp_network_state_t)a->network,
        (hfp_call_t)a->call,
        (hfp_callheld_t)a->callheld,
        (hfp_callsetup_t)a->callsetup,
        a->signal,
        (hfp_roaming_state_t)a->roam,
        a->battery);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hfp_ag] cind_response failed: %d\n", ret);
        return -EIO;
    }

    return 0;
}

/* ---- Public z_api HFP AG functions ---- */

int z_bt_hfp_ag_init(void)
{
    _info("[z_api] >>> z_bt_hfp_ag_init\n");
    return z_api_dispatch(ag_init_in_ipc, NULL);
}

int z_bt_hfp_ag_slc_connect(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_slc_connect\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_connect_in_ipc, &args);
}

int z_bt_hfp_ag_slc_disconnect(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_slc_disconnect\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_disconnect_in_ipc, &args);
}

int z_bt_hfp_ag_connect_audio(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_connect_audio\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_connect_audio_in_ipc, &args);
}

int z_bt_hfp_ag_disconnect_audio(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_disconnect_audio\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_disconnect_audio_in_ipc, &args);
}

int z_bt_hfp_ag_start_virtual_call(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_start_virtual_call\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_start_virtual_call_in_ipc, &args);
}

int z_bt_hfp_ag_stop_virtual_call(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_stop_virtual_call\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_stop_virtual_call_in_ipc, &args);
}

int z_bt_hfp_ag_phone_state_change(const uint8_t* addr,
    uint8_t num_active, uint8_t num_held,
    uint8_t call_state, uint8_t addr_type,
    const char* number, const char* name)
{
    _info("[z_api] >>> z_bt_hfp_ag_phone_state_change\n");
    if (!addr)
        return -EINVAL;

    ag_phone_state_args_t args;
    memset(&args, 0, sizeof(args));
    memcpy(args.z_addr, addr, 7);
    args.num_active = num_active;
    args.num_held = num_held;
    args.call_state = call_state;
    args.addr_type = addr_type;
    if (number)
        strncpy(args.number, number, HFP_PHONE_NUMBER_MAX);
    if (name)
        strncpy(args.name, name, 63);
    return z_api_dispatch(ag_phone_state_change_in_ipc, &args);
}

int z_bt_hfp_ag_volume_control(const uint8_t* addr, uint8_t type,
    uint8_t volume)
{
    _info("[z_api] >>> z_bt_hfp_ag_volume_control\n");
    if (!addr)
        return -EINVAL;

    ag_volume_args_t args;
    memcpy(args.z_addr, addr, 7);
    args.type = type;
    args.volume = volume;
    return z_api_dispatch(ag_volume_control_in_ipc, &args);
}

int z_bt_hfp_ag_start_voice_recognition(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_start_voice_recognition\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_start_vr_in_ipc, &args);
}

int z_bt_hfp_ag_stop_voice_recognition(const uint8_t* addr)
{
    _info("[z_api] >>> z_bt_hfp_ag_stop_voice_recognition\n");
    if (!addr)
        return -EINVAL;

    ag_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(ag_stop_vr_in_ipc, &args);
}

int z_bt_hfp_ag_device_status(const uint8_t* addr,
    uint8_t network, uint8_t roam, uint8_t signal, uint8_t battery)
{
    _info("[z_api] >>> z_bt_hfp_ag_device_status\n");
    if (!addr)
        return -EINVAL;

    ag_device_status_args_t args;
    memcpy(args.z_addr, addr, 7);
    args.network = network;
    args.roam = roam;
    args.signal = signal;
    args.battery = battery;
    return z_api_dispatch(ag_device_status_in_ipc, &args);
}

int z_bt_hfp_ag_send_at_cmd(const uint8_t* addr, const char* cmd)
{
    _info("[z_api] >>> z_bt_hfp_ag_send_at_cmd\n");
    if (!addr || !cmd)
        return -EINVAL;

    ag_at_cmd_args_t args;
    memcpy(args.z_addr, addr, 7);
    strncpy(args.cmd, cmd, 255);
    args.cmd[255] = '\0';
    return z_api_dispatch(ag_send_at_cmd_in_ipc, &args);
}

int z_bt_hfp_ag_clcc_response(const uint8_t* addr, uint32_t index,
    uint8_t dir, uint8_t call_state, uint8_t mode, uint8_t mpty,
    uint8_t addr_type, const char* number)
{
    _info("[z_api] >>> z_bt_hfp_ag_clcc_response\n");
    if (!addr)
        return -EINVAL;

    ag_clcc_args_t args;
    memset(&args, 0, sizeof(args));
    memcpy(args.z_addr, addr, 7);
    args.index = index;
    args.dir = dir;
    args.call_state = call_state;
    args.mode = mode;
    args.mpty = mpty;
    args.addr_type = addr_type;
    if (number)
        strncpy(args.number, number, HFP_PHONE_NUMBER_MAX);
    return z_api_dispatch(ag_clcc_response_in_ipc, &args);
}

int z_bt_hfp_ag_cind_response(const uint8_t* addr,
    uint8_t network, uint8_t call, uint8_t callsetup,
    uint8_t callheld, uint8_t signal, uint8_t roam, uint8_t battery)
{
    _info("[z_api] >>> z_bt_hfp_ag_cind_response\n");
    if (!addr)
        return -EINVAL;

    ag_cind_args_t args;
    memcpy(args.z_addr, addr, 7);
    args.network = network;
    args.call = call;
    args.callsetup = callsetup;
    args.callheld = callheld;
    args.signal = signal;
    args.roam = roam;
    args.battery = battery;
    return z_api_dispatch(ag_cind_response_in_ipc, &args);
}

typedef struct {
    uint8_t result;
} ag_dial_response_args_t;

#ifndef CONFIG_BLUETOOTH_HFP_AG_LOCAL_TELEPHONY
static int ag_dial_response_in_ipc(void* arg)
{
    ag_dial_response_args_t* a = (ag_dial_response_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;
    return (int)bt_hfp_ag_dial_response(ins, a->result);
}
#endif

int z_bt_hfp_ag_dial_response(uint8_t result)
{
    _info("[z_api] >>> z_bt_hfp_ag_dial_response: result=%d\n", result);
#ifdef CONFIG_BLUETOOTH_HFP_AG_LOCAL_TELEPHONY
    (void)result;
    /* Not supported when AG drives telephony internally. */
    return -ENOTSUP;
#else
    ag_dial_response_args_t args = { .result = result };
    return z_api_dispatch(ag_dial_response_in_ipc, &args);
#endif
}

typedef struct {
    uint8_t result;
    char number[HFP_PHONE_NUMBER_MAX + 1];
} ag_redial_response_args_t;

#ifndef CONFIG_BLUETOOTH_HFP_AG_LOCAL_TELEPHONY
static int ag_redial_response_in_ipc(void* arg)
{
    ag_redial_response_args_t* a = (ag_redial_response_args_t*)arg;
    bt_instance_t* ins = get_ins();
    if (!ins)
        return -EIO;
    return (int)bt_hfp_ag_redial_response(ins, a->result,
        a->number[0] ? a->number : NULL);
}
#endif

int z_bt_hfp_ag_redial_response(uint8_t result, const char* number)
{
    _info("[z_api] >>> z_bt_hfp_ag_redial_response: result=%d number=%s\n",
        result, number ? number : "(null)");
#ifdef CONFIG_BLUETOOTH_HFP_AG_LOCAL_TELEPHONY
    (void)result;
    (void)number;
    return -ENOTSUP;
#else
    ag_redial_response_args_t args = { .result = result };
    if (number) {
        strlcpy(args.number, number, sizeof(args.number));
    } else {
        args.number[0] = '\0';
    }
    return z_api_dispatch(ag_redial_response_in_ipc, &args);
#endif
}
