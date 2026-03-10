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

#define EXTRACT_LOWEST_BIT(pos, x) \
    do {                           \
        uint32_t _prev = (x);      \
        if (!_prev) {              \
            (pos) = 0;             \
        } else {                   \
            (x) &= (x) - 1;        \
            (pos) = _prev - (x);   \
        }                          \
    } while (0)

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

    struct bt_iso_big* sink; /** zephyr big info */
    struct bt_iso_chan_ops ops;
    struct bt_iso_chan_io_qos rx_qos; /** FIXME: Why do we need this QoS? */
    struct bt_iso_chan_qos qos;
    struct bt_iso_chan* channels[BT_AURACAST_SINK_NUM_BIS_SUPPORTED]; /** point to iso_channel */
    uint32_t bitfield[BT_AURACAST_SINK_NUM_BIS_SUPPORTED]; /** bitfield map for quick check */
} sal_auracast_sink_device_t;

typedef struct {
    bt_list_t* sink_list;
} sal_auracast_sink_info_t;

extern void* bt_sal_zephyr_pa_sync_get(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr);

static sal_auracast_sink_info_t* g_sal_auracast_sink_info = NULL;
static void iso_recv(struct bt_iso_chan* chan, const struct bt_iso_recv_info* info,
    struct net_buf* buf);
static void iso_connected(struct bt_iso_chan* chan);
static void iso_disconnected(struct bt_iso_chan* chan, uint8_t reason);

static sal_auracast_sink_device_t* device_new(bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid, struct bt_iso_big_sync_param* z_param)
{
    sal_auracast_sink_device_t* device;
    uint32_t bitfield = z_param->bis_bitfield;

    if (z_param->num_bis > BT_AURACAST_SINK_NUM_BIS_SUPPORTED) {
        BT_LOGE("%s, invalid num_bis(%d)", __func__, z_param->num_bis);
        return NULL;
    }

    if (z_param->num_bis != bt_utils_count_ones(bitfield)) {
        BT_LOGE("%s, invalid param", __func__);
        return NULL;
    }

    device = zalloc(sizeof(sal_auracast_sink_device_t));
    if (!device) {
        BT_LOGE("%s, malloc failed", __func__);
        return NULL;
    }

    memcpy(&device->addr, addr, sizeof(bt_le_address_t));
    device->id = id;
    device->sid = sid;
    bitfield <<= 1; /** Zephyr bitfield to SAL bitfield */

    for (uint8_t k = 0; k < z_param->num_bis; k++) {
        struct bt_iso_chan* channel = zalloc(sizeof(struct bt_iso_chan));
        if (!channel)
            goto error;

        device->ops.recv = iso_recv;
        device->ops.connected = iso_connected;
        device->ops.disconnected = iso_disconnected;
        device->qos.rx = &device->rx_qos;
        channel->ops = &device->ops;
        channel->qos = &device->qos;

        device->channels[k] = channel;

        EXTRACT_LOWEST_BIT(device->bitfield[k], bitfield);
    }

    return device;

error:
    for (uint8_t k = 0; k < BT_AURACAST_SINK_NUM_BIS_SUPPORTED; k++)
        free(device->channels[k]);

    free(device);
    return NULL;
}

static void device_delete(sal_auracast_sink_device_t* device)
{
    BT_LOGD("%s", __func__);

    if (!device)
        return;

    auracast_sink_on_terminated(device->id, &device->addr, device->sid);
    for (uint8_t k = 0; k < BT_AURACAST_SINK_NUM_BIS_SUPPORTED; k++)
        free(device->channels[k]);

    free(device);
}

static bool req_cmp(void* data, void* context)
{
    const sal_auracast_sink_device_t* device = (const sal_auracast_sink_device_t*)data;
    const sal_auracast_sink_req_t* req = (const sal_auracast_sink_req_t*)context;

    if (!device || !req)
        return false;

    if (memcmp(&device->addr, &req->addr, sizeof(bt_le_address_t)))
        return false;

    if (device->id != req->id || device->sid != req->sid)
        return false;

    return true;
}

static sal_auracast_sink_device_t* find_device_by_req(const sal_auracast_sink_req_t* req)
{
    if (!g_sal_auracast_sink_info || !g_sal_auracast_sink_info->sink_list || !req)
        return NULL;

    return (sal_auracast_sink_device_t*)bt_list_find(g_sal_auracast_sink_info->sink_list, req_cmp,
        (void*)req);
}

static bool channel_cmp(void* data, void* context)
{
    const sal_auracast_sink_device_t* device = (const sal_auracast_sink_device_t*)data;
    const struct bt_iso_chan* channel = (const struct bt_iso_chan*)context;

    if (!device || !channel)
        return false;

    for (uint8_t k = 0; k < BT_AURACAST_SINK_NUM_BIS_SUPPORTED; k++) {
        if (device->channels[k] == channel)
            return true;
    }

    return false;
}

