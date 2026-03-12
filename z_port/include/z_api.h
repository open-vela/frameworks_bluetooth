/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#ifndef __Z_API_H__
#define __Z_API_H__

/**
 * @brief Macro to define z_port API function names
 *
 * This macro is used to create function names that correspond to
 * Zephyr Bluetooth API functions with a 'z_' prefix.
 *
 * Example:
 *   z_api(bt_gatt_discover) expands to z_bt_gatt_discover
 *   z_api(bt_le_adv_start) expands to z_bt_le_adv_start
 *
 * Usage in autopts tester:
 *   Replace Zephyr API calls with z_api() macro:
 *   - bt_enable(NULL)           -> z_api(bt_enable)(NULL)
 *   - bt_le_adv_start(...)      -> z_api(bt_le_adv_start)(...)
 *   - bt_gatt_discover(...)     -> z_api(bt_gatt_discover)(...)
 *   - bt_l2cap_chan_connect(...) -> z_api(bt_l2cap_chan_connect)(...)
 */
#define z_api(name) z_##name

#endif /* __Z_API_H__ */
