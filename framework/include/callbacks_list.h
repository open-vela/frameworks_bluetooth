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

#ifndef __SERVICE_COMMON_CALLBACKS_LIST_H
#define __SERVICE_COMMON_CALLBACKS_LIST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "bt_list.h"

typedef struct callback {
    void* remote;
    void* callbacks;
} remote_callback_t;

typedef struct {
    uint8_t max_reg;
    uint8_t registed;
    bt_list_t* list;
    pthread_mutex_t lock;
} callbacks_list_t;

#define BT_CALLBACK_FOREACH(_cbsl, _type, _cback, args...)                                     \
    do {                                                                                       \
        callbacks_list_t* __cbsl = _cbsl;                                                      \
        if (__cbsl == NULL)                                                                    \
            break;                                                                             \
        bt_list_node_t* _node;                                                                 \
        bt_list_t* _list = __cbsl->list;                                                       \
        pthread_mutex_lock(&__cbsl->lock);                                                     \
        for (_node = bt_list_head(_list); _node != NULL; _node = bt_list_next(_list, _node)) { \
            remote_callback_t* _rcbk = (remote_callback_t*)bt_list_node(_node);                \
            _type* _cbs = (_type*)_rcbk->callbacks;                                            \
            void* _remote = _rcbk->remote ? _rcbk->remote : _rcbk;                             \
            if (_cbs && _cbs->_cback)                                                          \
                _cbs->_cback(_remote, args);                                                   \
        }                                                                                      \
        pthread_mutex_unlock(&__cbsl->lock);                                                   \
    } while (0)

callbacks_list_t* bt_callbacks_list_new(uint8_t max);
remote_callback_t* bt_callbacks_register(callbacks_list_t* cbsl, void* callbacks);
bool bt_callbacks_unregister(callbacks_list_t* cbsl, remote_callback_t* rcbks);
remote_callback_t* bt_remote_callbacks_register(callbacks_list_t* cbsl, void* remote, void* callbacks);
bool bt_remote_callbacks_unregister(callbacks_list_t* cbsl, void** remote, remote_callback_t* rcbks);
void bt_callbacks_foreach(callbacks_list_t* cbsl, void* context);
void bt_callbacks_list_free(void* cbsl);
uint8_t bt_callbacks_list_count(callbacks_list_t* cbsl);

#endif
