/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef CONFIG_BLUETOOTH_PBAP_PCE

#include "stack_adapter_pbap_pce.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_pbap_pce_interface.h"

static void connection_state_changed_cb(BD_ADDR remote_addr,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, 6);
    pce_on_connection_state_changed(&addr, bluelet_profile_connection_state(state));
}

static void dir_changed_cb(BD_ADDR remote_addr, SERVICE_BT_STATUS status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, 6);
    pce_on_dir_changed(&addr, status);
}

static void vcard_listing_data_cb(BD_ADDR remote_addr, char* obj, uint16_t len)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, 6);
    pce_on_vcard_listing_data_received(&addr, obj, len);
}

static void vcard_listing_end_cb(BD_ADDR remote_addr, uint16_t status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, 6);
    pce_on_vcard_listing_end(&addr, status);
}

static void vcard_data_cb(BD_ADDR remote_addr, char* obj, uint16_t len)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, 6);
    pce_on_vcard_data_received(&addr, obj, len);
}

static void vcard_end_cb(BD_ADDR remote_addr, uint16_t status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, 6);
    pce_on_vcard_end(&addr, status);
}

static PBAP_PCE_CALLBACKS_S pbap_pce_cbks = {
    .size = sizeof(pbap_pce_cbks),
    .pce_connection_state_cb = connection_state_changed_cb,
    .pce_dir_changed_cb = dir_changed_cb,
    .pce_vcard_listing_data_cb = vcard_listing_data_cb,
    .pce_vcard_listing_end_cb = vcard_listing_end_cb,
    .pce_vcard_data_cb = vcard_data_cb,
    .pce_vcard_end_cb = vcard_end_cb,
};

bt_status_t bt_sal_pce_init(void)
{
    SAL_CHECK_RET(service_adapter_pce_init(&pbap_pce_cbks), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_pce_cleanup(void)
{
    service_adapter_pce_cleanup();
}

bt_status_t bt_sal_pce_connect(bt_address_t* addr)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_pce_connect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pce_disconnect(bt_address_t* addr)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_pce_disconnect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pce_change_directory(bt_address_t* addr, const char* dir)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(dir);

    SAL_CHECK_RET(service_adapter_pce_change_dir(addr->addr, dir),
        SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pce_pull_vcard_listing(bt_address_t* addr, pbap_search_property_t property, const char* value)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_pce_pull_vcard_listing(addr->addr, property, value),
        SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pce_pull_vcard(bt_address_t* addr, const char* object, uint64_t filter)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(object);

    SAL_CHECK_RET(service_adapter_pce_pull_vcard(addr->addr, object, filter),
        SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif
