/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_gap.c
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
#include "bts_service_interface.h"
#include "btm_manager.h"
#include "bts_service.h"
#include "bts_gap.h"
#include "bts_le_advertise.h"
#include "bts_le_scan.h"

#define LOG_TAG "bts_gap"
#include "log.h"

typedef enum
{
  GAP_STACK_STATE_CHANGED = 1,
  GAP_DEVICE_FOUND,
  GAP_REMOTE_NAME,
  GAP_DISCOVERY_STATE_CHANGED,
  GAP_PIN_CODE_REQUEST,
  GAP_SSP_REQUEST,
  GAP_BOND_STATE_CHANGED,
  GAP_ACL_STATE_CHANGED,
  GAP_BLE_SCAN_REQUEST,
  GAP_BLE_ADV_STARTED,
  GAP_BLE_ADV_STOPPED,
  GAP_LINK_ROLE_CHANGED,
  GAP_SCAN_MODE_CHANGED,
  GAP_LINK_CONNECT_REQUEST,
  GAP_LINK_POLICY_CHANGED,
  GAP_HCI_EVENT,
  GAP_INIT_DONE,
  GAP_UPDATE_BR_LINKKEY,
  GAP_DELETE_BR_LINKKEY,
  GAP_PAIR_REQUEST,
  GAP_SERVICE_DISCOVERED,
  GAP_LINK_ENCRYPTION_STATE_CHANGED,
  GAP_SMP_REQUEST,
  GAP_UPDATE_BLE_BONDED_DEVICES,
  GAP_BLE_ADD_WHITE_LIST,
  GAP_REMOVE_WHITE_LIST,
  GAP_ADD_BLE_RESOlVING_LIST,
  GAP_REMOVE_BLE_RESOlVING_LIST,
  GAP_BLE_ADDRESS,
  GAP_BLE_PHY_UPDATE,
  GAP_BLE_IRK,
  GAP_BLE_PACKET_RECEIVED,

  GAP_EVENT_MAX_ID,
} gap_event_t;

typedef SERVICE_REMOTE_DEVICE_S device_found_t;

typedef struct {
  bd_addr_t bd_addr;
  char *bt_name;
  uint8_t length;
} name_request_t;

typedef struct{

  bd_addr_t  bd_addr;
  union 
  {
      device_found_t found_result;
      discovery_state discovery_state;
      pin_request_data_t pin_request_data;
      ssp_request_data_t ssp_request_data;
      acl_state_params_t acl_state;
      scan_result_t scan_result;
      bt_link_role link_role;
      bt_link_mode link_mode;
      bt_link_policy link_policy;
      bt_service_state stack_state;
      hci_event_t hci_event;
  }data;
  
} gap_event_data_t;

typedef struct
{
  gap_event_t event;
  gap_event_data_t event_data;
} gap_msg_t;


extern void InitTransportLayer(void);

gap_msg_t *gap_msg_new(gap_event_t event)
{
  gap_msg_t *msg;

  BT_LOGD("%s", __func__);

  msg = (gap_msg_t *)malloc(sizeof(gap_msg_t));
  if (!msg)
    return NULL;

  msg->event = event;
  memset(&msg->event_data, 0, sizeof(msg->event_data));

  return msg;
}

void gap_msg_destory(gap_msg_t *msg)
{

  free(msg);
}


static bts_gap_callback_t* bts_gap_callbacks = NULL;
/*process callback from stack */
void process_loop_in_gap(void *data, size_t data_size)
{
    BT_LOGD("%s", __func__);
    if (!data)
        return;
    gap_msg_t *gap_msg = (gap_msg_t*)data;
    switch (gap_msg->event){
        case GAP_STACK_STATE_CHANGED:{
            if (bts_gap_callbacks) {
                bts_gap_callbacks->adapter_state_changed_cb(gap_msg->event_data.data.stack_state);
            }
            bluetooth_service_interface * service_interface = get_bluetooth_service_interface();
            if (service_interface){
                service_interface->stack_state_change(gap_msg->event_data.data.stack_state);
            }
        }
        default:
        break;
    }

    gap_msg_destory(gap_msg);
}

void gap_send_message(gap_msg_t *msg)
{
  excute_service_context_t *context = (excute_service_context_t *)malloc(sizeof(excute_service_context_t));

  context->loop_func = process_loop_in_gap;
  context->data = (void *)msg;
  context->data_size = sizeof(gap_msg_t);
  process_in_loop(context);
}

bt_result_code bts_start_discovery(uint32_t timeout)
{
    service_adapter_gap_start_device_discovery(timeout);
}



