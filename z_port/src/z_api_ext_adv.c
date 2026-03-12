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
#include "bt_adapter.h"
#include "bt_le_advertiser.h"
#include "utils/log.h"

/* Dispatch mechanism */
typedef int (*z_api_func_t)(void *arg);
extern int z_api_dispatch(z_api_func_t func, void *arg);
extern bt_instance_t *local_bt_ins;

#define ADV_DATA_LEN_MAX 31
#define MAX_EXT_ADV 4

/* Extended ADV instance - stores params and data until start */
typedef struct {
    bool in_use;
    bool started;
    /* Zephyr bt_le_adv_param fields we care about */
    uint32_t options;
    uint32_t interval_min;
    uint32_t interval_max;
    /* Stored AD/SD data (raw bytes from bt_data[]) */
    uint8_t ad_raw[ADV_DATA_LEN_MAX];
    uint16_t ad_raw_len;
    uint8_t sd_raw[ADV_DATA_LEN_MAX];
    uint16_t sd_raw_len;
    /* Framework advertiser handle */
    bt_advertiser_t *fw_adv;
} ext_adv_inst_t;

static ext_adv_inst_t g_ext_adv[MAX_EXT_ADV];

/* Advertising callbacks */
static void ext_adv_start_cb(bt_advertiser_t *adv, uint8_t adv_id, uint8_t status)
{
    _info("[z_api] ext_adv_start_cb: adv_id=%d status=%d\n", adv_id, status);
}

static void ext_adv_stopped_cb(bt_advertiser_t *adv, uint8_t adv_id)
{
    _info("[z_api] ext_adv_stopped_cb: adv_id=%d\n", adv_id);
    for (int i = 0; i < MAX_EXT_ADV; i++) {
        if (g_ext_adv[i].fw_adv == adv) {
            g_ext_adv[i].fw_adv = NULL;
            g_ext_adv[i].started = false;
        }
    }
}

static advertiser_callback_t ext_adv_cbs = {
    sizeof(advertiser_callback_t),
    ext_adv_start_cb,
    ext_adv_stopped_cb
};

static bt_instance_t *get_ins(void)
{
    if (local_bt_ins) return local_bt_ins;
    return (bt_instance_t *)z_api(bt_svc_ins_get)();
}

int z_api(bt_le_ext_adv_create)(const void *param, const void *cb, void **out_adv)
{
    _info("[z_api] >>> z_bt_le_ext_adv_create\n");
    if (!out_adv) return -EINVAL;

    /* Parse Zephyr bt_le_adv_param */
    typedef struct {
        uint8_t id; uint8_t sid; uint8_t secondary_max_skip; uint8_t _pad;
        uint32_t options; uint32_t interval_min; uint32_t interval_max;
    } z_adv_param_t;
    const z_adv_param_t *zp = (const z_adv_param_t *)param;

    /* Find free slot */
    for (int i = 0; i < MAX_EXT_ADV; i++) {
        if (!g_ext_adv[i].in_use) {
            memset(&g_ext_adv[i], 0, sizeof(ext_adv_inst_t));
            g_ext_adv[i].in_use = true;
            if (zp) {
                g_ext_adv[i].options = zp->options;
                g_ext_adv[i].interval_min = zp->interval_min;
                g_ext_adv[i].interval_max = zp->interval_max;
            }
            *out_adv = &g_ext_adv[i];
            _info("[z_api] ext_adv_create: slot=%d opts=0x%x\n", i, g_ext_adv[i].options);
            return 0;
        }
    }
    return -ENOMEM;
}

int z_api(bt_le_ext_adv_delete)(void *adv)
{
    _info("[z_api] >>> z_bt_le_ext_adv_delete\n");
    ext_adv_inst_t *inst = (ext_adv_inst_t *)adv;
    if (!inst || !inst->in_use) return -EINVAL;

    if (inst->fw_adv) {
        bt_instance_t *ins = get_ins();
        if (ins) bt_le_stop_advertising(ins, inst->fw_adv);
    }
    memset(inst, 0, sizeof(ext_adv_inst_t));
    return 0;
}

int z_api(bt_le_ext_adv_set_data)(void *adv,
                                  const void *ad, size_t ad_len,
                                  const void *sd, size_t sd_len)
{
    _info("[z_api] >>> z_bt_le_ext_adv_set_data: ad_len=%zu sd_len=%zu\n", ad_len, sd_len);
    ext_adv_inst_t *inst = (ext_adv_inst_t *)adv;
    if (!inst || !inst->in_use) return -EINVAL;

    typedef struct { uint8_t type; uint8_t data_len; const uint8_t *data; } z_bt_data_t;

    /* Convert bt_data[] to raw AD bytes */
    inst->ad_raw_len = 0;
    if (ad && ad_len > 0) {
        const z_bt_data_t *arr = (const z_bt_data_t *)ad;
        for (size_t i = 0; i < ad_len && inst->ad_raw_len < ADV_DATA_LEN_MAX - 2; i++) {
            inst->ad_raw[inst->ad_raw_len++] = arr[i].data_len + 1;
            inst->ad_raw[inst->ad_raw_len++] = arr[i].type;
            if (arr[i].data && arr[i].data_len > 0) {
                size_t n = arr[i].data_len;
                if (inst->ad_raw_len + n > ADV_DATA_LEN_MAX)
                    n = ADV_DATA_LEN_MAX - inst->ad_raw_len;
                memcpy(&inst->ad_raw[inst->ad_raw_len], arr[i].data, n);
                inst->ad_raw_len += n;
            }
        }
    }

    inst->sd_raw_len = 0;
    if (sd && sd_len > 0) {
        const z_bt_data_t *arr = (const z_bt_data_t *)sd;
        for (size_t i = 0; i < sd_len && inst->sd_raw_len < ADV_DATA_LEN_MAX - 2; i++) {
            inst->sd_raw[inst->sd_raw_len++] = arr[i].data_len + 1;
            inst->sd_raw[inst->sd_raw_len++] = arr[i].type;
            if (arr[i].data && arr[i].data_len > 0) {
                size_t n = arr[i].data_len;
                if (inst->sd_raw_len + n > ADV_DATA_LEN_MAX)
                    n = ADV_DATA_LEN_MAX - inst->sd_raw_len;
                memcpy(&inst->sd_raw[inst->sd_raw_len], arr[i].data, n);
                inst->sd_raw_len += n;
            }
        }
    }

    _info("[z_api] ext_adv_set_data: ad_raw=%d sd_raw=%d\n", inst->ad_raw_len, inst->sd_raw_len);
    return 0;
}

