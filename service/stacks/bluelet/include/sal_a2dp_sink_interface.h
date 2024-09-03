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
#ifndef __SAL_A2DP_SINK_INTERFACE_H__
#define __SAL_A2DP_SINK_INTERFACE_H__

#include <stdint.h>

#include "a2dp_sink_service.h"
#include "bt_addr.h"
#include "bt_status.h"

#include "a2dp_audio.h"
#include "a2dp_codec.h"
#include "a2dp_device.h"
#include "a2dp_event.h"
#include "a2dp_sink.h"
#include "a2dp_state_machine.h"

#include "bt_utils.h"

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
#define A2DP_PREFERRED_CODEC SERVICE_AVDTP_CODEC_TYPE_MPEG2_4_AAC
#else
#define A2DP_PREFERRED_CODEC SERVICE_AVDTP_CODEC_TYPE_SBC
#endif

bt_status_t bt_sal_a2dp_sink_init(uint8_t max_connection);
void bt_sal_a2dp_sink_cleanup(void);
bt_status_t bt_sal_a2dp_sink_connect(bt_address_t* addr);
bt_status_t bt_sal_a2dp_sink_disconnect(bt_address_t* addr);
bt_status_t bt_sal_a2dp_sink_set_active_device(bt_address_t* addr);
bt_status_t bt_sal_a2dp_sink_start_stream(bt_address_t* addr);

void bt_sal_a2dp_sink_event_callback(a2dp_event_t* event);

#endif /* __SAL_A2DP_SINK_INTERFACE_H__ */