void adapter_device_found_callback(device_found_t *device)
{
    BT_LOGD("%s", __func__);
}
void adapter_received_remote_name_callback(bd_addr_t bd_addr, char *bt_name, uint8_t length)
{
    BT_LOGD("%s", __func__);
}

void adapter_discovery_state_changed_callback(discovery_state state)
{
    BT_LOGD("%s", __func__);
}

void adapter_pin_request_callback(pin_request_data_t *request_data)
{
    BT_LOGD("%s", __func__);
}

void adapter_ssp_request_callback(ssp_request_data_t *request_data)
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

void adapter_bond_state_changed_callback(bd_addr_t remote_addr, bt_bonde_state state)
{
    BT_LOGD("%s", __func__);
}

void adapter_acl_state_changed_callback(acl_state_params_t* acl_state_param)
{
    BT_LOGD("%s status:%d, state:%d, reasonCode:%d", __func__, acl_state_param->status, acl_state_param->state, acl_state_param->reasonCode);
}

void adapter_ble_scan_result_callback(scan_result_t* scan_result_data)
{
    BT_LOGD("%s", __func__);
    bts_le_scan_interface_t* scan_ift = get_bts_lescan_instance();
    BT_CBACK(scan_ift->callbacks, ble_scan_result, scan_result_data);
}

void adapter_ble_adv_started_callback(uint8_t adv_id)
{
    BT_LOGD("%s, adv_id:%d", __func__, adv_id);
    bts_le_advertise_interface_t* adv_ift = get_bts_bleadv_instance();
    BT_CBACK(adv_ift->callbacks, ble_advtise_started_cb, adv_id);
}

void adapter_ble_adv_stopped_callback(uint8_t adv_id)
{
    BT_LOGD("%s, adv_id:%d", __func__, adv_id);
    bts_le_advertise_interface_t* adv_ift = get_bts_bleadv_instance();
    BT_CBACK(adv_ift->callbacks, ble_advtise_stopped_cb, adv_id);
}

void adapter_bt_link_role_changed_callback(bd_addr_t remote_addr, bt_link_role link_role)
{
    BT_LOGD("%s", __func__);
}

void adapter_scan_mode_changed_callback(bt_scan_mode scan_mode)
{
    BT_LOGD("%s", __func__);
}

void adapter_link_mode_changed_callback(bd_addr_t remote_addr, bt_link_mode link_mode,
                                        uint16_t sniff_interval)
{
    BT_LOGD("%s", __func__);
}

void adapter_link_connect_request_callback(bd_addr_t remote_addr)
{
    BT_LOGD("%s", __func__);
    service_adapter_gap_reply_link_request(remote_addr, true);
}

void adapter_link_policy_changed_callback(bd_addr_t remote_addr, bt_link_policy link_policy)
{
    BT_LOGD("%s", __func__);
}

void adapter_stack_state_changed_callback(bt_service_state stack_state)
{
    BT_LOGD("Stack State Changed to %d", stack_state);
    gap_msg_t *msg = gap_msg_new(GAP_STACK_STATE_CHANGED);
    BT_LOGD("%s", __func__);


    msg->event_data.data.stack_state = stack_state;
    
    gap_send_message(msg);

    service_adapter_gap_set_local_device_class(BT_COD_SERVICE_RENDERING | BT_COD_SERVICE_AUDIO |
            BT_COD_SERVICE_TELEPHONY | BT_COD_AV_HEADSET);
    service_adapter_gap_set_local_io_capability(SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT);
    service_adapter_gap_set_scan_mode(SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
}

void adapter_hci_event_callback(hci_event_t *hci_event)
{
    BT_LOGD("%s", __func__);
}

extern int TL_h4_send(UINT8 *pBuf, UINT32 len);
void adapter_transport_write_packet_callback(uint8_t *hci_packet, uint32_t length)
{
    TL_h4_send(hci_packet, length);
}

void adapter_init_done_callback(void)
{
    BT_LOGD("%s", __func__);

    excute_service_context_t *context = (excute_service_context_t *)malloc(sizeof(excute_service_context_t));
    context->loop_func = process_loop_in_gap;
    context->data = NULL;
    //context->command_id = INIT_SERVICE_DONE_RESPONSE;
    process_in_loop(context);
}

void adapter_update_br_link_key_callback(remote_device_t* bonded_device)
{
    BT_LOGD("%s", __func__);
    gap_update_data_storage();
}

void adapter_delete_br_link_key_callback(bd_addr_t remote_addr)
{
    BT_LOGD("%s", __func__);
}

void adapter_pairing_request_callback(bd_addr_t remote_addr, bool local_initiate, bool is_bondable)
{
    BT_LOGD("%s", __func__);
    service_adapter_gap_reply_pairing_request(remote_addr, 0);
}

void adapter_service_discovered_callback( bd_addr_t remote_addr, br_service_t* services, uint16_t size)
{
    BT_LOGD("%s", __func__);
}

void adapter_link_encryption_state_callback( bd_addr_t remote_addr, bool br_link, bool encryption_on)
{
    BT_LOGD("%s", __func__);
}

void adapter_smp_request_callback(ssp_request_data_t *request_data)
{
    BT_LOGD("%s", __func__);
}

void adapter_update_ble_bonded_devices_callback(ble_keys_t *bonded_device_list, uint8_t count_in)
{
    BT_LOGD("%s", __func__);
}

void adapter_ble_add_white_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
}

