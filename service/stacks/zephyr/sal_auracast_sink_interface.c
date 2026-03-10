/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#define LOG_TAG "sal_auracast_sink"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>

#include "auracast_sink_service.h"
#include "bt_utils.h"
#include "sal_auracast_sink_interface.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#define ZEPHYR_AURACAST_SINK_SUPPORTED CONFIG_BT_ISO_RX

#if ZEPHYR_AURACAST_SINK_SUPPORTED

typedef void (*sal_func_t)(const void* args);

typedef struct {
    bt_le_address_t addr;
    bt_controller_id_t id;
    uint8_t sid;
    sal_func_t func;
    void* context;
} sal_auracast_sink_req_t;

typedef struct {
    bt_le_address_t addr;
    bt_controller_id_t id;
    uint8_t sid;
} sal_auracast_sink_device_t;

typedef struct {
    bt_list_t* sink_list;
} sal_auracast_sink_info_t;

static sal_auracast_sink_req_t* sal_auracast_sink_req(bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid, sal_func_t func, void* context)
{
    sal_auracast_sink_req_t* req = zalloc(sizeof(sal_auracast_sink_req_t));

    if (!req) {
        BT_LOGE("%s, req malloc fail", __func__);
        return NULL;
    }

    memcpy(&req->addr, addr, sizeof(bt_le_address_t));
    req->id = id;
    req->sid = sid;
    req->func = func;
    req->context = context;

    return req;
}

static void sal_invoke_async(service_work_t* work, void* data)
{
    sal_auracast_sink_req_t* req = data;

    SAL_ASSERT(req);
    req->func(req);

    free(data);
}

static bt_status_t sal_send_req(sal_auracast_sink_req_t* req)
{
    if (!req)
        return BT_STATUS_PARM_INVALID;

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_auracast_sink_init(void)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_auracast_sink_cleanup(void)
{
    return BT_STATUS_SUCCESS;
}

static void sal_create_sink_param_sal_to_zephyr(struct bt_iso_big_sync_param* z_param,
    const bt_sal_auracast_sink_param_t* params)
{
    z_param->bis_channels = NULL; /**< to be assigned in @ref create_sync() */
    z_param->num_bis = 0; /** TODO: count the valid numbers of bitfiled */
    z_param->bis_bitfield = params->bis >> 1;
    z_param->mse = params->mse;
    z_param->sync_timeout = params->sync_timeout;
    z_param->encryption = params->broadcast_code != NULL;
    if (params->broadcast_code)
        memcpy(z_param->bcode, params->broadcast_code, BT_ISO_BROADCAST_CODE_SIZE);
}

bt_status_t bt_sal_auracast_sink_create_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr, const bt_sal_auracast_sink_param_t* params)
{
    sal_auracast_sink_req_t* req;
    struct bt_iso_big_sync_param* z_param;

    BT_LOGD("%s", __func__);

    z_param = zalloc(sizeof(struct bt_iso_big_sync_param));
    if (!z_param)
        return BT_STATUS_NOMEM;

    sal_create_sink_param_sal_to_zephyr(z_param, params);
    req = sal_auracast_sink_req(id, addr, sid, create_sync, z_param);
    if (!req)
        goto error;

    if (sal_send_req(req) != BT_STATUS_SUCCESS)
        goto error;

    return BT_STATUS_SUCCESS;

error:
    free(z_param);
    free(req);

    return BT_STATUS_FAIL;
}

bt_status_t bt_sal_auracast_sink_terminate_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr)
{
    sal_auracast_sink_req_t* req = sal_auracast_sink_req(id, addr, sid, terminate_sync, NULL);
    if (!req)
        return BT_STATUS_NOMEM;

    if (sal_send_req(req) != BT_STATUS_SUCCESS) {
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

#else /* !ZEPHYR_AURACAST_SINK_SUPPORTED */
bt_status_t bt_sal_auracast_sink_init(void) { return BT_STATUS_NOT_SUPPORTED; }

bt_status_t bt_sal_auracast_sink_cleanup(void) { return BT_STATUS_NOT_SUPPORTED; }

bt_status_t bt_sal_auracast_sink_create_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr, const bt_sal_auracast_sink_param_t* params)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_auracast_sink_terminate_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr)
{
    return BT_STATUS_NOT_SUPPORTED;
}
#endif /* ZEPHYR_AURACAST_SINK_SUPPORTED */