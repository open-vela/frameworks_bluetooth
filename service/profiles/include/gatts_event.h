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
#ifndef __GATTS_EVENT_H__
#define __GATTS_EVENT_H__

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
    GATTS_EVENT_START,
    GATTS_EVENT_STOP,
    GATTS_EVENT_CONNECT_CHANGE,
    GATTS_EVENT_READ_REQUEST,
    GATTS_EVENT_WRITE_REQUEST,
    GATTS_EVENT_MTU,
    GATTS_EVENT_CHANGE_SEND,
} gatts_event_t;

typedef enum {
    GATTS_REQ_START,
    GATTS_REQ_STOP,
    GATTS_REQ_CONNECT,
    GATTS_REQ_DISCONNECT,
    GATTS_REQ_NOTIFY,
} gatts_request_t;

typedef struct
{
    gatts_event_t event;

    union {
        /**
         * @brief GATTS_EVENT_START
         */
        struct gatts_start_evt_param {
            uint16_t element_id;
            gatt_status_t status;
        } start;

        /**
         * @brief GATTS_EVENT_STOP
         */
        struct gatts_stop_evt_param {
            uint16_t element_id;
            gatt_status_t status;
        } stop;

        /**
         * @brief GATTS_EVENT_CONNECT_CHANGE
         */
        struct gatts_connect_change_evt_param {
            bt_address_t addr;
            int32_t connect_state;
            uint8_t reason;
        } connect_change;

        /**
         * @brief GATTS_EVENT_READ_REQUEST
         */
        struct gatts_read_evt_param {
            uint16_t element_id;
            uint32_t request_id;
            bt_address_t addr;
        } read;

        /**
         * @brief GATTS_EVENT_WRITER_REQUEST
         */
        struct gatts_write_evt_param {
            uint16_t element_id;
            uint32_t request_id;
            bt_address_t addr;
            bool need_rsp;
            uint16_t offset;
            uint16_t length;
            uint8_t value[0];
        } write;

        /**
         * @brief GATTS_EVENT_MTU
         */
        struct gatts_mtu_evt_param {
            bt_address_t addr;
            uint32_t mtu;
        } mtu;

        /**
         * @brief GATTS_EVENT_CHANGE_SEND
         */
        struct gatts_send_change_evt_param {
            uint16_t element_id;
            gatt_status_t status;
            bt_address_t addr;
        } change_send;

    } param;

} gatts_msg_t;

typedef struct
{
    gatts_request_t request;

    union {

        /**
         * @brief GATTS_REQ_START
         */
        struct gatts_start_req_param {
            void *srv_handle;
        } start;

        /**
         * @brief GATTS_REQ_STOP
         */
        struct gatts_stop_req_param {
            void *srv_handle;
        } stop;

        /**
         * @brief GATTS_REQ_CONNECT
         */
        struct gatts_connect_req_param {
            bt_address_t addr;
            ble_addr_type_t addr_type;
        } connect;

        /**
         * @brief GATTS_REQ_DISCONNECT
         */
        struct gatts_disconnect_req_param {
            void *srv_handle;
        } disconnect;

        /**
         * @brief GATTS_REQ_NOTIFY
         */
        struct gatts_notify_req_param {
            void *srv_handle;
            void *cmpl_cb;
            gatt_change_type_t type;
            uint16_t attr_handle;
        } notify;

    } param;

} gatts_op_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
gatts_msg_t *gatts_msg_new(gatts_event_t event, uint16_t playload_length);
void gatts_msg_destory(gatts_msg_t *msg);
gatts_op_t *gatts_op_new(gatts_request_t request);
void gatts_op_destory(gatts_op_t *operation);

#endif /* __GATTS_EVENT_H__ */
