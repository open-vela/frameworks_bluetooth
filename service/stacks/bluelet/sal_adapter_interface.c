/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#define LOG_TAG "bluelet"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_adapter.h"
#include "bt_device.h"
#include "bt_status.h"

#include "adapter_internel.h"

#include "hci_h4.h"
#include "sal.h"
#include "service_loop.h"
#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"
#include "stack_adapter_gatt.h"

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
#include "advertising.h"
#include "bt_le_scan.h"
#include "sal_adapter_interface.h"
#include "scan_manager.h"
#endif
#include "utils/log.h"

#define BTSTACK_THREAD_STACK_SIZE 4096
#define DEBUG_IMPL                BT_LOGD("%s", __func__);

static uint8_t sal_pair_type(bt_pair_type_t type)
{
    switch (type) {
    case PAIR_TYPE_PASSKEY_CONFIRMATION:
        return GAP_SPP_TYPE_PASSKEY_CONFIRMATION;
    case PAIR_TYPE_PASSKEY_ENTRY:
        return GAP_SPP_TYPE_PASSKEY_ENTRY;
    case PAIR_TYPE_CONSENT:
        return GAP_SPP_TYPE_CONSENT;
    case PAIR_TYPE_PASSKEY_NOTIFICATION:
        return GAP_SPP_TYPE_PASSKEY_NOTIFICATION;
    default:
        assert(0);
    }

    return 0;
}

static bt_status_t sal_status_translate(uint32_t status)
{
    switch (status) {
    case SERVICE_BT_STATUS_SUCCESS:
        return BT_STATUS_SUCCESS;
    case SERVICE_BT_STATUS_PAGE_TIMEOUT:
        return BT_STATUS_PAGE_TIMEOUT;
    case SERVICE_BT_STATUS_AUTH_FAILURE:
        return BT_STATUS_AUTH_FAILURE;
    case SERVICE_BT_STATUS_AUTH_REJECTED:
        return BT_STATUS_AUTH_REJECTED;
    case SERVICE_BT_STATUS_RMT_DEV_DOWN:
        return BT_STATUS_RMT_DEV_DOWN;
    case SERVICE_BT_STATUS_RMT_DEV_TERMINATE:
        return BT_STATUS_RMT_DEV_TERMINATE;
    case SERVICE_BT_STATUS_LOCAL_TERMINATED:
        return BT_STATUS_LOCAL_TERMINATED;
    case SERVICE_BT_STATUS_FAIL:
        return BT_STATUS_FAIL;
    case SERVICE_BT_STATUS_NOT_READY:
        return BT_STATUS_NOT_ENABLED;
    case SERVICE_BT_STATUS_NOMEM:
        return BT_STATUS_NOMEM;
    case SERVICE_BT_STATUS_BUSY:
        return BT_STATUS_BUSY;
    case SERVICE_BT_STATUS_DONE:
        return BT_STATUS_SERVICE_NOT_FOUND;
    case SERVICE_BT_STATUS_UNSUPPORTED:
        return BT_STATUS_NOT_SUPPORTED;
    case SERVICE_BT_STATUS_PARM_INVALID:
        return BT_STATUS_PARM_INVALID;
    case SERVICE_BT_STATUS_UNHANDLED:
        return BT_STATUS_ERROR_BUT_UNKNOWN;
    default:
        return BT_STATUS_ERROR_BUT_UNKNOWN;
    }
}

static void stack_state_changed_callback(SERVICE_BT_STACK_STATE stack_state)
{
    uint8_t state = BT_BREDR_STACK_STATE_OFF;

#if defined(CONFIG_OBELISK_BREDR_BLUELET) && !defined(CONFIG_OBELISK_LE_BLUELET)
    state = stack_state == BT_STATE_ON ? BT_BREDR_STACK_STATE_ON : BT_BREDR_STACK_STATE_OFF;
#else
    switch (adapter_get_state()) {
    case BT_ADAPTER_STATE_BLE_TURNING_ON:
        state = stack_state == BT_STATE_ON ? BLE_STACK_STATE_ON : BLE_STACK_STATE_OFF;
        break;
    case BT_ADAPTER_STATE_TURNING_ON:
        state = stack_state == BT_STATE_ON ? BT_BREDR_STACK_STATE_ON : BT_BREDR_STACK_STATE_OFF;
        break;
    case BT_ADAPTER_STATE_BLE_TURNING_OFF:
        state = BLE_STACK_STATE_OFF;
        break;
    case BT_ADAPTER_STATE_TURNING_OFF:
        state = BT_BREDR_STACK_STATE_OFF;
        break;
    default:
        break;
    }
#endif
    adapter_on_adapter_state_changed(state);
}

static void received_remote_name_callback(BD_ADDR bd_addr, char *bt_name, uint8_t length)
{
    bt_address_t addr;

    memcpy(addr.addr, bd_addr, 6);
    adapter_on_remote_name_recieved(&addr, bt_name);
}

static void device_found_callback(SERVICE_REMOTE_DEVICE_S *device)
{
    bt_discovery_result_t result;

    /* latency discovery with rssi */
    memcpy(result.addr.addr, device->bd_addr, 6);
    result.cod = device->cod;
    result.rssi = device->rssi;

    /* EIR name */
    strncpy(result.name, device->bt_name, BT_REM_NAME_MAX_LEN);
    /* EIR uuids */
    // TODO
    adapter_on_device_found(&result);
}

