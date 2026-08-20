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

/* All BNEP frame/control/response constants live in bnep_codec.h, which is
 * zero-dependency so the same header compiles in the host unit test.
 * Do not re-declare them here: the previous local copies had the response
 * codes wrong from 0x0003 up and gave 0x02 two conflicting meanings. */
#include "bnep_codec.h"

bt_status_t bt_sal_pan_init(uint8_t max_connections, uint8_t role);
void bt_sal_pan_cleanup(void);
bt_status_t bt_sal_pan_connect(bt_address_t* addr, uint8_t dst_role, uint8_t src_role);
bt_status_t bt_sal_pan_disconnect(bt_address_t* addr);
bt_status_t bt_sal_pan_write_eth(const bt_address_t* addr,
    const uint8_t* eth_frame, uint16_t eth_len);
uint16_t bt_sal_pan_get_tx_mtu(const bt_address_t* addr);

#endif /* __SAL_PAN_INTERFACE_H__ */
