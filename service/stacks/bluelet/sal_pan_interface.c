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

#ifdef CONFIG_BLUETOOTH_PAN
#include "stack_adapter_common.h"
#include "stack_adapter_pan.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_pan_interface.h"

void connection_state_change_cb(BD_ADDR remote_addr, SERVICE_PAN_ROLE_TYPE remote_role,
                                SERVICE_PAN_ROLE_TYPE local_role,
                                SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    pan_on_connection_state_changed(&addr, remote_role, local_role,
                                    bluelet_profile_connection_state(state));
}

void data_received_cb(BD_ADDR remote_addr, uint16_t protocol,
                      uint8_t *dst_addr, uint8_t *src_addr,
                      uint8_t *data, uint16_t length)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    pan_on_data_received(&addr, protocol, dst_addr,
                         src_addr, data, length);
}

static PAN_CALLBACKS_S pan_cbks = {
    .size = sizeof(pan_cbks),
    .pan_connection_state_cb = connection_state_change_cb,
    .pan_data_received_cb = data_received_cb,
    /* Not support */
    .pan_protocol_filter_cb = NULL,
    /* Not support */
    .pan_multicast_filter_cb = NULL,
};

bt_status_t bt_sal_pan_init(pan_role_t role, uint8_t max_connection)
{
    SAL_CHECK_RET(service_adapter_pan_init(max_connection, role, &pan_cbks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_pan_cleanup(void)
{
    service_adapter_pan_cleanup();
}

bt_status_t bt_sal_pan_connect(bt_address_t *addr,
                               pan_role_t dst_role,
                               pan_role_t src_role)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_pan_connect(addr->addr, dst_role, src_role),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pan_disconnect(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_pan_disconnect(addr->addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pan_write(bt_address_t *addr, uint16_t protocol,
                             uint8_t *dst_addr, uint8_t *src_addr,
                             uint8_t *buffer, uint16_t length)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_pan_write(addr->addr, protocol,
                                            dst_addr, src_addr,
                                            buffer, length),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}
#endif
