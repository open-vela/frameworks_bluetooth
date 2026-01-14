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

#ifndef __LE_CS_MANAGER_H__
#define __LE_CS_MANAGER_H__

#include <stdint.h>
#include <zephyr/bluetooth/cs.h>

 void bt_le_cs_run_distance_estimation(uint8_t *local_step_data, uint16_t local_data_len,
                             uint8_t *peer_step_data, uint16_t peer_data_len,
                             uint8_t antenna_count, enum bt_conn_le_cs_role device_role);


#endif //__LE_CS_MANAGER_H__