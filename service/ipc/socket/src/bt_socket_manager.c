/****************************************************************************
 * frameworks/media/media_daemon.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

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
#include "bt_message.h"
#include "bluetooth.h"
#include "service_loop.h"
#include "callbacks_list.h"
#include "bt_socket.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
  BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)


/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_manager_process(service_poll_t *poll,
    int fd, bt_instance_t *ins, bt_message_packet_t *packet)
{
  switch (packet->code)
  {
    case BT_MANAGER_CREATE_INSTANCE:
      {
        break;
      }
    
    default:
      break;
  }
}

int bt_socket_client_manager_callback(service_poll_t *poll,
    int fd, bt_instance_t *ins, bt_message_packet_t *packet)
{
  switch (packet->code)
  {
    default:
      return BT_STATUS_PARM_INVALID;
  }

  return BT_STATUS_SUCCESS;
}
