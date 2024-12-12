/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __SAL_A2DP_SINK_INTERFACE_H__
#define __SAL_A2DP_SINK_INTERFACE_H__

#ifdef CONFIG_BLUETOOTH_A2DP

#include "a2dp_event.h"
#include "bt_device.h"

bt_status_t bt_sal_a2dp_sink_init(uint8_t max_connection);
void bt_sal_a2dp_sink_cleanup(void);
bt_status_t bt_sal_a2dp_sink_connect(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_a2dp_sink_disconnect(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_a2dp_sink_start_stream(bt_controller_id_t id, bt_address_t* addr);

void bt_sal_a2dp_sink_event_callback(a2dp_event_t* event);

#endif /* CONFIG_BLUETOOTH_A2DP */
#endif /* __SAL_A2DP_SINK_INTERFACE_H__ */
