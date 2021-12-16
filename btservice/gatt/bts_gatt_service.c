/****************************************************************************
 * frameworks/bluetooth/btservice/gatt/bts_gatt_service.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>
#include "bts_gatt_service.h"
#include "log.h"

static bt_result_code gatt_init(void);
static void gatt_cleanup(void);

static gatt_interface_t gatt_if = {
    .size = sizeof(gatt_if),

    .init = gatt_init,
    .cleanup = gatt_cleanup,

    .client = NULL,
    .server = NULL,
    .scanner = NULL,
    .advertiser = NULL,
};

static bt_result_code gatt_init(void)
{
    gatt_status ret = service_adapter_gatt_init();
    if (ret != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt_init ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void gatt_cleanup(void)
{
    service_adapter_gatt_cleanup();
}

const gatt_interface_t* gatt_get_interface(void)
{
#if defined(CONFIG_BLUETOOTH_GATT_CLIENT)
    gatt_if.client = get_bts_gattc_instance();
#endif
#if defined(CONFIG_BLUETOOTH_GATT_SERVER)
    gatt_if.server = get_bts_gatts_instance();
#endif
#if defined(CONFIG_BLUETOOTH_LE_SCAN)
    gatt_if.scanner = get_bts_lescan_instance();
#endif
#if defined(CONFIG_BLUETOOTH_LE_ADVERTISE)
    gatt_if.advertiser = get_bts_bleadv_instance();
#endif
    return &gatt_if;
}