static sal_auracast_sink_device_t* find_device_by_channel(const struct bt_iso_chan* channel)
{
    if (!g_sal_auracast_sink_info || !g_sal_auracast_sink_info->sink_list || !channel)
        return NULL;

    return (sal_auracast_sink_device_t*)bt_list_find(g_sal_auracast_sink_info->sink_list,
        channel_cmp, (void*)channel);
}

static void sink_removed(void* data)
{
    sal_auracast_sink_device_t* device = (sal_auracast_sink_device_t*)data;

    BT_LOGD("%s", __func__);

    device_delete(device);
}

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

static void iso_recv(struct bt_iso_chan* chan, const struct bt_iso_recv_info* info,
    struct net_buf* buf)
{
    sal_auracast_sink_device_t* device = find_device_by_channel(chan);
    if (!device)
        return;

    for (uint8_t k = 0; k < BT_AURACAST_SINK_NUM_BIS_SUPPORTED; k++) {
        if (device->channels[k] == chan) {
            auracast_sink_on_data_received(device->id, &device->addr, device->sid,
                device->bitfield[k], info->ts, info->seq_num, buf->len, buf->data);
        }
    }
}

static void iso_connected(struct bt_iso_chan* chan)
{
    sal_auracast_sink_device_t* device;

    BT_LOGD("%s", __func__);

    device = find_device_by_channel(chan);
    if (!device)
        return;

    auracast_sink_on_established(device->id, &device->addr, device->sid);
}

static void iso_disconnected(struct bt_iso_chan* chan, uint8_t reason)
{
    sal_auracast_sink_device_t* device;

    BT_LOGD("%s", __func__);

    device = find_device_by_channel(chan);
    if (!device)
        return;

    auracast_sink_on_terminated(device->id, &device->addr, device->sid);
}

static void create_sync(const void* data)
{
    const sal_auracast_sink_req_t* req = (const sal_auracast_sink_req_t*)data;
    struct bt_iso_big_sync_param* z_param = (struct bt_iso_big_sync_param*)req->context;
    sal_auracast_sink_device_t* device;
    void* sync;
    int err;

    BT_LOGD("%s", __func__);

    device = device_new(req->id, &req->addr, req->sid, z_param);
    if (!device) {
        auracast_sink_on_terminated(req->id, &req->addr, req->sid);
        free(z_param);
        return;
    }

    z_param->bis_channels = device->channels;
    sync = bt_sal_zephyr_pa_sync_get(req->id, req->sid, &req->addr);
    if (!sync) {
        BT_LOGE("periodic advertising does not exist");
        goto error;
    }

    err = bt_iso_big_sync(sync, z_param, &device->sink);
    if (err) {
        BT_LOGE("failed to create sync, err = %d", err);
        goto error;
    }

    if (!device->sink) {
        BT_LOGE("big not generated");
        goto error;
    }

    bt_list_add_tail(g_sal_auracast_sink_info->sink_list, device);

    free(z_param);
    return;

error:
    device_delete(device);
    free(z_param);
    return;
}

static void terminate_sync(const void* data)
{
    const sal_auracast_sink_req_t* req = (const sal_auracast_sink_req_t*)data;
    sal_auracast_sink_device_t* device;
    int err;

    BT_LOGD("%s", __func__);

    device = find_device_by_req(req);
    if (!device) {
        auracast_sink_on_terminated(req->id, &req->addr, req->sid);
        return;
    }

    err = bt_iso_big_terminate(device->sink);
    if (err) {
        BT_LOGE("failed to terminate sync, err = %d", err);
        bt_list_remove(g_sal_auracast_sink_info->sink_list, device);
        return;
    }
}

bt_status_t bt_sal_auracast_sink_init(void)
{
    g_sal_auracast_sink_info = zalloc(sizeof(sal_auracast_sink_info_t));
    if (!g_sal_auracast_sink_info)
        return BT_STATUS_NOMEM;

    g_sal_auracast_sink_info->sink_list = bt_list_new(sink_removed);
    if (g_sal_auracast_sink_info->sink_list == NULL)
        goto error;

    return BT_STATUS_SUCCESS;

error:
    bt_list_free(g_sal_auracast_sink_info->sink_list);
    free(g_sal_auracast_sink_info);
    return BT_STATUS_FAIL;
}

bt_status_t bt_sal_auracast_sink_cleanup(void)
{
    if (!g_sal_auracast_sink_info)
        return BT_STATUS_DONE;

    /* TODO: add unregisteration for stack callbacks */
    bt_list_free(g_sal_auracast_sink_info->sink_list);
    free(g_sal_auracast_sink_info);
    g_sal_auracast_sink_info = NULL;

    return BT_STATUS_SUCCESS;
}

static void sal_create_sink_param_sal_to_zephyr(struct bt_iso_big_sync_param* z_param,
    const bt_sal_auracast_sink_param_t* params)
{
    z_param->bis_channels = NULL; /**< to be assigned in @ref create_sync() */
    z_param->num_bis = bt_utils_count_ones(params->bis);
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