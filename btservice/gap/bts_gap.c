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
  GAP_LINK_MODE_CHANGED,
  GAP_SCAN_MODE_CHANGED,
  GAP_LINK_CONNECT_REQUEST,
  GAP_LINK_POLICY_CHANGED,
  GAP_HCI_EVENT,
  GAP_INIT_DONE,
  GAP_UPDATE_BR_LINKKEY,
  GAP_DELETE_BR_LINKKEY,
  GAP_PAIR_REQUEST,
  GAP_CONNECT_REQUEST,
  GAP_SERVICE_DISCOVERED,
  GAP_LINK_ENCRYPTION_STATE_CHANGED,
  GAP_SMP_REQUEST,
  GAP_UPDATE_BLE_BONDED_DEVICES,
  GAP_BLE_ADD_WHITE_LIST,
  GAP_BLE_REMOVE_WHITE_LIST,
  GAP_ADD_BLE_RESOlVING_LIST,
  GAP_REMOVE_BLE_RESOlVING_LIST,
  GAP_BLE_ADDRESS,
  GAP_BLE_PHY_UPDATE,
  GAP_BLE_IRK,
  GAP_BLE_PACKET_RECEIVED,
  GAP_BR_LINK_KEY_CHANGED,
  GAP_DELETE_LINK_KEY_CHANGED,
  GAP_EVENT_MAX_ID,
} gap_event_t;

typedef SERVICE_REMOTE_DEVICE_S device_found_t;

typedef struct {
  char *bt_name;
  uint8_t length;
} remote_name_info_t;

typedef struct {
    bool local_initiate;
    bool is_bondable;
}pair_request_t;

typedef struct{
    br_service_t* services;
    uint16_t size;
}discovery_service_t;

typedef struct {
    bool br_link;
    bool encryption_on;
}link_encryption_t;

typedef struct {
    ble_keys_t *bonded_device_list;
    uint8_t count_in;
}ble_bonded_update_t;

typedef struct
{
    ble_phy_type_t tx_phy;
    ble_phy_type_t rx_phy;
}phy_update_changed_t;

typedef struct 
{
    uint16_t private_cid;
    uint8_t *packet;
    uint16_t packet_size;
}ble_packet_receive_t;

typedef struct{
  bd_addr_t  bd_addr;
  ble_addr_type addr_type;
  uint32_t valueint1;
  bt_status status;
  union {
    device_found_t* found_result;
    discovery_state discovery_state;
    pin_request_data_t* pin_request_data;
    ssp_request_data_t* ssp_request_data;
    acl_state_params_t* acl_state_params;
    scan_result_t scan_result;
    bt_link_role link_role;
    bt_link_mode link_mode;
    bt_link_policy link_policy;
    bt_service_state stack_state;
    hci_event_t* hci_event;
    remote_name_info_t remote_name;
    bt_bonde_state bond_state;
    bt_scan_mode scan_mode;
    remote_device_t* bonded_device;
    pair_request_t pair_request;
    discovery_service_t discovery_service;
    link_encryption_t link_encryption;
    ble_bonded_update_t ble_bonded_update;
    phy_update_changed_t phy_update;
    bt_common_key irk;
    ble_packet_receive_t ble_packet_receive;
    bt_device_t* device;
  }data;
} gap_event_data_t;

typedef struct
{
  struct list_node node;
  gap_event_t event;
  gap_event_data_t event_data;
} gap_msg_t;

typedef struct 
{
    struct list_node node;
    bt_device_t* device;
}discovery_device_t;

struct list_node *msg_list;
struct list_node *discovery_list = NULL;

extern void InitTransportLayer(void);

