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
#ifndef __GATTC_EVENT_H__
#define __GATTC_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bluetooth.h"
#include "bt_addr.h"
#include "gatt_define.h"
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

typedef enum {
    GATTC_EVENT_CONNECT_CHANGE,
    GATTC_EVENT_DISCOVER_RESULT,
    GATTC_EVENT_DISOCVER_CMPL,
    GATTC_EVENT_READ,
    GATTC_EVENT_WRITE,
    GATTC_EVENT_SUBSCRIBE,
    GATTC_EVENT_NOTIFY,
    GATTC_EVENT_MTU_UPDATE,
    GATTC_EVENT_PHY_READ,
    GATTC_EVENT_PHY_UPDATE,
    GATTC_EVENT_RSSI_READ,
    GATTC_EVENT_CONN_PARAM_UPDATE,
} gattc_event_t;

typedef enum {
    GATTC_REQ_CONNECT,
    GATTC_REQ_DISCONNECT,
    GATTC_REQ_DISCOVER,
    GATTC_REQ_READ,
    GATTC_REQ_WRITE,
    GATTC_REQ_SUBSCRIBE,
    GATTC_REQ_EXCHANGE_MTU,
    GATTC_REQ_READ_PHY,
    GATTC_REQ_UPDATE_PHY,
    GATTC_REQ_READ_RSSI,
} gattc_request_t;

typedef struct
{
    gattc_event_t event;
    bt_address_t addr;

    union {
        /**
         * @brief GATTC_EVENT_CONNECT_CHANGE
         */
        struct gattc_connect_change_evt_param {
            int32_t state;
            uint8_t reason;
        } connect_change;

        /**
         * @brief GATTC_EVENT_DISCOVER_RESULT
         */
        struct gattc_discover_result_evt_param {
            gatt_element_t* elements;
            uint16_t size;
        } discover_res;

        /**
         * @brief GATTC_EVENT_DISOCVER_CMPL
         */
        struct gattc_discover_complete_evt_param {
            gatt_status_t status;
        } discover_cmpl;

        /**
         * @brief GATTC_EVENT_READ
         */
        struct gattc_read_evt_param {
            gatt_status_t status;
            uint16_t element_id;
            uint16_t length;
            uint8_t value[0];
        } read;

        /**
         * @brief GATTC_EVENT_WRITE
         */
        struct gattc_write_evt_param {
            gatt_status_t status;
            uint16_t element_id;
        } write;

        /**
         * @brief GATTC_EVENT_SUBSCRIBE
         */
        struct gattc_subscribe_evt_param {
            gatt_status_t status;
            uint16_t element_id;
            bool enable;
        } subscribe;

        /**
         * @brief GATTC_EVENT_NOTIFY
         */
        struct gattc_notify_evt_param {
            bool is_notify;
            uint16_t element_id;
            uint16_t length;
            uint8_t value[0];
        } notify;

        /**
         * @brief GATTC_EVENT_MTU_UPDATE
         */
        struct gattc_mtu_evt_param {
            gatt_status_t status;
            uint32_t mtu;
        } mtu;

        /**
         * @brief GATTC_EVENT_PHY
         */
        struct gattc_phy_evt_param {
            gatt_status_t status;
            ble_phy_type_t tx_phy;
            ble_phy_type_t rx_phy;
        } phy;

        /**
         * @brief GATTC_EVENT_RSSI_READ
         */
        struct gattc_rssi_read_evt_param {
            gatt_status_t status;
            int32_t rssi;
        } rssi_read;

        /**
         * @brief GATTC_EVENT_CONN_PARAM_UPDATE
         */
        struct gattc_conn_param_update_evt_param {
            bt_status_t status;
            uint16_t interval;
            uint16_t latency;
            uint16_t timeout;
        } conn_param;

    } param;

} gattc_msg_t;

typedef struct
{
    gattc_request_t request;

    union {

        /**
         * @brief GATTC_REQ_CONNECT
         */
        struct gattc_connect_req_param {
            bt_address_t addr;
            ble_addr_type_t addr_type;
        } connect;

        /**
         * @brief GATTC_REQ_DISCONNECT
         */
        struct gattc_disconnect_req_param {
            void* conn_handle;
        } disconnect;

        /**
         * @brief GATTC_REQ_DISCOVER
         */
        struct gattc_discover_req_param {
            void* conn_handle;
            gatt_discover_type_t type;
        } discover;

        /**
         * @brief GATTC_REQ_READ
         */
        struct gattc_read_req_param {
            void* conn_handle;
            uint16_t attr_handle;
        } read;

        /**
         * @brief GATTC_REQ_WRITE
         */
        struct gattc_write_req_param {
            void* conn_handle;
            uint16_t attr_handle;
            gatt_write_type_t type;
        } write;

        /**
         * @brief GATTC_REQ_EXCHANGE_MTU
         */
        struct gattc_exchange_mtu_req_param {
            void* conn_handle;
            uint32_t mtu;
        } exchange_mtu;

        /**
         * @brief GATTC_REQ_PHY
         */
        struct gattc_phy_req_param {
            void* conn_handle;
            ble_phy_type_t tx_phy;
            ble_phy_type_t rx_phy;
        } phy;

        /**
         * @brief GATTC_REQ_READ_RSSI
         */
        struct gattc_read_rssi_req_param {
            void* conn_handle;
        } read_rssi;

    } param;

} gattc_op_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
gattc_msg_t* gattc_msg_new(gattc_event_t event, bt_address_t* addr, uint16_t playload_length);
void gattc_msg_destory(gattc_msg_t* msg);
gattc_op_t* gattc_op_new(gattc_request_t request);
void gattc_op_destory(gattc_op_t* operation);

#endif /* __GATTC_EVENT_H__ */
