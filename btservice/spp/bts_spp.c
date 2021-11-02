/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <pty.h>
#include <sys/types.h>

#include <nuttx/list.h>

#include "uv.h"
// bluelet dependent
#include "stack_adapter_service_base.h"
#include "stack_adapter_spp.h"

// internel dependent
#include "btm_manager.h"
#include "bts_service.h"

#include "uuid.h"
#include "bts_spp.h"
#include "btm_spp.h"

#define LOG_TAG "bts_spp"
#include "log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONNECTIONS_MAX       CONFIG_BLUETOOTH_SPP_MAX_CONNECTIONS
#define SERVER_CONNECTION_MAX CONFIG_BLUETOOTH_SPP_SERVER_MAX_CONNECTIONS
#define CONNECTIONS_BASE      (1 << 6)
#define INDEX_MAX             (CONNECTIONS_MAX >> 5)
#define INVALID_FD            -1

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  uint32_t                conn_id_map[INDEX_MAX];
  uint8_t                 conn_id_next;
  struct list_node        dev_list;
  spp_service_callbacks_t *cbs;
} spp_handle_t;

typedef struct
{
  struct list_node        node;
  uv_poll_t               *handle;
  bool                    accept;
  bt_address              addr;
  uint16_t                svr_port;
  uint16_t                conn_port;
  int                     mfd;
  int                     sfd;
  char                    pty_name[20];
  spp_connection_state_t  state;
} spp_pty_device_t;

typedef struct
{
  enum
  {
    STATE_CHANEG = 0,
    DATA_SENT,
    DATA_RECEIVED,
    CONN_REQ_RECEIVED,
  } event;
  bt_address  addr;
  uint16_t    port;
  uint8_t     *buffer;
  uint16_t    length;
  uint16_t    sent_length;
  spp_connection_state_t state;
} spp_adapter_msg_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static void spp_pty_poll_callback(uv_poll_t *req, int status, int events);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static spp_handle_t g_spp_handle;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
uint8_t alloc_connection_port(uint8_t svr_port)
{
  uint8_t conn_id = 0;
#if 0
  uint8_t start = g_spp_handle.conn_id_next;
  int index;
  int bitno;

  for (;;)
  {
    conn_id = g_spp_handle.conn_id_next;
    if ((g_spp_handle.conn_id_next - CONNECTIONS_BASE) >= CONNECTIONS_MAX)
    {
      g_spp_handle.conn_id_next = CONNECTIONS_BASE;
    }
    else
    {
      g_spp_handle.conn_id_next++;
    }
    if ((conn_id & 0x3F) != svr_port)
      continue;

    index = (conn_id - CONNECTIONS_BASE) >> 5;
    bitno = (conn_id - CONNECTIONS_BASE) & 31;
    if ((g_spp_handle.conn_id_map[index] & (1 << bitno)) == 0)
    {
      g_spp_handle.conn_id_map[index] |= (1 << bitno);
      return (int)conn_id;
    }

    if (start == conn_id)
    {
      return -ENOMEM;
    }
  }
#else
  for (; (conn_id < CONNECTIONS_MAX) && ((1 << conn_id) & g_spp_handle.conn_id_map[0]); conn_id++)
    ;
  if (conn_id < CONNECTIONS_MAX) {
    g_spp_handle.conn_id_map[0] |= (1 << conn_id);
    return (svr_port + (conn_id << 6));
  }

  return -ENOMEM;
#endif
}

void free_connection_port(uint8_t conn_id)
{
#if 0
  int index;
  int bitno;

  index = (conn_id - CONNECTIONS_BASE) >> 5;
  bitno = (conn_id - CONNECTIONS_BASE) & 31;

  //DEBUGASSERT((g_spp_handle.conn_id_map[index] |= (1 << bitno)) != 0);
  g_spp_handle.conn_id_map[index] &= ~(1 << bitno);

  if (g_spp_handle.conn_id_next)
    {
      g_spp_handle.conn_id_next = conn_id;
    }
#else
  if (conn_id < CONNECTIONS_MAX) {
    g_spp_handle.conn_id_map[0] &= ~(1 << conn_id);
  }
#endif
}

static spp_pty_device_t *alloc_new_device(bt_address addr, uint16_t port, bool accept)
{
  spp_pty_device_t *device =
      (spp_pty_device_t *)malloc(sizeof(spp_pty_device_t));
  if (device == NULL)
    return NULL;

  device->svr_port = port;
  device->conn_port = alloc_connection_port(port);
  if (!device->conn_port) {
    free(device);
    return NULL;
  }
  device->accept = accept;
  device->handle = NULL;
  device->mfd = INVALID_FD;
  device->sfd = INVALID_FD;
  device->state = SPP_CONNECTION_STATE_DISCONNECTED;
  memcpy(device->addr, addr, sizeof(device->addr));
  list_add_tail(&g_spp_handle.dev_list, &device->node);

  return device;
}