/* Dispatch helper for ext_adv_start */
static int ext_adv_start_in_ipc(void *arg)
{
    ext_adv_inst_t *inst = (ext_adv_inst_t *)arg;
    bt_instance_t *ins = get_ins();

    _info("[z_api] ext_adv_start_in_ipc: ins=%p opts=0x%x ad=%d sd=%d\n",
          ins, inst->options, inst->ad_raw_len, inst->sd_raw_len);
    if (!ins) return -EIO;

    /* Stop previous if running */
    if (inst->fw_adv) {
        bt_le_stop_advertising(ins, inst->fw_adv);
        inst->fw_adv = NULL;
    }

    ble_adv_params_t params;
    memset(&params, 0, sizeof(params));

    /* Use LEGACY types since nRF54L15 doesn't support extended adv */
    if (inst->options & 0x03)       /* CONNECTABLE */
        params.adv_type = BT_LE_LEGACY_ADV_IND;
    else if (inst->options & 0x200)  /* SCANNABLE */
        params.adv_type = BT_LE_LEGACY_ADV_SCAN_IND;
    else
        params.adv_type = BT_LE_LEGACY_ADV_NONCONN_IND;

    uint32_t interval = (inst->interval_min + inst->interval_max) / 2;
    params.interval = interval ? interval : 160;
    params.channel_map = BT_LE_ADV_CHANNEL_DEFAULT;
    params.duration = 0;
    params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE;

    _info("[z_api] ext_adv_start_in_ipc: type=%d interval=%d\n",
          params.adv_type, params.interval);

    inst->fw_adv = bt_le_start_advertising(ins, &params,
        inst->ad_raw, inst->ad_raw_len,
        inst->sd_raw, inst->sd_raw_len, &ext_adv_cbs);

    _info("[z_api] ext_adv_start_in_ipc: fw_adv=%p\n", inst->fw_adv);
    if (inst->fw_adv) inst->started = true;
    return inst->fw_adv ? 0 : -EIO;
}

int z_api(bt_le_ext_adv_start)(void *adv, const void *param)
{
    _info("[z_api] >>> z_bt_le_ext_adv_start\n");
    ext_adv_inst_t *inst = (ext_adv_inst_t *)adv;
    if (!inst || !inst->in_use) return -EINVAL;
    return z_api_dispatch(ext_adv_start_in_ipc, inst);
}

/* Dispatch helper for ext_adv_stop */
static int ext_adv_stop_in_ipc(void *arg)
{
    ext_adv_inst_t *inst = (ext_adv_inst_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins || !inst->fw_adv) return 0;
    bt_le_stop_advertising(ins, inst->fw_adv);
    inst->fw_adv = NULL;
    inst->started = false;
    return 0;
}

int z_api(bt_le_ext_adv_stop)(void *adv)
{
    _info("[z_api] >>> z_bt_le_ext_adv_stop\n");
    ext_adv_inst_t *inst = (ext_adv_inst_t *)adv;
    if (!inst || !inst->in_use) return -EINVAL;
    return z_api_dispatch(ext_adv_stop_in_ipc, inst);
}

int z_api(bt_le_ext_adv_update_param)(void *adv, const void *param)
{
    _info("[z_api] >>> z_bt_le_ext_adv_update_param\n");
    ext_adv_inst_t *inst = (ext_adv_inst_t *)adv;
    if (!inst || !inst->in_use) return -EINVAL;

    typedef struct {
        uint8_t id; uint8_t sid; uint8_t secondary_max_skip; uint8_t _pad;
        uint32_t options; uint32_t interval_min; uint32_t interval_max;
    } z_adv_param_t;
    const z_adv_param_t *zp = (const z_adv_param_t *)param;
    if (zp) {
        inst->options = zp->options;
        inst->interval_min = zp->interval_min;
        inst->interval_max = zp->interval_max;
    }
    return 0;
}

int z_api(bt_le_adv_set_enable_ext)(void *adv, bool enable, const void *param)
{
    if (enable)
        return z_api(bt_le_ext_adv_start)(adv, param);
    else
        return z_api(bt_le_ext_adv_stop)(adv);
}