gap_msg_t *gap_msg_new(gap_event_t event)
{
  gap_msg_t *msg;
  if(!msg_list)
    return NULL;
  msg = (gap_msg_t *)malloc(sizeof(gap_msg_t));
  if (!msg)
    return NULL;

  msg->event = event;
  memset(&msg->event_data, 0, sizeof(msg->event_data));
  list_add_tail(msg_list, &msg->node);
  
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
    while (!list_is_empty(msg_list)){

        gap_msg_t *gap_msg = list_remove_head(msg_list);
        //gap_msg_t *gap_msg = (gap_msg_t*)data;
        switch (gap_msg->event){
            case GAP_STACK_STATE_CHANGED:{
                bluetooth_service_interface * service_interface = get_bluetooth_service_interface();
                if (service_interface){
                    service_interface->stack_state_change(gap_msg->event_data.data.stack_state);
                }
                break;
            }
            case GAP_DISCOVERY_STATE_CHANGED:{
                if (gap_msg->event_data.data.discovery_state == BT_DISCOVERY_STARTED) {
                    if(!discovery_list){
                        discovery_list = malloc(sizeof(struct list_node));
                        list_initialize(discovery_list);
                    }
                }
                if (gap_msg->event_data.data.discovery_state == BT_DISCOVERY_STOPPED){
                    struct list_node *node;
                    discovery_device_t *discovery_devce;
                    while (!list_is_empty(discovery_list))
                    {
                        node = list_remove_head(discovery_list);
                        free(node);
                    }
                    
                    free(discovery_list);
                    discovery_list = NULL;
                }
                if ((bts_gap_callbacks) && (bts_gap_callbacks->discovery_state_changed_cb)) {
                    bts_gap_callbacks->discovery_state_changed_cb(gap_msg->event_data.data.discovery_state);
                }
                break;
            }
            case GAP_REMOTE_NAME:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->remote_name_cb)) {
                    bts_gap_callbacks->remote_name_cb(gap_msg->event_data.bd_addr, 
                        gap_msg->event_data.data.remote_name.bt_name, 
                        gap_msg->event_data.data.remote_name.length);
                }
                break;
            }
            case GAP_SSP_REQUEST:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->spp_request_cb)) {
                        bts_gap_callbacks->spp_request_cb(gap_msg->event_data.data.ssp_request_data);
                }            
                break;
            }
            case GAP_DEVICE_FOUND:{
                struct list_node *node;
                discovery_device_t *discovery_devce;
                bt_device_t *device = gap_msg->event_data.data.device;

                list_for_every(discovery_list, node){
                    discovery_devce = (discovery_device_t *)node;
                    if(!memcmp(discovery_devce->device->addr, device->addr, BT_ADDR_LENGTH)){
                        return ;
                    }
                }
                discovery_devce = malloc(sizeof(discovery_device_t));
                discovery_devce->device = device;
                list_add_tail(discovery_list, &discovery_devce->node);

                if ((bts_gap_callbacks) && (bts_gap_callbacks->device_found_cb)) {
                        bts_gap_callbacks->device_found_cb(device);
                }                
                break;
            } 
            case GAP_BOND_STATE_CHANGED:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->bond_state_changed_cb)) {
                            bt_device_t *new_device  = malloc(sizeof(bt_device_t));
                            memcpy(new_device->addr, gap_msg->event_data.bd_addr, BT_ADDR_LENGTH);
                            bts_gap_callbacks->bond_state_changed_cb(gap_msg->event_data.data.device,gap_msg->event_data.data.bond_state);
                }                            
                break;
            }         
            case GAP_ACL_STATE_CHANGED:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->connection_state_changed_cb)) {
                            bt_device_t *new_device  = malloc(sizeof(bt_device_t));
                            memcpy(new_device->addr, gap_msg->event_data.bd_addr, BT_ADDR_LENGTH);
                            bts_gap_callbacks->connection_state_changed_cb(gap_msg->event_data.data.device,gap_msg->event_data.data.acl_state_params->state);
                }                
                break;
            }                 
            case GAP_HCI_EVENT:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->hci_event_cb)) {
                        bts_gap_callbacks->hci_event_cb(gap_msg->event_data.data.hci_event);
                }                            
                break;
            }                
            case GAP_UPDATE_BLE_BONDED_DEVICES:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->update_ble_bonede_device_cb)) {
                        bts_gap_callbacks->update_ble_bonede_device_cb(gap_msg->event_data.data.ble_bonded_update.bonded_device_list, 
                                gap_msg->event_data.data.ble_bonded_update.count_in);
                }         
                break;
            }                    
            case GAP_SMP_REQUEST:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->smp_request_cb)) {
                        bts_gap_callbacks->smp_request_cb(gap_msg->event_data.data.ssp_request_data);
                }                     
                break;
            }              
            case GAP_BLE_PHY_UPDATE:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->ble_phy_update_cb)) {
                        bts_gap_callbacks->ble_phy_update_cb(gap_msg->event_data.bd_addr,
                            gap_msg->event_data.data.phy_update.tx_phy,gap_msg->event_data.data.phy_update.rx_phy,
                            gap_msg->event_data.status);
                }                 
                break;
            }              
            case GAP_BLE_ADDRESS:{
                if ((bts_gap_callbacks) && (bts_gap_callbacks->ble_address_cb)) {
                        bts_gap_callbacks->ble_address_cb(gap_msg->event_data.data.device->addr, gap_msg->event_data.addr_type);
                }                            
                break;
            }      
            default:
            break;
        }

        gap_msg_destory(gap_msg);
        
    }
}