static spp_pty_device_t *find_pty_device(bt_address addr, uint16_t port)
{
  struct list_node *list = &g_spp_handle.dev_list;
  spp_pty_device_t *device;
  struct list_node *node;

  list_for_every(list, node) {
    device = (spp_pty_device_t *)node;
    if (memcmp(addr, device->addr, 6) == 0 && port == device->conn_port)
      return device;
  }

  return NULL;
}

static spp_pty_device_t *find_pty_device_by_handle(uv_poll_t *handle)
{
  struct list_node *list = &g_spp_handle.dev_list;
  spp_pty_device_t *device;
  struct list_node *node;

  list_for_every(list, node) {
    device = (spp_pty_device_t *)node;
    if (device->handle == handle)
      return device;
  }

  return NULL;
}

static void remove_pty_device(spp_pty_device_t *device)
{
  free_connection_port(device->conn_port);

  list_delete(&device->node);
  free(device);
}

static spp_pty_device_t *spp_open_pty_device(bt_address addr, uint16_t port)
{
  int ret;
  spp_pty_device_t *device;
  struct termios tio;

  device = find_pty_device(addr, port);
  if (device == NULL)
    return NULL;

  //ret = openpty(&device->mfd, &device->sfd, device->pty_name, NULL, NULL);
  if (ret != 0) {
    BT_LOGE("pty create failed");
    remove_pty_device(device);
    return NULL;
  }
  //set mfd raw data
  ioctl(device->mfd, TCGETS, &tio);
  cfmakeraw(&tio);
  ioctl(device->mfd, TCSETS, &tio);
  //set sfd raw data
  ioctl(device->sfd, TCGETS, &tio);
  cfmakeraw(&tio);
  ioctl(device->sfd, TCSETS, &tio);
  BT_LOGD("pty create success, name:%s, master:%d, slave:%d",
          device->pty_name, device->mfd, device->sfd);
  return device;
}

static void spp_close_pty_device(spp_pty_device_t *device)
{
  if (device->mfd != -1) {
    close(device->mfd);
    device->mfd = -1;
  }

  if (device->sfd != -1) {
    close(device->sfd);
    device->sfd = -1;
  }

  device->handle = NULL;
  if (device->state == SPP_CONNECTION_STATE_CONNECTED)
    service_adapter_spp_disconnect_by_port(device->conn_port);

  remove_pty_device(device);
}

static void spp_close_all_device(void)
{
  struct list_node *list = &g_spp_handle.dev_list;
  spp_pty_device_t *device;
  struct list_node *node;
  struct list_node *tmp;

  list_for_every_safe(list, node, tmp) {
    device = (spp_pty_device_t *)node;
    spp_close_pty_device(device);
  }
}

static void spp_notify_connection_state(bt_address addr, uint16_t port, spp_connection_state_t state)
{
  if (g_spp_handle.cbs && g_spp_handle.cbs->connection_state_cb)
    g_spp_handle.cbs->connection_state_cb(addr, port, state);
}

static void spp_notify_pty_open(bt_address addr, uint16_t port, char *name, int fd)
{
  if (g_spp_handle.cbs && g_spp_handle.cbs->pty_open_cb)
    g_spp_handle.cbs->pty_open_cb(addr, port, name, fd);
}

static void spp_on_connection_state_chaneged(bt_address addr, uint16_t port,
                                             spp_connection_state_t state)
{
  spp_pty_device_t *device;

  BT_LOGD("%s, addr: %02x:%02x:%02x:%02x:%02x:%02x, port: %d, state: %d", __func__, addr[0],
          addr[1], addr[2], addr[3], addr[4], addr[5], port, state);
  spp_notify_connection_state(addr, port, state);

  if (state == SPP_CONNECTION_STATE_CONNECTED) {
    device = spp_open_pty_device(addr, port);
    if (device == NULL)
      return;
    device->state = SPP_CONNECTION_STATE_CONNECTED;
    device->handle = bts_uv_poll_start(device->mfd, UV_READABLE | UV_DISCONNECT, spp_pty_poll_callback);
    if (device->handle == NULL) {
      spp_close_pty_device(device);
      return;
    }
    spp_notify_pty_open(addr, port, device->pty_name, device->sfd);
  }
  else if (state == SPP_CONNECTION_STATE_DISCONNECTED) {
    device = find_pty_device(addr, port);
    if (!device)
      return;
    device->state = SPP_CONNECTION_STATE_DISCONNECTED;
    spp_close_pty_device(device);
  }
}