static void discovery_state_changed_callback(SERVICE_BT_DISCOVERY_STATE state)
{
    adapter_on_discovery_state_changed(state == BT_DISCOVERY_STARTED ? BT_DISCOVERY_STATE_STARTED : BT_DISCOVERY_STATE_STOPPED);
}

static void link_connect_request_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_connect_request(&addr);
}

static void acl_state_changed_callback(SERVICE_ACL_STATE_PARAM_S *acl_state_param)
{
    acl_state_param_t param;

    memcpy(&param.addr.addr, acl_state_param->remote_addr, 6);
    if (acl_state_param->state < SERVICE_BT_ACL_STATE_LE_CONNECTED || acl_state_param->state == SERVICE_BT_ACL_STATE_BR_BONDED_FULL)
        param.link_type = BT_TRANSPORT_BREDR;
    else {
        param.link_type = BT_TRANSPORT_BLE;
        param.addr_type = acl_state_param->addr_type;
    }

    param.connection_state = CONNECTION_STATE_DISCONNECTED;
    if (acl_state_param->state == SERVICE_BT_ACL_STATE_CONNECTED || acl_state_param->state == SERVICE_BT_ACL_STATE_LE_CONNECTED)
        param.connection_state = CONNECTION_STATE_CONNECTED;
    else if (acl_state_param->state == SERVICE_BT_ACL_STATE_CONNECTING || acl_state_param->state == SERVICE_BT_ACL_STATE_CONNECT_REQUEST || acl_state_param->state == SERVICE_BT_ACL_STATE_LE_CONNECTING)
        param.connection_state = CONNECTION_STATE_CONNECTING;

    param.hci_reason_code = acl_state_param->reasonCode;
    param.status = sal_status_translate(acl_state_param->status);
    adapter_on_connection_state_changed(&param);
}

static void pairing_request_callback(BD_ADDR remote_addr, bool local_initiate,
                                     bool is_bondable)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_pairing_request(&addr, local_initiate, is_bondable);
}

static void pin_request_callback(SERVICE_PIN_REQUEST_DATA_S *request_data)
{
    bt_address_t addr;

    memcpy(addr.addr, request_data->remote_addr, 6);
    adapter_on_pin_request(&addr, request_data->cod,
                           request_data->min_16_digit,
                           (const char *)request_data->bt_name);
}

static void pair_authentication_request(SERVICE_SSP_REQUEST_DATA_S *request_data, uint8_t transport)
{
    bt_address_t addr;
    bt_pair_type_t type;

    memcpy(addr.addr, request_data->remote_addr, 6);
    switch (request_data->ssp_type) {
    case GAP_SPP_TYPE_PASSKEY_CONFIRMATION:
        type = PAIR_TYPE_PASSKEY_CONFIRMATION;
        break;
    case GAP_SPP_TYPE_PASSKEY_ENTRY:
        type = PAIR_TYPE_PASSKEY_ENTRY;
        break;
    case GAP_SPP_TYPE_CONSENT:
        type = PAIR_TYPE_CONSENT;
        break;
    case GAP_SPP_TYPE_PASSKEY_NOTIFICATION:
        type = PAIR_TYPE_PASSKEY_NOTIFICATION;
        break;
    default:
        return;
    }

    adapter_on_ssp_request(&addr, transport, request_data->cod,
                           type, request_data->pass_key,
                           (const char *)request_data->bt_name);
}

static void ssp_request_callback(SERVICE_SSP_REQUEST_DATA_S *request_data)
{
    pair_authentication_request(request_data, BT_TRANSPORT_BREDR);
}

static void bond_state_changed_callback(BD_ADDR remote_addr, SERVICE_BT_BOND_STATE state)
{
    bt_address_t addr;
    bond_state_t bond_state;
    uint8_t link_type = BT_TRANSPORT_BLE;

    memcpy(addr.addr, remote_addr, 6);
    if (state < SERVICE_BT_BOND_STATE_BLE_NONE)
        link_type = BT_TRANSPORT_BREDR;

    bond_state = BOND_STATE_NONE;
    if (state == SERVICE_BT_BOND_STATE_BONDED || state == SERVICE_BT_BOND_STATE_BLE_BONDED)
        bond_state = BOND_STATE_BONDED;
    else if (state == SERVICE_BT_BOND_STATE_BONDING || state == SERVICE_BT_BOND_STATE_BLE_BONDING)
        bond_state = BOND_STATE_BONDING;
    else if (state == SERVICE_BT_BOND_STATE_SDP_DONE) {
        /* had bonded, ignore it*/
        return;
    }

    adapter_on_bond_state_changed(&addr, bond_state, link_type);
}

static void ble_scan_result_callback(SERVICE_SCAN_RESULT_DATA_S *scan_result_data)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    ble_scan_result_t result;

    memcpy(result.addr.addr, scan_result_data->remote_addr, 6);
    result.dev_type = scan_result_data->device_type;
    result.rssi = scan_result_data->rssi;
    result.addr_type = scan_result_data->addr_type;
    result.adv_type = scan_result_data->evt_type;
    result.length = scan_result_data->length;

    scan_on_result_data_update(&result, scan_result_data->adv_data);
#endif
}

