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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "bt_internal.h"

#include "adapter_internel.h"
#include "bluetooth.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "bt_trace.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_log_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_LOG_ENABLE: {
        BTSYMBOLS(bluetooth_enable_btsnoop_log)
        (ins);
        break;
    }
    case BT_LOG_DISABLE: {
        BTSYMBOLS(bluetooth_disable_btsnoop_log)
        (ins);
        break;
    }
    case BT_LOG_SET_FILTER: {
        BTSYMBOLS(bluetooth_set_btsnoop_filter)
        (ins, packet->log_pl._bt_log_set_flag.filter_flag);
        break;
    }
    case BT_LOG_REMOVE_FILTER: {
        BTSYMBOLS(bluetooth_remove_btsnoop_filter)
        (ins, packet->log_pl._bt_log_remove_flag.filter_flag);
        break;
    }
    default:
        break;
    }
}

#endif