void gap_send_message(gap_msg_t *msg)
{
  excute_service_context_t *context = (excute_service_context_t *)malloc(sizeof(excute_service_context_t));

  context->loop_func = process_loop_in_gap;
  //context->data = (void *)msg;
  //context->data_size = sizeof(gap_msg_t);
  context->profile_id = BT_PROFILE_GAP_ID;
  process_in_loop(context);
}

bt_result_code bts_start_discovery(uint32_t timeout)
{
    service_adapter_gap_start_device_discovery(timeout);
}

void adapter_device_found_callback(device_found_t *device)
{
    //BT_LOGD("%s", __func__);
    if ((!bts_gap_callbacks) || (!bts_gap_callbacks->device_found_cb))
        return;
    bt_device_t *new_device  = malloc(sizeof(bt_device_t));
    memcpy(new_device->addr, device->bd_addr, BT_ADDR_LENGTH);    
    gap_msg_t *msg = gap_msg_new(GAP_DEVICE_FOUND);
    msg->event_data.data.device = new_device;
    gap_send_message(msg);
}

void adapter_received_remote_name_callback(bd_addr_t bd_addr, char *bt_name, uint8_t length)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_REMOTE_NAME);

    msg->event_data.data.remote_name.bt_name = (char*)malloc(length);
    memcpy(msg->event_data.data.remote_name.bt_name, bt_name, length);
    msg->event_data.data.remote_name.length = length;
    gap_send_message(msg);
}

void adapter_discovery_state_changed_callback(discovery_state state)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_DISCOVERY_STATE_CHANGED);
    msg->event_data.data.discovery_state = state;
    gap_send_message(msg);
}

void adapter_pin_request_callback(pin_request_data_t *request_data)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_PIN_CODE_REQUEST);
    msg->event_data.data.pin_request_data = request_data;
    gap_send_message(msg);
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
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_SSP_REQUEST);
    msg->event_data.data.ssp_request_data = request_data;
    gap_send_message(msg);
}

void adapter_bond_state_changed_callback(bd_addr_t remote_addr, bt_bonde_state state)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_BOND_STATE_CHANGED);
    memcpy(msg->event_data.bd_addr, remote_addr, BD_ADDR_LEN);
    msg->event_data.data.bond_state= state;
    gap_send_message(msg);    
}

void adapter_acl_state_changed_callback(acl_state_params_t* acl_state_param)
{
    BT_LOGD("%s status:%d, state:%d, reasonCode:%d", __func__, acl_state_param->status, acl_state_param->state, acl_state_param->reasonCode);
    gap_msg_t *msg = gap_msg_new(GAP_ACL_STATE_CHANGED);
    msg->event_data.data.acl_state_params= acl_state_param;
    gap_send_message(msg);        
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
    gap_msg_t *msg = gap_msg_new(GAP_LINK_ROLE_CHANGED);
    memcpy(msg->event_data.bd_addr, remote_addr, BD_ADDR_LEN);
    msg->event_data.data.link_role = link_role;
    gap_send_message(msg);    
}

void adapter_scan_mode_changed_callback(bt_scan_mode scan_mode)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_SCAN_MODE_CHANGED);
    msg->event_data.data.scan_mode = scan_mode;
    gap_send_message(msg);        
}

void adapter_link_mode_changed_callback(bd_addr_t remote_addr, bt_link_mode link_mode,
                                        uint16_t sniff_interval)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_LINK_MODE_CHANGED);
    msg->event_data.data.link_mode = link_mode;
    msg->event_data.valueint1 = sniff_interval;
    gap_send_message(msg); 
}