static void ble_adv_started_callback(uint8_t adv_id)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    advertising_on_state_changed(adv_id, LE_ADVERTISING_STARTED);
#endif
}

static void ble_adv_stopped_callback(uint8_t adv_id)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    advertising_on_state_changed(adv_id, LE_ADVERTISING_STOPPED);
#endif
}

static void bt_link_role_changed_callback(BD_ADDR remote_addr, SERVICE_BT_LINK_ROLE link_role)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_link_role_changed(&addr, link_role);
}

static void scan_mode_changed_callback(SERVICE_BT_SCAN_MODE scan_mode)
{
    adapter_on_scan_mode_changed(scan_mode);
}

static void link_mode_changed_callback(BD_ADDR remote_addr, SERVICE_BT_LINK_MODE link_mode,
                                       uint16_t sniff_interval)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_link_mode_changed(&addr, link_mode, sniff_interval);
}

static void link_policy_changed_callback(BD_ADDR remote_addr,
                                         SERVICE_BT_LINK_POLICY link_policy)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_link_policy_changed(&addr, link_policy);
}

static void bt_hci_event_callback(SERVICE_BT_HCI_EVENT_S *hci_event) { DEBUG_IMPL }

static void transport_write_packet_callback(uint8_t *hci_packet, uint32_t length)
{
    assert(hci_packet);

    bt_sal_hci_send_packet(hci_packet, length);
}

static void update_br_link_key_callback(SERVICE_REMOTE_DEVICE_S *bonded_device)
{
    bt_address_t addr;

    memcpy(addr.addr, bonded_device->bd_addr, 6);
    adapter_on_link_key_update(&addr, bonded_device->link_key, bonded_device->link_key_type);
}

static void delete_br_link_key_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS reason)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_link_key_removed(&addr, sal_status_translate(reason));
}

static void service_discovered_callback(BD_ADDR remote_addr, SERVICE_BR_SERVICE_S *services,
                                        uint16_t size)
{
    bt_uuid_t uuids[size];
    bt_address_t addr;
    SERVICE_BR_SERVICE_S *sinfo;

    memcpy(addr.addr, remote_addr, 6);
    sinfo = services;
    for (int i = 0; i < size; i++) {
        bt_uuid128_create(&uuids[i], sinfo->uuid);
        sinfo++;
    }

    adapter_on_service_search_done(&addr, uuids, size);
}

static void link_encryption_state_callback(BD_ADDR remote_addr, bool br_link, bool encryption_on)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_encryption_state_changed(&addr, encryption_on,
                                        br_link ? BT_TRANSPORT_BREDR : BT_TRANSPORT_BLE);
}

static void smp_request_callback(SERVICE_SSP_REQUEST_DATA_S *request_data)
{
    pair_authentication_request(request_data, BT_TRANSPORT_BLE);
}

static void update_ble_bonded_devices_callback(SERVICE_BLE_KEYS_S *bonded_device_list,
                                               uint8_t count_in) { DEBUG_IMPL }
static void ble_add_white_list_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status) { DEBUG_IMPL }
static void ble_remove_white_list_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status) { DEBUG_IMPL }
static void ble_add_resolving_list_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status) { DEBUG_IMPL }
static void ble_remove_resolving_list_callback(BD_ADDR remote_addr,
                                               SERVICE_BT_STATUS status) { DEBUG_IMPL }

static void ble_address_callback(BD_ADDR ble_addr, SERVICE_BLE_ADDR_TYPE ble_addr_type)
{
    bt_address_t addr;

    memcpy(addr.addr, ble_addr, 6);
    adapter_on_le_addr_update(&addr, ble_addr_type);
}

static void ble_phy_update_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
                                    SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_BT_STATUS status)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    adapter_on_le_phy_update(&addr, tx_phy, rx_phy, sal_status_translate(status));
}

static void ble_irk_callback(BT_COMMON_KEY irk, BD_ADDR ble_addr,
                             SERVICE_BLE_ADDR_TYPE ble_addr_type) { DEBUG_IMPL }

static void ble_packet_received_callback(BD_ADDR remote_addr, uint16_t private_cid,
                                         uint8_t *packet, uint16_t packet_size) { DEBUG_IMPL }
static void ssp_local_oob_data_callback(BT_COMMON_KEY c_192_val, BT_COMMON_KEY r_192_val,
                                        BT_COMMON_KEY c_256_val, BT_COMMON_KEY r_256_val) { DEBUG_IMPL }
static void ble_local_oob_data_callback(BD_ADDR remote_addr, BT_COMMON_KEY c_val,
                                        BT_COMMON_KEY r_val) { DEBUG_IMPL }

static void ble_scan_started_callback(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    scan_on_state_changed(SCAN_STATE_STARTED);
#endif
}

static void ble_scan_stopped_callback(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    scan_on_state_changed(SCAN_STATE_STOPPED);
#endif
}

static void ble_connection_updated_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status,
                                            uint16_t connection_interval, uint16_t peripheral_latency,
                                            uint16_t supervision_timeout) { DEBUG_IMPL }

