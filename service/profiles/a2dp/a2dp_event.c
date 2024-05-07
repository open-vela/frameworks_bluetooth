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
#include <stdlib.h>
#include <string.h>

#include "a2dp_event.h"

a2dp_event_t* a2dp_event_new(a2dp_event_type_t event,
    bt_address_t* bd_addr)
{
    return a2dp_event_new_ext(event, bd_addr, NULL, 0);
}

a2dp_event_t* a2dp_event_new_ext(a2dp_event_type_t event,
    bt_address_t* bd_addr, void* data, size_t size)
{
    a2dp_event_t* a2dp_event;

    a2dp_event = (a2dp_event_t*)zalloc(sizeof(a2dp_event_t));
    if (a2dp_event == NULL)
        return NULL;

    a2dp_event->event = event;

    if (bd_addr != NULL)
        memcpy(&a2dp_event->event_data.bd_addr, bd_addr, sizeof(bt_address_t));

    if (size > 0) {
        a2dp_event->event_data.size = size;
        a2dp_event->event_data.data = malloc(size);
        memcpy(a2dp_event->event_data.data, data, size);
    }

    return a2dp_event;
}

void a2dp_event_destory(a2dp_event_t* a2dp_event)
{
    free(a2dp_event->event_data.data);
    free(a2dp_event);
}
