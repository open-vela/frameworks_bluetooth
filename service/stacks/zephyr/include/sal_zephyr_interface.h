/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef __SAL_ZEPHYR_INTERFACE_H_
#define __SAL_ZEPHYR_INTERFACE_H_

#include <stdint.h>

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_addr.h"
#include "bt_status.h"
#include "power_manager.h"
#include "vhal/bt_vhal.h"

#define GATT_ROLE_SERVER (1UL << 0)
#define GATT_ROLE_CLIENT (1UL << 1)

struct bt_conn* get_le_conn_from_addr(bt_address_t* addr);
bt_status_t get_le_addr_from_conn(struct bt_conn* conn, bt_address_t* addr);

#if defined(CONFIG_BT_USER_PHY_UPDATE)
ble_phy_type_t le_phy_convert_from_stack(uint8_t mode);
uint8_t le_phy_convert_from_service(ble_phy_type_t mode);
#endif

#endif /* __SAL_ZEPHYR_INTERFACE_H_ */