static const GAP_CALLBACKS_S sal_gap_callbacks = {
    .size = sizeof(sal_gap_callbacks),
    /* done */
    .gap_stack_state_changed_cb = stack_state_changed_callback,
    /* done */
    .gap_received_remote_name_cb = received_remote_name_callback,
    /* done */
    .gap_device_found_cb = device_found_callback,
    /* done */
    .gap_discovery_state_changed_cb = discovery_state_changed_callback,
    /* done */
    .gap_pin_request_cb = pin_request_callback,
    /* done */
    .gap_ssp_request_cb = ssp_request_callback,
    /* done */
    .gap_bond_state_changed_cb = bond_state_changed_callback,
    /* done */
    .gap_acl_state_changed_cb = acl_state_changed_callback,
    /* done */
    .gap_ble_scan_result_cb = ble_scan_result_callback,
    /* done */
    .gap_ble_adv_started_cb = ble_adv_started_callback,
    /* done */
    .gap_ble_adv_stopped_cb = ble_adv_stopped_callback,
    /* done */
    .gap_link_connect_request_cb = link_connect_request_callback,
    /* done */
    .gap_bt_link_role_changed_cb = bt_link_role_changed_callback,
    /* done */
    .gap_scan_mode_changed_cb = scan_mode_changed_callback,
    /* done */
    .gap_link_mode_changed_cb = link_mode_changed_callback,
    /* done */
    .gap_link_policy_changed_cb = link_policy_changed_callback,
    .gap_hci_event_cb = bt_hci_event_callback,
    /* done */
    .gap_transport_write_packet_cb = transport_write_packet_callback,
    /* don't implement this callback */
    .gap_init_done_cb = NULL,
    /* done */
    .gap_update_br_link_key_cb = update_br_link_key_callback,
    /* done */
    .gap_delete_br_link_key_cb = delete_br_link_key_callback,
    /* done */
    .gap_pairing_request_cb = pairing_request_callback,
    /* done */
    .gap_service_discovered_cb = service_discovered_callback,
    /* done */
    .gap_link_encryption_state_cb = link_encryption_state_callback,
    /* LE event field */
    /* done */
    .gap_smp_request_cb = smp_request_callback,
    .gap_update_ble_bonded_devices_cb = update_ble_bonded_devices_callback,
    /* ignore */
    .gap_ble_add_white_list_cb = ble_add_white_list_callback,
    .gap_ble_remove_white_list_cb = ble_remove_white_list_callback,
    .gap_ble_add_resolving_list_cb = ble_add_resolving_list_callback,
    .gap_ble_remove_resolving_list_cb = ble_remove_resolving_list_callback,
    /*  */
    .gap_ble_address_cb = ble_address_callback,
    .gap_ble_phy_update_cb = ble_phy_update_callback,
    .gap_ble_irk_cb = ble_irk_callback,
    .gap_ble_packet_received_cb = ble_packet_received_callback,
    /* don't implement this callback */
    .gap_local_name_set_cb = NULL,
    .gap_ssp_local_oob_data_cb = ssp_local_oob_data_callback,
    .gap_ble_local_oob_data_cb = ble_local_oob_data_callback,
    /* done */
    .gap_ble_scan_started_cb = ble_scan_started_callback,
    /* done */
    .gap_ble_scan_stopped_cb = ble_scan_stopped_callback,
    .gap_ble_connection_updated_cb = ble_connection_updated_callback,
};

static bool stack_initialized = false;
static int hci_fd = -1;
static service_poll_t *hci_handle;

/* classics bluetooth*/
static void hci_remove_recv(void *data)
{
    (void)data;

    BT_LOGD("%s", __func__);
    service_loop_remove_poll(hci_handle);
    hci_handle = NULL;
}

static void hci_poll_recv(service_poll_t *poll, int revent, void *userdata)
{
    (void)poll;
    (void)userdata;

    if (revent & (POLL_ERROR | POLL_DISCONNECT))
        hci_remove_recv(NULL);

    if (revent & POLL_READABLE)
        bt_sal_hci_transport_recv();
}

static void hci_add_recv(void *data)
{
    (void)data;

    BT_LOGD("%s", __func__);
    hci_handle = service_loop_poll_fd(hci_fd, POLL_READABLE, hci_poll_recv, NULL);
    if (!hci_handle) {
        BT_LOGD("hci fd:%d add poll failed", hci_fd);
        assert(0);
    }
}

static void *stack_schedule_loop(void *data)
{
    extern int ScheduleLoop(void);
    assert(data);

    sem_post((sem_t *)data);
    ScheduleLoop();
    BT_LOGD("%s quit", __func__);

    return NULL;
}

static bt_status_t bluelet_stack_init(void)
{
    pthread_attr_t pattr;
    pthread_t thread_id;
    sem_t startup;
    bt_status_t status;

    if (stack_initialized)
        return BT_STATUS_SUCCESS;

    /* Open hci uart driver */
    hci_fd = bt_sal_hci_transport_init();
    if (hci_fd < 0)
        return BT_STATUS_FAIL;

    /* Register gap callbacks and initialize bluelet stack */
    service_adapter_debug_init();
    service_adapter_gap_init();
    service_adapter_gap_register_gap_callback((GAP_CALLBACKS_S *)&sal_gap_callbacks);

    sem_init(&startup, 0, 0);
    pthread_attr_init(&pattr);
    pthread_attr_setstacksize(&pattr, BTSTACK_THREAD_STACK_SIZE);

    /* Create bluelet stack schedule thread */
    if (pthread_create(&thread_id, &pattr, stack_schedule_loop, &startup) != 0) {
        service_adapter_gap_cleanup();
        bt_sal_hci_transport_cleanup();
        BT_LOGE("%s pthread_create : %s", __func__, strerror(errno));
        status = BT_STATUS_FAIL;
    } else {
        pthread_setname_np(thread_id, "bluelet_thread");
        sem_wait(&startup);

        /* action polling start should do in service loop*/
        add_init_process(hci_add_recv);
        status = BT_STATUS_SUCCESS;
    }

    pthread_attr_destroy(&pattr);
    sem_destroy(&startup);
    stack_initialized = true;

    return status;
}

