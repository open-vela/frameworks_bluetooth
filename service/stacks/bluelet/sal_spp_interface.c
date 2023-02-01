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
#include "stack_adapter_spp.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_spp_interface.h"

#ifdef CONFIG_BLUETOOTH_SPP
static void connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
                                        SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    spp_on_connection_state_changed(&addr, conn_port, bluelet_profile_connection_state(state));
}

static void data_sent_cb(SERVICE_SPP_PORT conn_port, uint8_t *buffer, uint16_t length,
                         uint16_t sent_length)
{
    spp_on_data_sent(conn_port, buffer, length, sent_length);
}

static void data_received_cb(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
                             uint8_t *buffer, uint16_t length)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    spp_on_data_received(&addr, conn_port, buffer, length);
}

static void connection_mfs_cb(SERVICE_SPP_PORT conn_port, uint16_t mfs)
{
    spp_on_connection_mfs_update(conn_port, mfs);
}

static void server_connection_req_received_cb(BD_ADDR remote_addr,
                                              SERVICE_SPP_PORT svr_port)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    spp_on_server_recieve_connect_request(&addr, svr_port);
}

static SPP_CALLBACKS_S spp_callbacks = {
    .size = sizeof(spp_callbacks),
    .spp_connection_state_changed_cb = connection_state_changed_cb,
    .spp_data_sent_cb = data_sent_cb,
    .spp_data_received_cb = data_received_cb,
    .spp_server_connection_req_received_cb = server_connection_req_received_cb,
    .spp_connection_mfs_cb = connection_mfs_cb,
};

bt_status_t bt_sal_spp_init(void)
{
    SAL_CHECK_RET(service_adapter_spp_init(&spp_callbacks), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_spp_cleanup(void)
{
    service_adapter_spp_cleanup();
}

bt_status_t bt_sal_spp_server_start(uint16_t scn, bt_uuid_t *uuid128, uint8_t max_conn_cnt)
{
    if (scn == 0 || scn > 29 || max_conn_cnt > 31)
        return BT_STATUS_PARM_INVALID;

    SAL_CHECK_RET(service_adapter_spp_server_open(scn, uuid128 ? uuid128->val.u128 : NULL, max_conn_cnt),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_server_stop(uint16_t scn)
{
    if (scn == 0 || scn > 29)
        return BT_STATUS_PARM_INVALID;

    SAL_CHECK_RET(service_adapter_spp_server_close(scn), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_connect(bt_address_t *addr, uint16_t conn_port, bt_uuid_t *uuid128)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_spp_client_open(addr->addr, conn_port, uuid128 ? uuid128->val.u128 : NULL),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_disconnect(uint16_t conn_port)
{
    SAL_CHECK_RET(service_adapter_spp_client_close(conn_port), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

// SERVICE_BT_STATUS service_adapter_spp_disconnect_by_remote_addr(BD_ADDR remote_addr);

bt_status_t bt_sal_spp_write(uint16_t conn_port, uint8_t *buffer, uint16_t length)
{
    SAL_CHECK_PARAM(buffer);

    SAL_CHECK_RET(service_adapter_spp_write(conn_port, buffer, length), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_add_credits(uint16_t conn_port, uint8_t credits)
{
    SAL_CHECK_RET(service_adapter_spp_add_credits(conn_port, credits), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_data_received_response(uint16_t conn_port, uint8_t *buffer)
{
    SAL_CHECK_PARAM(buffer);

    SAL_CHECK_RET(service_adapter_spp_data_received_rsp(conn_port, buffer), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_connect_request_reply(bt_address_t *addr, uint16_t conn_port, bool accept)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_spp_send_connection_rsp(addr->addr, conn_port, accept), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}
#endif