void adapter_link_connect_request_callback(bd_addr_t remote_addr)
{
    BT_LOGD("%s", __func__);
    service_adapter_gap_reply_link_request(remote_addr, true);

    gap_msg_t *msg = gap_msg_new(GAP_CONNECT_REQUEST);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    gap_send_message(msg); 
}

void adapter_link_policy_changed_callback(bd_addr_t remote_addr, bt_link_policy link_policy)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_LINK_POLICY_CHANGED);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.link_policy = link_policy;
    gap_send_message(msg);     
}

void adapter_stack_state_changed_callback(bt_service_state stack_state)
{
    gap_msg_t *msg = gap_msg_new(GAP_STACK_STATE_CHANGED);
    BT_LOGD("%s", __func__);
    msg->event_data.data.stack_state = stack_state;    
    gap_send_message(msg);

}

void adapter_hci_event_callback(hci_event_t *hci_event)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_HCI_EVENT);
    msg->event_data.data.hci_event = hci_event;
    gap_send_message(msg);     
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
    gap_msg_t *msg = gap_msg_new(GAP_BR_LINK_KEY_CHANGED);
    msg->event_data.data.bonded_device = bonded_device;
    gap_send_message(msg);     
    gap_update_data_storage();
}

void adapter_delete_br_link_key_callback(bd_addr_t remote_addr)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_DELETE_LINK_KEY_CHANGED);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    gap_send_message(msg);     

    gap_update_data_storage();

}

void adapter_pairing_request_callback(bd_addr_t remote_addr, bool local_initiate, bool is_bondable)
{
    BT_LOGD("%s", __func__);
    service_adapter_gap_reply_pairing_request(remote_addr, 0);
    gap_msg_t *msg = gap_msg_new(GAP_PAIR_REQUEST);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.pair_request.is_bondable = is_bondable;
    msg->event_data.data.pair_request.local_initiate = local_initiate;

    gap_send_message(msg);     
}

void adapter_service_discovered_callback( bd_addr_t remote_addr, br_service_t* services, uint16_t size)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_SERVICE_DISCOVERED);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.discovery_service.services = services;
    msg->event_data.data.discovery_service.size = size;

    gap_send_message(msg);         
}

void adapter_link_encryption_state_callback( bd_addr_t remote_addr, bool br_link, bool encryption_on)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_LINK_ENCRYPTION_STATE_CHANGED);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.link_encryption.br_link = br_link;
    msg->event_data.data.link_encryption.encryption_on = encryption_on;
    gap_send_message(msg);         
}

void adapter_smp_request_callback(ssp_request_data_t *request_data)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_SMP_REQUEST);
    msg->event_data.data.ssp_request_data = request_data;
    gap_send_message(msg);         
}

void adapter_update_ble_bonded_devices_callback(ble_keys_t *bonded_device_list, uint8_t count_in)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_UPDATE_BLE_BONDED_DEVICES);
    msg->event_data.data.ble_bonded_update.bonded_device_list = bonded_device_list;
    msg->event_data.data.ble_bonded_update.count_in = count_in;
    gap_send_message(msg);             
}

void adapter_ble_add_white_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_BLE_ADD_WHITE_LIST);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);   
}

void adapter_ble_remove_white_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_BLE_REMOVE_WHITE_LIST);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);   
}

void adapter_ble_add_resolving_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_ADD_BLE_RESOlVING_LIST);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);       
}

void adapter_ble_remove_resolving_list_callback( bd_addr_t remote_addr, bt_status status)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_REMOVE_BLE_RESOlVING_LIST);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);           
}

void adapter_ble_address_callback( bd_addr_t ble_addr, ble_addr_type ble_addr_type)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_BLE_ADDRESS);
    memcpy(msg->event_data.bd_addr, ble_addr, BT_ADDR_LENGTH);
    msg->event_data.addr_type = ble_addr_type;
    gap_send_message(msg);               
}

void adapter_ble_phy_update_callback( bd_addr_t remote_addr, ble_phy_type_t tx_phy,
                                     ble_phy_type_t rx_phy, bt_status status)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_BLE_PHY_UPDATE);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.phy_update.tx_phy = tx_phy;
    msg->event_data.data.phy_update.rx_phy = rx_phy;
    msg->event_data.status = status;
    gap_send_message(msg);              
}

