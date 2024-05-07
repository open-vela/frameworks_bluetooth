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

#include "stack_adapter_common.h"
#include "stack_adapter_hid.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_hid_device_interface.h"

#ifdef CONFIG_BLUETOOTH_HID_DEVICE
static void app_state_cb(SERVICE_BTHD_APP_STATE state)
{
    hid_device_on_app_state_changed(bluelet_hid_app_state(state));
}

static void connection_state_changed_cb(BD_ADDR remote_addr, bool le_hid,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    hid_device_on_connection_state_changed(&addr, le_hid, bluelet_profile_connection_state(state));
}

static void get_report_cb(BD_ADDR remote_addr, uint8_t rpt_type,
    uint8_t rpt_id, uint16_t buffer_size)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    hid_device_on_get_report(&addr, rpt_type, rpt_id, buffer_size);
}

static void set_report_cb(BD_ADDR remote_addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    hid_device_on_set_report(&addr, rpt_type, rpt_size, rpt_data);
}

static void set_protocol_cb(BD_ADDR remote_addr, uint8_t protocol)
{
    // TODO
}

static void intr_data_cb(BD_ADDR remote_addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    hid_device_on_receive_report(&addr, rpt_type, rpt_size, rpt_data);
}

static void virtual_cable_unplug_cb(BD_ADDR remote_addr)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    hid_device_on_virtual_cable_unplug(&addr);
}

static HID_DEVICE_CALLBACKS_S hid_device_callbacks = {
    .size = sizeof(hid_device_callbacks),
    .bthd_app_state_cb = app_state_cb,
    .bthd_connection_state_changed_cb = connection_state_changed_cb,
    .bthd_get_report_cb = get_report_cb,
    .bthd_set_report_cb = set_report_cb,
    .bthd_set_protocol_cb = set_protocol_cb,
    .bthd_intr_data_cb = intr_data_cb,
    .bthd_virtual_cable_unplug_cb = virtual_cable_unplug_cb,
};

bt_status_t bt_sal_hid_device_init(void)
{
    SAL_CHECK_RET(service_adapter_hid_device_init(&hid_device_callbacks), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_hid_device_cleanup(void)
{
    service_adapter_hid_device_cleanup();
}

bt_status_t bt_sal_hid_device_register_app(hid_device_sdp_settings_t* sdp, bool le_hid)
{
    SERVICE_HID_SERVICE_INFO_S hid_svc_info;

    memset(&hid_svc_info, 0, sizeof(SERVICE_HID_SERVICE_INFO_S));
    hid_svc_info.name = sdp->name;
    hid_svc_info.description = sdp->description;
    hid_svc_info.provider = sdp->provider;
    hid_svc_info.hids_info.attr_mask = sdp->hids_info.attr_mask;
    hid_svc_info.hids_info.sub_class = sdp->hids_info.sub_class;
    hid_svc_info.hids_info.country_code = sdp->hids_info.country_code;
    hid_svc_info.hids_info.vendor_id = sdp->hids_info.vendor_id;
    hid_svc_info.hids_info.product_id = sdp->hids_info.product_id;
    hid_svc_info.hids_info.version = sdp->hids_info.version;
    hid_svc_info.hids_info.supervision_timeout = sdp->hids_info.supervision_timeout;
    hid_svc_info.hids_info.ssr_max_latency = sdp->hids_info.ssr_max_latency;
    hid_svc_info.hids_info.ssr_min_timeout = sdp->hids_info.ssr_min_timeout;
    hid_svc_info.hids_info.dsc_list_length = sdp->hids_info.dsc_list_length;
    hid_svc_info.hids_info.dsc_list = sdp->hids_info.dsc_list;
    hid_svc_info.br_hid = !le_hid;
    hid_svc_info.le_hid = le_hid;

    SAL_CHECK_RET(service_adapter_hid_device_register_app(&hid_svc_info, NULL, NULL), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_unregister_app(void)
{
    SAL_CHECK_RET(service_adapter_hid_device_unregister_app(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_connect(bt_address_t* addr)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_hid_device_connect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_disconnect(bt_address_t* addr)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_hid_device_disconnect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_get_report_response(bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(rpt_data);

    SAL_CHECK_RET(service_adapter_hid_device_get_report_response(addr->addr, rpt_type, rpt_data, rpt_size), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_report_error(bt_address_t* addr, hid_status_error_t error)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_hid_device_report_error(addr->addr, error), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_send_report(bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(rpt_data);

    SAL_CHECK_RET(service_adapter_hid_device_send_intr_report(addr->addr, rpt_id, rpt_data, rpt_size), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_virtual_unplug(bt_address_t* addr)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_hid_device_virtual_unplug(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif
