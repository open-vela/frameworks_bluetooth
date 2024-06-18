/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
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
#define LOG_TAG "a2dp_device"
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "a2dp_device.h"
#include "a2dp_state_machine.h"
#include "bt_utils.h"
#include "utils/log.h"

a2dp_device_t* find_a2dp_device_by_addr(struct list_node* list, bt_address_t* bd_addr)
{
    a2dp_device_t* device;
    struct list_node* node;

    list_for_every(list, node)
    {
        device = (a2dp_device_t*)node;
        if (memcmp(&device->bd_addr, bd_addr, sizeof(bt_address_t)) == 0)
            return device;
    }

    return NULL;
}

a2dp_device_t* a2dp_device_new(void* ctx, uint8_t peer_sep, bt_address_t* bd_addr)
{
    a2dp_device_t* device;
    a2dp_state_machine_t* a2dp_sm;

    device = (a2dp_device_t*)malloc(sizeof(a2dp_device_t));
    if (!device)
        return NULL;

    memcpy(&device->bd_addr, bd_addr, sizeof(bt_address_t));
    device->peer.bd_addr = &device->bd_addr;
    a2dp_sm = a2dp_state_machine_new(ctx, peer_sep, bd_addr);
    if (!a2dp_sm) {
        BT_LOGE("Create state machine failed");
        free(device);
        return NULL;
    }

    device->a2dp_sm = a2dp_sm;

    return device;
}

void a2dp_device_delete(a2dp_device_t* device)
{
    a2dp_event_t* a2dp_event;

    if (!device)
        return;

    a2dp_event = a2dp_event_new(DISCONNECT_REQ, NULL);
    a2dp_state_machine_handle_event(device->a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);

    a2dp_event = a2dp_event_new(DISCONNECTED_EVT, NULL);
    a2dp_state_machine_handle_event(device->a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);

    a2dp_state_machine_destory(device->a2dp_sm);
    list_delete(&device->node);
    free((void*)device);
}
