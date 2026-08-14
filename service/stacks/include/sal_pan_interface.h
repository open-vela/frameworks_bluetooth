/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#ifndef __SAL_PAN_INTERFACE_H__
#define __SAL_PAN_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "pan_service.h"

/* BNEP PSM (Bluetooth Core Spec Vol 3 Part E) */
#define BT_BNEP_PSM 0x000F

/* BNEP control types */
#define BNEP_FRAME_ETH            0x00 /* Ethernet frame (uncompressed) */
#define BNEP_SETUP_CONN_REQ       0x01
#define BNEP_SETUP_CONN_RESP      0x02
#define BNEP_FILTER_NET_TYPE_SET  0x03
#define BNEP_FILTER_NET_TYPE_RESP 0x04
#define BNEP_FILTER_MULTI_ADDR_SET 0x05
#define BNEP_FILTER_MULTI_ADDR_RESP 0x06
#define BNEP_EXT_CONTROL          0x7F

/* BNEP setup connection response codes */
#define BNEP_CONN_RESP_SUCCESS      0x0000
#define BNEP_CONN_RESP_FAIL_INVALID_DEST_ROLE 0x0001
#define BNEP_CONN_RESP_FAIL_INVALID_SRC_ROLE  0x0002
#define BNEP_CONN_RESP_FAIL_CONN_NOT_ALLOWED  0x0003
#define BNEP_CONN_RESP_FAIL_CONN_NOT_CONNECTED 0x0004
#define BNEP_CONN_RESP_FAIL_INVALID_DEV_ADDR  0x0005
#define BNEP_CONN_RESP_FAIL_CONN_FAILED      0x0006
#define BNEP_CONN_RESP_FAIL_REQ_NOT_SUPPORTED 0x0007

bt_status_t bt_sal_pan_init(uint8_t max_connections, uint8_t role);
void bt_sal_pan_cleanup(void);
bt_status_t bt_sal_pan_connect(bt_address_t* addr, uint8_t dst_role, uint8_t src_role);
bt_status_t bt_sal_pan_disconnect(bt_address_t* addr);
bt_status_t bt_sal_pan_write(bt_address_t* addr, uint16_t protocol,
    uint8_t* dst_addr, uint8_t* src_addr,
    uint8_t* data, uint16_t length);

#endif /* __SAL_PAN_INTERFACE_H__ */