static void bluelet_stack_cleanup(void)
{
    if (!stack_initialized)
        return;

    stack_initialized = false;
    service_adapter_gap_cleanup();
    do_in_service_loop_sync(hci_remove_recv, NULL);
    bt_sal_hci_transport_cleanup();
    hci_fd = -1;
}

// #ifdef CONFIG_OBELISK_BREDR_BLUELET

bt_status_t bt_sal_init(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    return bluelet_stack_init();
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    bluelet_stack_cleanup();
#endif
}

bt_status_t bt_sal_enable(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (service_adapter_gap_get_stack_state() == BT_STATE_ON) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_ON);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(service_adapter_gap_enable(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_disable(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (service_adapter_gap_get_stack_state() == BT_STATE_OFF) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(service_adapter_gap_disable(true), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_local_name(char *name)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(name);
    SAL_CHECK_RET(service_adapter_gap_set_local_name(name, strlen(name) + 1), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_get_local_address(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_get_local_address(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_local_io_capability(bt_io_capability_t cap)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_set_local_io_capability((SERVICE_BT_IO_CAPABILITY)cap),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

const char *bt_sal_get_local_name(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    static char adapter_local_name[BT_LOC_NAME_MAX_LEN + 1];
    char *pname = adapter_local_name;

    memset(pname, 0, BT_LOC_NAME_MAX_LEN + 1);
    service_adapter_gap_get_local_name(&pname, BT_LOC_NAME_MAX_LEN);

    return (const char *)pname;
#else
    return NULL;
#endif
}

bt_status_t bt_sal_set_local_device_class(uint32_t cod)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_set_local_device_class(cod), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t bt_sal_get_local_device_class(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    uint32_t cod = 0;

    service_adapter_gap_get_local_device_class(&cod);

    return cod;
#else
    return 0;
#endif
}

bt_status_t bt_sal_set_scan_mode(bt_scan_mode_t scan_mode, bool bondable)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_set_scan_mode(scan_mode, bondable), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_discovery_filter(bt_discovery_filter_t *filter)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_ASSERT_PARAM(filter != NULL);

    SERVICE_BT_EVENT_FILTER_S set_filter;

    set_filter.filter_type = BT_EVENT_FILTER_INQUIRY_RESULT;
    switch (filter->condition_type) {
    case BT_BR_FILTER_CONDITION_ALL_DEVICES:
        set_filter.condition_type = BT_EVENT_FILTER_CONDITION_ALL_DEVICES;
        break;
    case BT_BR_FILTER_CONDITION_DEVICE_CLASS:
        set_filter.condition_type = BT_EVENT_FILTER_CONDITION_DEVICE_CLASS;
        set_filter.condition_device_class = filter->condition_cod;
        break;
    case BT_BR_FILTER_CONDITION_BDADDR:
        set_filter.condition_type = BT_EVENT_FILTER_CONDITION_BDADDR;
        memcpy(&set_filter.condition_bdaddr, filter->condition_bdaddr.addr, BT_ADDR_LENGTH);
        break;
    case BT_BR_FILTER_CONDITION_RSSI:
    case BT_BR_FILTER_CONDITION_UUIDS:
    default:
        return BT_STATUS_NOT_SUPPORTED;
    }
    set_filter.auto_accept_flag = BT_CONNECTION_AUTO_ACCEPT_OFF;
    SAL_CHECK_RET(service_adapter_gap_set_event_filter(&set_filter), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_start_discovery(uint32_t timeout)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(timeout)
    SAL_CHECK_RET(service_adapter_gap_start_device_discovery(timeout), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_stop_discovery(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_stop_device_discovery(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_page_scan_parameters(bt_scan_type_t type,
                                            uint16_t interval,
                                            uint16_t window)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_set_page_scan_parameters((SERVICE_BT_SCAN_TYPE)type, interval, window),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_inquiry_scan_parameters(bt_scan_type_t type,
                                               uint16_t interval,
                                               uint16_t window)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_set_inquiry_scan_parameters((SERVICE_BT_SCAN_TYPE)type, interval, window),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* Remote device */
bt_status_t bt_sal_get_remote_name(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_get_remote_name(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* useless, remove ? */
int bt_sal_get_remote_services(bt_address_t *addr, bt_uuid_t *uuids, uint8_t count_in)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(uuids);
    BT_UUID_T service_list[count_in];
    SAL_CHECK_RET(service_adapter_gap_get_remote_services(addr->addr, service_list, count_in),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_reply_sco_link_request(bt_address_t *addr, bool accept)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);

    if (accept) {
        SAL_CHECK_RET(service_adapter_gap_accept_sco_link(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    } else {
        SAL_CHECK_RET(service_adapter_gap_reject_sco_link(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    }

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_reply_link_request(bt_address_t *addr, bool accept)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);

    /* with link role
     service_adapter_gap_accept_link();
     service_adapter_gap_reject_link();
    */

    SAL_CHECK_RET(service_adapter_gap_reply_link_request(addr->addr, accept), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_reply_pair_request(bt_address_t *addr, uint8_t reason)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    service_adapter_gap_reply_pairing_request(addr->addr, reason);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_ssp_reply(bt_address_t *addr,
                             bool accept,
                             bt_pair_type_t type,
                             uint32_t passkey)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SERVICE_SSP_REPLY_DATA_S reply;

    memcpy(reply.remote_addr, addr->addr, sizeof(reply.remote_addr));
    reply.accept = accept;
    reply.type = sal_pair_type(type);
    reply.passkey = passkey;
    SAL_CHECK_RET(service_adapter_gap_ssp_reply(&reply), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_pin_reply(bt_address_t *addr,
                             bool accept,
                             char *pincode,
                             int len)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_pin_reply(addr->addr, accept, pincode, len), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

uint16_t bt_sal_get_acl_link_handle(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    return service_adapter_gap_get_acl_handle(addr->addr);
#else
    return 0;
#endif
}

bt_status_t bt_sal_connect(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_reconnect_link(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_disconnect(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_disconnect_link(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_create_bond(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_create_bond(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_cancel_bond(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_cancel_bond(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_remove_bond(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_remove_bond(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_ssp_set_remote_oob_data(bt_address_t *addr,
                                           bt_128key_t c_192_val, bt_128key_t r_192_val,
                                           bt_128key_t c_256_val, bt_128key_t r_256_val)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ssp_set_remote_oob_data(addr->addr, c_192_val, r_192_val, c_256_val, r_256_val),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_ssp_get_local_oob_data(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ssp_get_local_oob_data(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bluelet_set_remote_property(remote_device_properties_t *prop,
                                 SERVICE_REMOTE_DEVICE_S *remote)
{
    /* address */
    memcpy(prop->addr.addr, remote->bd_addr, 6);
    /* only LE device address use */
    prop->addr_type = remote->addr_type;
    /* name */
    strncpy(prop->name, remote->bt_name, BT_REM_NAME_MAX_LEN);
    /* uuid */
    // TODO: add uuid support
    /* link key */
    memcpy(prop->link_key, remote->link_key, 16);
    prop->link_key_type = remote->link_key_type;
    /* cod */
    prop->class_of_device = remote->cod;
    /* device type */
    prop->device_type = remote->device_type;
}

bt_status_t bt_sal_get_remote_device_info(bt_address_t *addr, remote_device_properties_t *prop)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(prop);
    SERVICE_REMOTE_DEVICE_S remote;

    SAL_CHECK_RET(service_adapter_gap_get_remote_device_info(addr->addr, &remote),
                  SERVICE_BT_STATUS_SUCCESS);
    bluelet_set_remote_property(prop, &remote);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_bonded_devices(remote_device_properties_t *prop)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(prop);
    SERVICE_REMOTE_DEVICE_S remote;

    memcpy(remote.bd_addr, prop->addr.addr, 6);
    strncpy(remote.bt_name, prop->name, BT_DEVICE_NAME_MAX_LEN);
    memcpy(remote.link_key, prop->link_key, 16);
    remote.link_key_type = prop->link_key_type;
    remote.cod = prop->class_of_device;
    remote.device_type = prop->device_type;

    SAL_CHECK_RET(service_adapter_gap_set_bonded_device(&remote), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* useless */
bt_status_t bt_sal_get_bonded_devices(remote_device_properties_t *props, int *cnt)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(props);
    SAL_CHECK_PARAM(cnt);
    remote_device_properties_t **prop = (remote_device_properties_t **)props;
    SERVICE_REMOTE_DEVICE_S remotes[*cnt];

    *cnt = service_adapter_gap_get_bonded_devices(remotes, *cnt);
    for (int i = 0; i < *cnt; i++)
        bluelet_set_remote_property(prop[i], &remotes[i]);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* useless */
bt_status_t bt_sal_get_connected_devices(remote_device_properties_t *props, int *cnt)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(props);
    SAL_CHECK_PARAM(cnt);
    remote_device_properties_t **prop = (remote_device_properties_t **)props;
    SERVICE_REMOTE_DEVICE_S remotes[*cnt];

    *cnt = service_adapter_gap_get_connected_devices(remotes, *cnt);
    for (int i = 0; i < *cnt; i++)
        bluelet_set_remote_property(prop[i], &remotes[i]);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_start_service_discovery(bt_address_t *addr, bt_uuid_t *uuid)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_start_service_discovery(addr->addr, uuid ? uuid->val.u128 : NULL),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_stop_service_discovery(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_stop_service_discovery(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_link_role(bt_address_t *addr, bt_link_role_t role)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_set_link_role(addr->addr, role), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_link_mode(bt_address_t *addr,
                                 bt_link_mode_t mode,
                                 bt_sniff_params_t *param)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(param);
    SERVICE_BT_SNIFF_PARAM_S set_param;

    set_param.sniff_interval = param->sniff_interval;
    set_param.sniff_attempt = param->sniff_attempt;
    set_param.sniff_timeout = param->sniff_timeout;
    SAL_CHECK_RET(service_adapter_gap_set_link_mode(addr->addr, mode, &set_param), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_afh_channel_classification(uint16_t central_frequency,
                                                  uint16_t band_width,
                                                  uint16_t number)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    SERVICE_RADIO_CHANNEL_INFO_S channel_info = { central_frequency, band_width };

    SAL_CHECK_RET(service_adapter_set_afh_channel_classification(&channel_info, number),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* BLE */

//#if defined(CONFIG_BLUETOOTH_BLE_SUPPORT) && defined(CONFIG_OBELISK_LE_BLUELET)
// #ifdef CONFIG_OBELISK_LE_BLUELET

bt_status_t bt_sal_le_init(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    return bluelet_stack_init();
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_le_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    bluelet_stack_cleanup();
#endif
}

bt_status_t bt_sal_le_enable(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    if (service_adapter_gap_get_stack_state() == BT_STATE_ON) {
        adapter_on_adapter_state_changed(BLE_STACK_STATE_ON);
        return BT_STATUS_SUCCESS;
    }

    if (service_adapter_gap_enable() != SERVICE_BT_STATUS_SUCCESS)
        return BT_STATUS_FAIL;

    if (service_adapter_gatt_init() != GATT_SUCCESS)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_disable(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    if (service_adapter_gap_get_stack_state() == BT_STATE_OFF) {
        adapter_on_adapter_state_changed(BLE_STACK_STATE_OFF);
        return BT_STATUS_SUCCESS;
    }

    if (service_adapter_gap_disable(true) != SERVICE_BT_STATUS_SUCCESS)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_io_capability(bt_io_capability_t cap)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_set_local_io_capability((SERVICE_BT_IO_CAPABILITY)cap),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BLE_SCAN
bt_status_t bt_sal_le_set_scan_parameters(ble_scan_params_t *params)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(params);
    SERVICE_SCAN_PARAMS_S scan_params;

    scan_params.scan_interval = params->scan_interval;
    scan_params.scan_window = params->scan_window;
    scan_params.scan_phy = params->scan_phy;
    SAL_CHECK_RET(service_adapter_gap_set_ble_scan_parameters(&scan_params), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* maybe implement it in scan service */
bt_status_t bt_sal_le_set_scan_filters(ble_scan_filter_t *filter)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(filter);
    SERVICE_BLE_SCAN_FILTER_S *scan_filter = malloc(sizeof(SERVICE_BLE_SCAN_FILTER_S) + filter->length);

    if (scan_filter == NULL)
        return BT_STATUS_NOMEM;

    memcpy(scan_filter->bd_addr, filter->addr.addr, 6);
    scan_filter->length = filter->length;
    memcpy(scan_filter->adv_data_mask, filter->adv_data_mask, filter->length);
    if (service_adapter_gap_set_ble_scan_filter(scan_filter) != SERVICE_BT_STATUS_SUCCESS) {
        free(scan_filter);
        return BT_STATUS_FAIL;
    }
    free(scan_filter);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_start_scan(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_start_ble_scan(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_stop_scan(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_stop_ble_scan(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}
#endif

#ifdef CONFIG_BLUETOOTH_BLE_ADV
bt_status_t bt_sal_le_start_adv(uint8_t adv_id,
                                ble_adv_params_t *params,
                                uint8_t *adv_data,
                                uint16_t adv_len,
                                uint8_t *scan_rsp_data,
                                uint16_t scan_rsp_len)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(params);
    SERVICE_SCAN_ADV_PARAMS_S sap;

    sap.adv_id = adv_id;
    sap.params.adv_type = params->adv_type;
    memcpy(sap.params.peer_addr, params->peer_addr.addr, 6);
    sap.params.peer_addr_type = params->peer_addr_type;
    memcpy(sap.params.own_addr, params->own_addr.addr, 6);
    sap.params.own_addr_type = params->own_addr_type;
    sap.params.interval = params->interval;
    sap.params.tx_power = params->tx_power;
    sap.params.channel_map = params->channel_map;
    sap.params.filter_policy = params->filter_policy;
    sap.adv_data = (char *)adv_data;
    sap.adv_length = adv_len;
    sap.scan_rsp_data = (char *)scan_rsp_data;
    sap.scan_rsp_length = scan_rsp_len;
    sap.duration = params->duration;

    SAL_CHECK_RET(service_adapter_gap_start_ble_adv(&sap), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_stop_adv(uint8_t adv_id)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_stop_ble_adv(adv_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}
#endif

int bt_sal_get_le_bonded_devices(void)
{
    return BT_STATUS_NOT_SUPPORTED;
    // ret = service_adapter_gap_ble_get_bonded_devices(bonded_list, MAX_PAIR_DEVICE);
}

int bt_sal_get_le_connected_devices(void)
{
    // ret = service_adapter_gap_ble_get_connected_devices(connected_list, max_out);
    return BT_STATUS_NOT_SUPPORTED;
}

int bt_sal_get_le_whitelist_devices(void)
{
    // ret = service_adapter_gap_ble_get_white_list_devices(whitelist_list, MAX_PAIR_DEVICE);
    return BT_STATUS_NOT_SUPPORTED;
}

int bt_sal_get_le_resolvinglist_devices(void)
{
    // ret = service_adapter_gap_ble_get_resolving_list_devices(resolvinglist_list, MAX_PAIR_DEVICE);
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_le_set_static_identity(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ble_set_static_identity(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_public_identity(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ble_set_public_identity(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_remote_irk(bt_address_t *addr, ble_addr_type_t type, bt_128key_t irk)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(irk);
    SAL_CHECK_RET(service_adapter_gap_ble_set_remote_irk(addr->addr, type, irk),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_get_current_irk(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ble_get_current_irk(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_address(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_set_address(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_get_address(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ble_get_address(), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_bonded_devices(void)
{
    // SERVICE_BT_STATUS ret = service_adapter_gap_ble_set_bonded_devices((SERVICE_BLE_KEYS_S*)bonded_device_list, count_in);
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_le_connect(bt_address_t *addr,
                              ble_addr_type_t type,
                              ble_connect_params_t *params)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(params);
    SERVICE_LE_CONNECT_PARAMS_S conn_param;

    memcpy(conn_param.peer_addr, addr->addr, 6);
    conn_param.peer_addr_type = type;
    conn_param.filter_policy = params->filter_policy;
    conn_param.use_default_params = params->use_default_params;
    conn_param.init_phy = params->init_phy;
    conn_param.scan_interval = params->scan_interval;
    conn_param.scan_window = params->scan_window;
    conn_param.connection_interval_min = params->connection_interval_min;
    conn_param.connection_interval_max = params->connection_interval_max;
    conn_param.connection_latency = params->connection_latency;
    conn_param.supervision_timeout = params->supervision_timeout;
    conn_param.min_ce_length = params->min_ce_length;
    conn_param.max_ce_length = params->max_ce_length;

    SAL_CHECK_RET(service_adapter_gap_ble_connect(&conn_param), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_disconnect(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_disconnect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_create_bond(bt_address_t *addr, ble_addr_type_t type)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SERVICE_LE_CONNECT_PARAMS_S conn_param;

    memcpy(conn_param.peer_addr, addr->addr, 6);
    conn_param.peer_addr_type = type;
    conn_param.use_default_params = true;

    SAL_CHECK_RET(service_adapter_gap_ble_connect(&conn_param), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_remove_bond(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_remove_bond(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_smp_reply(bt_address_t *addr,
                                bool accept,
                                bt_pair_type_t type,
                                uint32_t passkey)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SERVICE_SSP_REPLY_DATA_S reply_data;

    memcpy(reply_data.remote_addr, addr->addr, 6);
    reply_data.accept = accept;
    reply_data.type = sal_pair_type(type);
    reply_data.passkey = passkey;
    SAL_CHECK_RET(service_adapter_gap_ble_smp_reply(&reply_data), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_remote_oob_data(bt_address_t *addr,
                                          bt_128key_t tk_val,
                                          bt_128key_t c_val,
                                          bt_128key_t r_val)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_set_remote_oob_data(addr->addr, tk_val, c_val, r_val),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_get_local_oob_data(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_get_local_oob_data(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_add_white_list(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_add_white_list(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_remove_white_list(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_remove_white_list(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_add_resolving_list(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_add_resolving_list(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_remove_resolving_list(bt_address_t *addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_remove_resolving_list(addr->addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_phy(bt_address_t *addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_set_phy(addr->addr, tx_phy, rx_phy),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_add_private_channel(uint16_t private_cid)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ble_add_private_channel(private_cid),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_send_packet(bt_address_t *addr,
                                  uint16_t private_cid,
                                  uint8_t *packet,
                                  uint16_t packet_size)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gap_ble_send_packet(addr->addr, private_cid, packet, packet_size),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_le_set_appearance(uint16_t appearance)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ble_set_local_appearance(appearance), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

uint16_t bt_sal_le_get_appearance(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    uint16_t appearance;
    SAL_CHECK_RET(service_adapter_gap_ble_get_local_appearance(&appearance), SERVICE_BT_STATUS_SUCCESS);

    return appearance;
#else
    return 0;
#endif
}

bt_status_t bt_sal_le_enable_key_derivation(bool brkey_to_lekey,
                                            bool lekey_to_brkey)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    SAL_CHECK_RET(service_adapter_gap_ble_enable_key_derivation(brkey_to_lekey, lekey_to_brkey),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* HCI VSC command */
bt_status_t bt_sal_send_hci_command(uint8_t ogf, uint16_t ocf, uint8_t length, uint8_t *buf)
{
    SERVICE_HCI_COMMAND_S *command = malloc(sizeof(SERVICE_HCI_COMMAND_S) + length);

    if (command == NULL)
        return BT_STATUS_NOMEM;

    command->ogf = ogf;
    command->ocf = ocf;
    command->cb = bt_hci_event_callback;
    command->length = length;
    memcpy(command->params, buf, length);
    if (service_adapter_gap_send_hci_command(command) != SERVICE_BT_STATUS_SUCCESS) {
        free(command);
        return BT_STATUS_FAIL;
    }
    free(command);

    return BT_STATUS_SUCCESS;
}
//#endif

#if 0
/* Test */
bt_status_t bt_sal_enter_bluetooth_test_mode(test_mode mode)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_enter_bluetooth_test_mode((SERVICE_BT_TEST_MODE)mode);
}
#endif
