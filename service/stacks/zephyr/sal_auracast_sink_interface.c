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

bt_status_t bt_sal_auracast_sink_init(void)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_auracast_sink_cleanup(void)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_auracast_sink_create_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr, const bt_sal_auracast_sink_param_t* params)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_auracast_sink_terminate_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr)
{
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