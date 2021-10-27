/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/common.c
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "btdatatype.h"
#include "global.h"
#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"

#include "btm_manager.h"
#include "bts_service.h"
#include "bts_gap.h"
#include "bts_leadv.h"

#define LOG_TAG "bts_gap"
#include "log.h"

bts_gap_callback_t *gap_callback_cb = NULL;
void bts_common_register_callback(bts_gap_callback_t *cb)
{
    gap_callback_cb = cb;
}

/*process callback from stack */
void process_loop_in_common(void *data, size_t data_size)
{
    //int common_size = 0;
    //char *common_buff = NULL;

    // if (NULL == context)
    // {
    //     return;
    // }

    // if(context->command_id > COMMON_COMMAND_MAX_ID)
    // {
    //     switch (context->command_id)
    //     {
    //     case INIT_SERVICE_DONE_RESPONSE:

    //         gap_callback_cb->gap_init_done_cb();

    //         break;
    //     default:
    //         break;

    free(data);
}

void adapter_device_found_callback(SERVICE_REMOTE_DEVICE_S *device)
{
}
void adapter_received_remote_name_callback(BD_ADDR bd_addr, char *bt_name, uint8_t length)
{
}

void adapter_discovery_state_changed_callback(SERVICE_BT_DISCOVERY_STATE state)
{
}

void adapter_pin_request_callback(SERVICE_PIN_REQUEST_DATA_S *request_data)
{
}

void adapter_ssp_request_callback(SERVICE_SSP_REQUEST_DATA_S *request_data)
{
    if (request_data->ssp_type == GAP_SPP_TYPE_PASSKEY_CONFIRMATION)
    {
        SERVICE_SSP_REPLY_DATA_S reply;
        memcpy(reply.remote_addr, request_data->remote_addr, 6);
        reply.accept = TRUE;
        reply.type = GAP_SPP_TYPE_PASSKEY_CONFIRMATION;
        service_adapter_gap_ssp_reply(&reply);
    }
}

void adapter_bond_state_changed_callback(BD_ADDR remote_addr, SERVICE_BT_BOND_STATE state)
{
}

void adapter_acl_state_changed_callback(SERVICE_ACL_STATE_PARAM_S *acl_state_param)
{
}

void adapter_ble_scan_result_callback(SERVICE_SCAN_RESULT_DATA_S *scan_result_data)
{
}

void adapter_ble_adv_started_callback(uint8_t adv_id)
{
    BT_LOGD("%s, adv_id:%d", __func__, adv_id);
    gatt_advertise_interface_t* adv_ift = get_ble_advertise_instance();
    BT_CBACK(adv_ift->callbacks, ble_advtise_started_cb, adv_id);
}

void adapter_ble_adv_stopped_callback(uint8_t adv_id)
{
    BT_LOGD("%s, adv_id:%d", __func__, adv_id);
    gatt_advertise_interface_t* adv_ift = get_ble_advertise_instance();
    BT_CBACK(adv_ift->callbacks, ble_advtise_stopped_cb, adv_id);
}

void adapter_bt_link_role_changed_callback(BD_ADDR remote_addr, SERVICE_BT_LINK_ROLE link_role)
{
}

void adapter_scan_mode_changed_callback(SERVICE_BT_SCAN_MODE scan_mode)
{
}

void adapter_link_mode_changed_callback(BD_ADDR remote_addr, SERVICE_BT_LINK_MODE link_mode,
                                        uint16_t sniff_interval)
{
}

void adapter_link_connect_request_callback(BD_ADDR remote_addr)
{
    BT_LOGD("%s", __func__);
    service_adapter_gap_reply_link_request(remote_addr, true);
}

void adapter_link_policy_changed_callback(BD_ADDR remote_addr, SERVICE_BT_LINK_POLICY link_policy)
{
}

void adapter_stack_state_changed_callback(SERVICE_BT_STACK_STATE stack_state)
{
    BT_LOGD("Stack State Changed to %d", stack_state);
    if (stack_state == BT_STATE_ON)
    {
        uint8_t local_name[] = "BlueLet-NuttX";
        service_adapter_gap_set_local_name((char *)local_name, sizeof(local_name));
        service_adapter_gap_set_local_device_class(BT_COD_SERVICE_RENDERING | BT_COD_SERVICE_AUDIO |
                BT_COD_SERVICE_TELEPHONY | BT_COD_AV_HEADSET);
        service_adapter_gap_set_local_io_capability(SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT);
        service_adapter_gap_set_scan_mode(SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
    }
}

void adapter_hci_event_callback(SERVICE_BT_HCI_EVENT_S *hci_event)
{
}

extern int TL_h4_send(UINT8 *pBuf, UINT32 len);
void adapter_transport_write_packet_callback(uint8_t *hci_packet, uint32_t length)
{
    TL_h4_send(hci_packet, length);
}

void adapter_init_done_callback(void)
{
    excute_service_context_t *context = (excute_service_context_t *)malloc(sizeof(excute_service_context_t));
    context->loop_func = process_loop_in_common;
    context->data = NULL;
    // context->command_id = INIT_SERVICE_DONE_RESPONSE;
    process_in_loop(context);
}

void adapter_update_br_link_key_callback(SERVICE_REMOTE_DEVICE_S *bonded_device)
{
}

void adapter_delete_br_link_key_callback(BD_ADDR remote_addr)
{
}

void adapter_pairing_request_callback(BD_ADDR remote_addr, bool local_initiate, bool is_bondable)
{
    BT_LOGD("%s", __func__);
    service_adapter_gap_reply_pairing_request(remote_addr, 0);
}

void adapter_service_discovered_callback(BD_ADDR remote_addr, SERVICE_BR_SERVICE_S *services, uint16_t size)
{
}

void adapter_link_encryption_state_callback(BD_ADDR remote_addr, bool br_link, bool encryption_on)
{
}

void adapter_smp_request_callback(SERVICE_SSP_REQUEST_DATA_S *request_data)
{
}

void adapter_update_ble_bonded_devices_callback(SERVICE_BLE_KEYS_S *bonded_device_list, uint8_t count_in)
{
}

void adapter_ble_add_white_list_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status)
{
}

