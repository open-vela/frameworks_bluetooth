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
#include "bt_hid_device.h"
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

/* ---- HID state management ---- */

static void *hid_cb_handle;
static bool hid_initialized;
static bool hid_app_registered;

/* BTP HID callback declarations */
extern void btp_hid_connected_cb(const uint8_t *addr);
extern void btp_hid_disconnected_cb(const uint8_t *addr);

/* ---- HID Framework callbacks ---- */

static void hid_connection_state_cb(void *cookie, bt_address_t *addr,
    bool le_hid, profile_connection_state_t state)
{
    uint8_t z_addr[7];

    _info("[z_api_hid] connection_state_cb: state=%d le=%d\n", state, le_hid);

    fw_addr_to_zephyr(addr, 0x00, z_addr);

    if (state == PROFILE_STATE_CONNECTED) {
        btp_hid_connected_cb(z_addr);
    } else if (state == PROFILE_STATE_DISCONNECTED) {
        btp_hid_disconnected_cb(z_addr);
    }
}

static const hid_device_callbacks_t hid_cbs = {
    .size = sizeof(hid_device_callbacks_t),
    .connection_state_cb = hid_connection_state_cb,
};

/* ---- IPC dispatch functions ---- */

static int hid_init_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (hid_initialized) return 0;

    hid_cb_handle = bt_hid_device_register_callbacks(ins, &hid_cbs);
    if (!hid_cb_handle) {
        _info("[z_api_hid] register_callbacks failed\n");
        return -EIO;
    }

    hid_initialized = true;
    _info("[z_api_hid] initialized\n");
    return 0;
}

typedef struct {
    uint8_t sub_class;
} hid_register_args_t;

static int hid_register_in_ipc(void *arg)
{
    hid_register_args_t *a = (hid_register_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (!hid_initialized) {
        hid_init_in_ipc(NULL);
    }

    if (hid_app_registered) return 0;

    /* Minimal SDP settings for HID device */
    hid_device_sdp_settings_t sdp_settings = {
        .name = "AutoPTS HID Device",
        .description = "BTP HID Tester",
        .provider = "XiaoMi",
        .hids_info = {
            .attr_mask = 0,
            .sub_class = a->sub_class,
            .country_code = 0,
            .vendor_id = 0,
            .product_id = 0,
            .version = 0,
        },
    };

    bt_status_t ret = bt_hid_device_register_app(ins, &sdp_settings, false);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hid] register_app failed: %d\n", ret);
        return -EIO;
    }

    hid_app_registered = true;
    _info("[z_api_hid] app registered\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
} hid_addr_args_t;

static int hid_connect_in_ipc(void *arg)
{
    hid_addr_args_t *a = (hid_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (!hid_initialized) {
        hid_init_in_ipc(NULL);
    }

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hid_device_connect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hid] connect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hid] connect initiated\n");
    return 0;
}

static int hid_disconnect_in_ipc(void *arg)
{
    hid_addr_args_t *a = (hid_addr_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hid_device_disconnect(ins, &fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hid] disconnect failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hid] disconnect initiated\n");
    return 0;
}

typedef struct {
    uint8_t z_addr[7];
    uint8_t report_id;
    uint16_t data_len;
    uint8_t data[256];
} hid_send_report_args_t;

static int hid_send_report_in_ipc(void *arg)
{
    hid_send_report_args_t *a = (hid_send_report_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_address_t fw_addr;
    zephyr_addr_to_fw(a->z_addr, &fw_addr);

    bt_status_t ret = bt_hid_device_send_report(ins, &fw_addr,
        a->report_id, a->data, a->data_len);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api_hid] send_report failed: %d\n", ret);
        return -EIO;
    }

    _info("[z_api_hid] report sent\n");
    return 0;
}

/* ---- Public z_api HID functions ---- */

int z_bt_hid_register(uint8_t sub_class)
{
    _info("[z_api] >>> z_bt_hid_register: sub_class=%d\n", sub_class);

    hid_register_args_t args;
    args.sub_class = sub_class;
    return z_api_dispatch(hid_register_in_ipc, &args);
}

int z_bt_hid_connect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_hid_connect\n");
    if (!addr) return -EINVAL;

    hid_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hid_connect_in_ipc, &args);
}

int z_bt_hid_disconnect(const uint8_t *addr)
{
    _info("[z_api] >>> z_bt_hid_disconnect\n");
    if (!addr) return -EINVAL;

    hid_addr_args_t args;
    memcpy(args.z_addr, addr, 7);
    return z_api_dispatch(hid_disconnect_in_ipc, &args);
}

int z_bt_hid_send_report(const uint8_t *addr, uint8_t report_id,
                         const uint8_t *data, uint16_t len)
{
    _info("[z_api] >>> z_bt_hid_send_report: id=%d len=%d\n", report_id, len);
    if (!addr || !data) return -EINVAL;

    hid_send_report_args_t args;
    memcpy(args.z_addr, addr, 7);
    args.report_id = report_id;
    args.data_len = (len > 256) ? 256 : len;
    memcpy(args.data, data, args.data_len);
    return z_api_dispatch(hid_send_report_in_ipc, &args);
}