static int spp_on_incoming_data_received(bt_address addr, uint16_t port,
                                         uint8_t *buffer, uint16_t length)
{
  int ret;
  spp_pty_device_t *device;

  BT_LOGD("%s", __func__);
  device = find_pty_device(addr, port);
  if (!device)
    return -1;

  lib_dumpbuffer("s write:", buffer, length);
  do {
    ret = write(device->sfd, buffer, length);
  } while (ret == -1 && errno == EINTR);

  if (!ret) {
    BT_LOGE("spp write to slave port failed");
    return ret;
  }
  if (ret != length) {
    BT_LOGE("spp write ret: %d, length: %d", ret, length);
  }

  return 0;
}

static void do_spp_write(spp_pty_device_t *device, uint16_t length)
{
  uint16_t remaining = length;
  uint8_t size, ret;
  uint8_t *tmp;

  if (!device)
    return;

  do {
      size = remaining > 255 ? 255 : remaining;
      tmp = (uint8_t *)malloc(size);
      ret = read(device->mfd, tmp, size);
      lib_dumpbuffer("m read:", tmp, ret);
      service_adapter_spp_write(device->conn_port, tmp, ret);
      remaining -= ret;
  } while (remaining);
}

static void spp_pty_poll_callback(uv_poll_t *req, int status, int events)
{
  if (status != 0)
    return;

  if (events & UV_READABLE) {
    size_t size;

    spp_pty_device_t *device = find_pty_device_by_handle(req);
    if (!device)
      return;
    if (ioctl(device->mfd, FIONREAD, &size) == 0 && size) {
      do_spp_write(device, size);
    }
  }
  if (events & UV_WRITABLE) {
    //todo:implement write list
  }

  if (events & UV_DISCONNECT) {
    bts_uv_poll_stop(req);
    spp_pty_device_t *device = find_pty_device_by_handle(req);
    if (device)
      spp_close_pty_device(device);
  }
}

static void spp_adapter_event_process(void *data, size_t size)
{
  BT_LOGD("%s", __func__);
  if (!data || !size)
    return;
  spp_adapter_msg_t *msg = (spp_adapter_msg_t *)data;

  switch (msg->event) {
    case STATE_CHANEG: {
      spp_on_connection_state_chaneged(msg->addr, msg->port, msg->state);
      break;
    }
    case DATA_SENT: {
      if (msg->buffer && msg->length)
        free(msg->buffer);
      break;
    }
    case DATA_RECEIVED: {
      spp_on_incoming_data_received(msg->addr, msg->port, msg->buffer, msg->length);
      service_adapter_spp_data_received_rsp(msg->port, msg->buffer);
      break;
    }
    case CONN_REQ_RECEIVED: {
      spp_pty_device_t *device;

      device = alloc_new_device(msg->addr, msg->port, true);
      if (device) {
        BT_LOGD("CONN_REQ_RECEIVED: svr_port:%d, conn:%d", msg->port, device->conn_port);
        service_adapter_spp_send_connection_rsp(msg->addr, device->conn_port, true);
      } else {
        BT_LOGW("device alloc failed, reject connection: svr_port:%d,", msg->port);
        service_adapter_spp_send_connection_rsp(msg->addr, device->conn_port, false);
      }
      break;
    }
    default:
      break;
  }

  free(data);
}

static void spp_adp_send_to_service(spp_adapter_msg_t *msg)
{
  excute_service_context_t *context = (excute_service_context_t *)malloc(sizeof(excute_service_context_t));
  spp_adapter_msg_t *spp_msg = (spp_adapter_msg_t *)malloc(sizeof(spp_adapter_msg_t));

  memcpy(spp_msg, msg, sizeof(spp_adapter_msg_t));
  context->loop_func = spp_adapter_event_process;
  context->data = (void *)spp_msg;
  context->data_size = sizeof(spp_adapter_msg_t);
  process_in_loop(context);
}

static void adp_connection_state_changed_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
                                                  SERVICE_PROFILE_CONNECTION_STATE state)
{
  spp_adapter_msg_t msg;
  spp_connection_state_t conn_state;

  switch (state) {
  case SERVICE_PROFILE_DISCONNECTED:
    conn_state = SPP_CONNECTION_STATE_DISCONNECTED;
    break;
  case SERVICE_PROFILE_CONNECTING:
    conn_state = SPP_CONNECTION_STATE_CONNECTING;
    break;
  case SERVICE_PROFILE_CONNECTED:
    conn_state = SPP_CONNECTION_STATE_CONNECTED;
    break;
  case SERVICE_PROFILE_DISCONNECTING:
    conn_state = SPP_CONNECTION_STATE_DISCONNECTING;
    break;
  }
  msg.event = STATE_CHANEG;
  msg.state = conn_state;
  msg.port = conn_port;
  memcpy(msg.addr, remote_addr, sizeof(bt_address));

  spp_adp_send_to_service(&msg);
}