void adapter_ble_remove_white_list_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status)
{
}

void adapter_ble_add_resolving_list_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status)
{
}

void adapter_ble_remove_resolving_list_callback(BD_ADDR remote_addr, SERVICE_BT_STATUS status)
{
}

void adapter_ble_address_callback(BD_ADDR ble_addr, SERVICE_BLE_ADDR_TYPE ble_addr_type)
{
}

void adapter_ble_phy_update_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
                                     SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_BT_STATUS status)
{
}

void adapter_ble_irk_callback(BT_COMMON_KEY irk, BD_ADDR ble_addr,
                                     SERVICE_BLE_ADDR_TYPE ble_addr_type)
{

}
void adapter_ble_packet_received_callback(BD_ADDR remote_addr, uint16_t private_cid,
        uint8_t *packet, uint16_t packet_size)
{
}

void get_local_address(void)
{
    // TODO adapter interface
    // get_address.address
}
void get_state(void)
{
    // TODO adapter interface
    BT_LOGD("get_state");
}

void set_local_address(char *buff, size_t size)
{
    bool status = false;
    if (false == status)
    {
        BT_LOGD("Decoding failed");
    }
    // TODO adapter interface
    // set_address.address
}

GAP_CALLBACKS_S gap_callback = {
    sizeof(GAP_CALLBACKS_S),
    adapter_stack_state_changed_callback,
    adapter_received_remote_name_callback,
    adapter_device_found_callback,
    adapter_discovery_state_changed_callback,
    adapter_pin_request_callback,
    adapter_ssp_request_callback,
    adapter_bond_state_changed_callback,
    adapter_acl_state_changed_callback,
    adapter_ble_scan_result_callback,
    adapter_ble_adv_started_callback,
    adapter_ble_adv_stopped_callback,
    adapter_link_connect_request_callback,
    adapter_bt_link_role_changed_callback,
    adapter_scan_mode_changed_callback,
    adapter_link_mode_changed_callback,
    adapter_link_policy_changed_callback,
    adapter_hci_event_callback,
    adapter_transport_write_packet_callback,
    adapter_init_done_callback,
    adapter_update_br_link_key_callback,
    adapter_delete_br_link_key_callback,
    adapter_pairing_request_callback,
    adapter_service_discovered_callback,
    adapter_link_encryption_state_callback,
    adapter_smp_request_callback,
    adapter_update_ble_bonded_devices_callback,
    adapter_ble_add_white_list_callback,
    adapter_ble_remove_white_list_callback,
    adapter_ble_add_resolving_list_callback,
    adapter_ble_remove_resolving_list_callback,
    adapter_ble_address_callback,
    adapter_ble_phy_update_callback,
    adapter_ble_irk_callback,
    adapter_ble_packet_received_callback
};

void register_callback_to_stack(void)
{
}

static void *bluelet_loop_thread(void *arg)
{
    BT_LOGD("%s", __func__);
    service_adapter_gap_enable();
    while (1)
        ScheduleLoop();
    
    return NULL;
}

extern void InitTransportLayer(void);

int bluelet_init(void)
{
    pthread_t loop_pthread;
    pthread_attr_t attr;

    service_adapter_gap_init();
    service_adapter_gap_register_gap_callback(&gap_callback);

    InitTransportLayer();
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);
    pthread_create(&loop_pthread, &attr, bluelet_loop_thread, NULL);
    pthread_detach(loop_pthread);

    return 0;
}

void bts_common_init(void)
{
    //BT_LOGD(" common_init coming");
    bluelet_init();
    register_callback_to_stack();
}

static bts_gap_callback_t* bts_gap_callbacks = NULL;

bt_result_code gap_init(bts_gap_callback_t* cb)
{
    bts_gap_callbacks = cb;
    service_adapter_gap_init();
    service_adapter_gap_register_gap_callback(&gap_callback);
    return BT_RESULT_SUCCESS;
}

void gap_cleanup(void)
{
    bts_gap_callbacks = NULL;
    service_adapter_gap_cleanup();
}

bt_result_code gap_enable(void)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_enable();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap enable fail,ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_disable(bool normal_disable)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_disable(normal_disable);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap disable fail,ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static stack_state_t gap_get_stack_state(void)
{
    return BT_STATE_ON;
}

static gap_interface_t gap_interface = {
    .size = sizeof(gap_interface),

    .init = gap_init,
    .cleanup = gap_cleanup,
    .enable = gap_enable,
    .disable = gap_disable,
    .gap_get_stack_state = gap_get_stack_state,
};

const gap_interface_t* get_gap_instance(void)
{
    return &gap_interface;
}