void adapter_ble_irk_callback(bt_common_key irk,  bd_addr_t ble_addr,
                                     ble_addr_type ble_addr_type)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_BLE_IRK);
    memcpy(msg->event_data.bd_addr, ble_addr, BT_ADDR_LENGTH);
    msg->event_data.addr_type = ble_addr_type;
    memcpy(msg->event_data.data.irk, irk, BT_COMMON_KEY_SIZE);
    gap_send_message(msg);               
}

void adapter_ble_packet_received_callback( bd_addr_t remote_addr, uint16_t private_cid,
        uint8_t *packet, uint16_t packet_size)
{
    BT_LOGD("%s", __func__);
    gap_msg_t *msg = gap_msg_new(GAP_BLE_PACKET_RECEIVED);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.ble_packet_receive.packet = packet;
    msg->event_data.data.ble_packet_receive.packet_size = packet_size;
    msg->event_data.data.ble_packet_receive.private_cid = private_cid;
    gap_send_message(msg);            
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


void get_local_address(void)
{
    // TODO adapter interface
    // get_address.address
}
static void get_state(void)
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

bt_result_code gap_init(bts_gap_callback_t* cb)
{
    bts_gap_callbacks = cb;
    service_adapter_gap_init();

    service_adapter_gap_register_gap_callback(&gap_callback);
    if(!msg_list)
        msg_list = malloc(sizeof(struct list_node));
    
    list_initialize(msg_list);
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

int  bts_get_remote_services(bt_device_t* device, bt_uuid_t *service_list, uint8_t count_in)
{
    int ret = -1;
    if (!device)
        return ret;
    ret = service_adapter_gap_get_remote_services(device->addr, service_list, count_in);
    return ret;   
}

 bt_bond_state bts_get_bond_state(bt_device_t *device)
 {
     //TODO, need bond state
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
bt_result_code bts_get_bonded_devices(void)
{
     bt_status ret = BT_RESULT_FAILED;
     
     return ret;   

}

/*Connection*/
 bt_result_code bts_connect_all(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;

     return ret;       
}
 bt_result_code bts_disonnect_all(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
     return ret;       
}
 bt_result_code bts_get_connected_devices(void)
{
     bt_status ret = BT_RESULT_FAILED;
     //service_adapter_gap_get_connected_devices
     return ret;       
}

/*Discovery*/
 bt_result_code bts_set_scan_mode(bt_scan_mode scan_mode, bool bondable)
{
     bt_status ret = BT_RESULT_FAILED;
    ret = service_adapter_gap_set_scan_mode(scan_mode, bondable);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;    
    }
     return ret;       
}

 bt_result_code bts_stop_discovery(void)
{
     bt_status ret = BT_RESULT_FAILED;

    ret = service_adapter_gap_stop_device_discovery();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }
     return ret;       
}
 
 /*service discovery*/

bt_result_code bts_start_service_discovery(bt_device_t *device, bt_uuid_t uuid)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;     
    ret = service_adapter_gap_start_service_discovery(device->addr, uuid);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }     
     return ret;       
}
bt_result_code bts_stop_service_discovery(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;     
    ret = service_adapter_gap_stop_service_discovery(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }          
     return ret;       
}

/*VSC command*/
 bt_result_code bts_send_hci_command_v1(bt_hci_command_t *command, bt_service_hci_command_complete_event event_type)
{
    bt_status ret = BT_RESULT_FAILED;
    if (!command)
        return BT_RESULT_FAILED;     
    ret = service_adapter_gap_send_hci_command_v1(command, event_type);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }       
    return ret;       
}

/*ble interface*/
bt_result_code bts_set_ble_scan_parameters(scan_params_t *scan_param)
{
     bt_status ret = BT_RESULT_FAILED;
     return ret;       
}
bt_result_code bts_set_ble_scan_filter(ble_scan_filter_t *scan_filter)
{
     bt_status ret = BT_RESULT_FAILED;
     return ret;       
}
bt_result_code bts_start_ble_scan(void)
{
     bt_status ret = BT_RESULT_FAILED;
     return ret;       
}
bt_result_code bts_stop_ble_scan(void)
{
     bt_status ret = BT_RESULT_FAILED;
     return ret;       
}
bt_result_code bts_start_ble_adv(advertise_param_t *scan_params)
{
     bt_status ret = BT_RESULT_FAILED;
     return ret;       
}
bt_result_code bts_stop_ble_adv(uint8_t adv_id)
{
     bt_status ret = BT_RESULT_FAILED;
     return ret;       
}