static void adp_data_sent_callback(SERVICE_SPP_PORT conn_port, uint8_t *buffer, uint16_t length,
                                   uint16_t sent_length)
{
  spp_adapter_msg_t msg;

  msg.event = DATA_SENT;
  msg.port = conn_port;
  msg.length = length;
  msg.sent_length = sent_length;
  msg.buffer = buffer;

  spp_adp_send_to_service(&msg);
}

static void adp_data_received_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port, uint8_t *buffer, uint16_t length)
{
  spp_adapter_msg_t msg;

  msg.event = DATA_RECEIVED;
  msg.port = conn_port;
  msg.length = length;
  msg.buffer = buffer;
  memcpy(msg.addr, remote_addr, sizeof(bt_address));

  spp_adp_send_to_service(&msg);
}

static void adp_server_connection_req_received_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT svr_port)
{
  spp_adapter_msg_t msg;

  msg.event = CONN_REQ_RECEIVED;
  msg.port = svr_port;
  memcpy(msg.addr, remote_addr, sizeof(bt_address));

  spp_adp_send_to_service(&msg);
}

static SPP_CALLBACKS_S spp_adp_callbacks = {
    sizeof(SPP_CALLBACKS_S),
    adp_connection_state_changed_callback,
    adp_data_sent_callback,
    adp_data_received_callback,
    adp_server_connection_req_received_callback,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bt_result_code bts_spp_init(spp_service_callbacks_t *callbacks)
{
  SERVICE_BT_STATUS status;

  g_spp_handle.cbs = callbacks;
  list_initialize(&g_spp_handle.dev_list);
  g_spp_handle.conn_id_next = CONNECTIONS_BASE;
  memset(&g_spp_handle.conn_id_map, 0, sizeof(g_spp_handle.conn_id_map));

  status = service_adapter_spp_init(&spp_adp_callbacks);
  if (status != SERVICE_BT_STATUS_SUCCESS) {
    list_delete(&g_spp_handle.dev_list);
    return BT_RESULT_FAILED;
  }

  return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_server_start(uint16_t port, uint16_t uuid)
{
  SERVICE_BT_STATUS status;
  struct bt_uuid_16 uuid_src;
  struct bt_uuid_128 uuid_128_dst;

  bt_uuid_create((struct bt_uuid *)&uuid_src, (uint8_t *)&uuid, 2);
  uuid_to_uuid128((struct bt_uuid *)&uuid_src, &uuid_128_dst);
  status = service_adapter_spp_server_open(port, uuid_128_dst.val, SERVER_CONNECTION_MAX);
  if (status != SERVICE_BT_STATUS_SUCCESS)
    return BT_RESULT_FAILED;

  return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_server_stop(uint16_t port)
{
  SERVICE_BT_STATUS status;
  status = service_adapter_spp_server_close(port);
  if (status != SERVICE_BT_STATUS_SUCCESS)
    return BT_RESULT_FAILED;

  return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_client_connect(bt_address addr, uint16_t port, uint16_t uuid)
{
  SERVICE_BT_STATUS status;
  spp_pty_device_t *device;
  struct bt_uuid_16 uuid_src;
  struct bt_uuid_128 uuid_128_dst;

  bt_uuid_create((struct bt_uuid *)&uuid_src, (uint8_t *)&uuid, 2);
  uuid_to_uuid128((struct bt_uuid *)&uuid_src, &uuid_128_dst);

  device = alloc_new_device(addr, 0, false);
  if (!device)
    return BT_RESULT_ALLOC_BUFFER_FAILED;

  status = service_adapter_spp_client_open(addr, device->conn_port, uuid_128_dst.val);
  if (status != SERVICE_BT_STATUS_SUCCESS) {
    //spp_notify_connection_state(addr, device->conn_port, SPP_CONNECTION_STATE_DISCONNECTED);
    return BT_RESULT_FAILED;
  }
  // todo: start connect timer, release device if timeout
  device->state = SPP_CONNECTION_STATE_CONNECTING;

  return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_disconnect(bt_address addr, uint16_t port)
{
  SERVICE_BT_STATUS status;
  spp_pty_device_t *device = find_pty_device(addr, port);
  if (device == NULL)
    return BT_RESULT_SUCCESS;

  device->state = SPP_CONNECTION_STATE_DISCONNECTING;
  status = service_adapter_spp_disconnect_by_port(port);
  if (status != SERVICE_BT_STATUS_SUCCESS)
    return BT_RESULT_FAILED;

  return BT_RESULT_SUCCESS;
}

void bts_spp_cleanup(void)
{
  spp_close_all_device();
  list_delete(&g_spp_handle.dev_list);
  service_adapter_spp_cleanup();
}
