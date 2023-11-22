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
#ifndef __GATTS_SERVICE_H__
#define __GATTS_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_gatt_defs.h"
#include "bt_gatts.h"
#include "gatt_define.h"

typedef enum {
    GATTS_SRV_STATE_IDLE = 0,
    GATTS_SRV_STATE_STARTING,
    GATTS_SRV_STATE_STOPPING,
    GATTS_SRV_STATE_STARTED,
} gatts_srv_state_t;

typedef enum {
    GATTS_CONN_STATE_DISCONNECTED = 0,
    GATTS_CONN_STATE_DISCONNECTING,
    GATTS_CONN_STATE_CONNECTING,
    GATTS_CONN_STATE_CONNECTED,
} gatts_conn_state_t;

/*
 * sal callback
 */
void if_gatts_on_connection_state_changed(bt_address_t *addr, profile_connection_state_t state);
void if_gatts_on_elements_added(gatt_status_t status, uint16_t element_id, uint16_t size);
void if_gatts_on_elements_removed(gatt_status_t status, uint16_t element_id, uint16_t size);
void if_gatts_on_received_element_read_request(bt_address_t *addr, uint32_t request_id, uint16_t element_id);
void if_gatts_on_received_element_write_request(bt_address_t *addr, uint32_t request_id, uint16_t element_id,
                                                uint8_t *value, uint16_t offset, uint16_t length, bool need_rsp);
void if_gatts_on_mtu_changed(bt_address_t *addr, uint32_t mtu);
void if_gatts_on_notification_sent(bt_address_t *addr, uint16_t element_id, gatt_status_t status);

/*
 * gatts remote
 */
void *if_gatts_get_remote(void *srv_handle);

typedef struct gatts_interface {
    size_t size;
    bt_status_t (*register_service)(void *remote, void **phandle, gatts_callbacks_t *callbacks);
    bt_status_t (*unregister_service)(void *srv_handle);
    bt_status_t (*connect)(void *srv_handle, bt_address_t *addr, ble_addr_type_t addr_type);
    bt_status_t (*disconnect)(void *srv_handle);
    bt_status_t (*create_service_table)(void *srv_handle, gatt_srv_db_t *srv_db);
    bt_status_t (*start)(void *srv_handle);
    bt_status_t (*stop)(void *srv_handle);
    bt_status_t (*response)(void *srv_handle, uint32_t req_handle, uint8_t *value, uint16_t length);
    bt_status_t (*notify)(void *srv_handle, uint16_t attr_handle, uint8_t *value, uint16_t length);
    bt_status_t (*indicate)(void *srv_handle, uint16_t attr_handle, uint8_t *value, uint16_t length);
} gatts_interface_t;

/*
 * register profile to service manager
 */
void register_gatts_service(void);

#endif /* __GATTS_SERVICE_H__ */