bt_result_code bts_ble_set_static_identity(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return BT_RESULT_FAILED;     
    ret = service_adapter_gap_ble_set_static_identity(device);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }       
     return ret;       
}
bt_result_code bts_ble_get_current_irk(void)
{
     bt_status ret = BT_RESULT_FAILED;
    ret = service_adapter_gap_ble_get_current_irk();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }            
     return ret;       
}
bt_result_code bts_ble_set_address(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    ret = service_adapter_gap_ble_set_address(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }      
     return ret;       
}
bt_result_code bts_ble_get_address(void)
{
    bt_status ret = BT_RESULT_FAILED;
    ret = service_adapter_gap_ble_get_address();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }      
     return ret;       
}
bt_result_code bts_ble_set_bonded_devices(ble_keys_t *bonded_device_list, uint8_t count_in)
{
    bt_status ret = BT_RESULT_FAILED;
    if (!bonded_device_list)
        return ret;
    ret = service_adapter_gap_ble_set_bonded_devices(bonded_device_list, count_in);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }      
     return ret;       
}
bt_result_code bts_ble_connect(ble_connect_params_t *conn_param)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!conn_param)
        return ret;
    ret = service_adapter_gap_ble_connect(conn_param);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }      
     return ret;       
}
bt_result_code bts_ble_disconnect(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return ret;
    ret = service_adapter_gap_ble_disconnect(device);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }           
     return ret;       
}
bt_result_code bts_ble_smp_reply(spp_reply_data_t *reply_data)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!reply_data)
        return ret;
    ret = service_adapter_gap_ble_smp_reply(reply_data);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }                
     return ret;       
}
bt_result_code bts_ble_add_white_list(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return ret;
    ret = service_adapter_gap_ble_add_white_list(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }                
     return ret;       
}
bt_result_code bts_ble_remove_white_list(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return ret;
    ret = service_adapter_gap_ble_remove_white_list(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }                
     return ret;       
}
bt_result_code bts_ble_add_resolving_list(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return ret;
    ret = service_adapter_gap_ble_add_resolving_list(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }               
     return ret;       
}
bt_result_code bts_ble_remove_resolving_list(bt_device_t *device)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return ret;
    ret = service_adapter_gap_ble_remove_resolving_list(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }              
     return ret;       
}
bt_result_code bts_ble_set_phy(bt_device_t *device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return ret;
    ret = service_adapter_gap_ble_set_phy(device->addr, tx_phy, rx_phy);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }              
     return ret;       
}
bt_result_code bts_ble_add_private_channel(uint16_t private_cid)
{
     bt_status ret = BT_RESULT_FAILED;
    ret = service_adapter_gap_ble_add_private_channel(private_cid);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }      
     return ret;       
}
bt_result_code bts_ble_send_packet(bt_device_t *device, uint16_t private_cid,
        uint8_t *packet, uint16_t packet_size)
{
     bt_status ret = BT_RESULT_FAILED;
    if (!device)
        return ret;
    ret = service_adapter_gap_ble_send_packet(device, private_cid, packet, packet_size);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }      
     return ret;       
}       
/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code bts_send_hci_command(uint8_t *hci_cmd_packet, hci_event_callback cb)
{
     bt_status ret = BT_RESULT_FAILED;
    if ((!hci_cmd_packet) || (!cb))
        return ret;
    service_gap_send_hci_command(hci_cmd_packet, cb);
    ret = BT_RESULT_SUCCESS;
     return ret;       
}
bt_result_code bts_enter_bluetooth_test_mode(bt_test_mode test_mode)
{
     bt_status ret = BT_RESULT_FAILED;
    ret = service_adapter_gap_enter_bluetooth_test_mode(test_mode);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d",  __func__, ret);
        return BT_RESULT_FAILED;
    }      
     return ret;       
}