void adapter_ble_remove_white_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
}

void adapter_ble_add_resolving_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
}

void adapter_ble_remove_resolving_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
}

void adapter_ble_address_callback( bd_addr_t ble_addr, ble_addr_type ble_addr_type)
{
    BT_LOGD("%s", __func__);
}

void adapter_ble_phy_update_callback( bd_addr_t remote_addr, ble_phy_type_t tx_phy,
                                     ble_phy_type_t rx_phy, bt_status status)
{
    BT_LOGD("%s", __func__);
}

void adapter_ble_irk_callback(bt_common_key irk,  bd_addr_t ble_addr,
                                     ble_addr_type ble_addr_type)
{
    BT_LOGD("%s", __func__);

}

void adapter_ble_packet_received_callback( bd_addr_t remote_addr, uint16_t private_cid,
        uint8_t *packet, uint16_t packet_size)
{
    BT_LOGD("%s", __func__);
}

void get_local_address(void)
{
    // TODO adapter interface
    // get_address.address
}
void get_state(void)
{
    BT_LOGD("%s", __func__);
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

bt_result_code gap_enable()
{
    bt_status ret = service_adapter_gap_enable();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap enable fail,ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_disable(bool normal_disable)
{
    bt_status ret = service_adapter_gap_disable(normal_disable);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap disable fail,ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

 bt_result_code bts_set_local_name(char *bt_name, uint8_t len)
 {
    bt_status ret = service_adapter_gap_set_local_name(bt_name, len);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
 }

 bt_result_code bts_set_local_address(bt_device_t *device)
 {
    if (!device)
        return BT_RESULT_FAILED;
   bt_status ret = service_adapter_gap_set_local_address(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;     
 }

 bt_address* bts_get_local_address(void)
 {
    bt_address * addr = malloc(sizeof(bt_address));

   bt_status ret = service_adapter_gap_get_local_address(addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        free(addr);
        return NULL;
    }
    return addr;     
 }

 bt_result_code bts_set_local_io_capability(bt_io_capability io_capability)
 {
    bt_status ret = service_adapter_gap_set_local_io_capability(io_capability);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;         
 }

 char* bts_get_local_name()
 {
     char *name = malloc(MAX_NAME_LEN);
    bt_status ret = service_adapter_gap_get_local_name(&name, MAX_NAME_LEN);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return NULL;
    }
    return name;         
 }

  bt_result_code bts_set_local_device_class(uint32_t class_of_device)
  {
    bt_status ret = service_adapter_gap_set_local_device_class(class_of_device);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;         
  }

  uint32_t bts_get_local_device_class()
  {
    uint32_t device_class = 0;
    device_class = bts_get_local_device_class();
    return device_class;         
  }

 bt_result_code bts_get_remote_name(bt_device_t *device)
 {
    if (!device)
        return BT_RESULT_FAILED;
    bt_status ret = service_adapter_gap_get_remote_name(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;         
 }

 bt_result_code bts_get_connection_state(bt_device_t *device)
 {
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;         
 }

int  bts_get_remote_services(bt_device_t remote_addr, bt_uuid_t *service_list, uint8_t count_in)
{
    return -1;
}

 bt_bond_state bts_get_bond_state(bt_device_t *device)
 {
     return -1;
 }

 bt_result_code bts_reply_pair_request(bt_device_t *device, bool accept)
 {
    bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;
    service_adapter_gap_reply_pairing_request(device->addr, accept);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;   

 }

 bt_result_code bts_create_bond(bt_device_t *device)
 {
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;
    ret = service_adapter_gap_create_bond(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;   
    
 }

bt_result_code bts_cancel_bond(bt_device_t *device)
 {
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;
    ret = service_adapter_gap_cancel_bond(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;   
    
 }

 bt_result_code bts_remove_bond(bt_device_t *device)
 {
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;
    ret = service_adapter_gap_remove_bond(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
    return ret;   
    
 }
