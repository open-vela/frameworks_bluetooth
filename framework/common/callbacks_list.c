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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <assert.h>
#include <stdlib.h>

#include "callbacks_list.h"

static bool callback_is_found(void* data, void* context)
{
    remote_callback_t* rcbk = data;

    return rcbk->callbacks == context;
}

static bool remote_is_found(void* data, void* context)
{
    remote_callback_t* rcbk = data;

    return rcbk->remote == context;
}

callbacks_list_t* bt_callbacks_list_new(uint8_t max)
{
    pthread_mutexattr_t attr;
    callbacks_list_t* cbsl = malloc(sizeof(callbacks_list_t));

    if (!cbsl)
        return NULL;

    cbsl->list = bt_list_new(free);
    if (!cbsl->list) {
        free(cbsl);
        return NULL;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&cbsl->lock, &attr) < 0) {
        bt_list_free(cbsl->list);
        free(cbsl);
        return NULL;
    }

    cbsl->max_reg = max;
    cbsl->registed = 0;

    return cbsl;
}

remote_callback_t* bt_callbacks_register(callbacks_list_t* cbsl, void* callbacks)
{
    return bt_remote_callbacks_register(cbsl, NULL, callbacks);
}

bool bt_callbacks_unregister(callbacks_list_t* cbsl, remote_callback_t* rcbks)
{
    return bt_remote_callbacks_unregister(cbsl, NULL, rcbks);
}

remote_callback_t* bt_remote_callbacks_register(callbacks_list_t* cbsl, void* remote, void* callbacks)
{
    void* cbs;
    remote_callback_t* remote_cbk;

    assert(cbsl);

    pthread_mutex_lock(&cbsl->lock);
    if (cbsl->registed == cbsl->max_reg) {
        pthread_mutex_unlock(&cbsl->lock);
        return NULL;
    }

    if (remote)
        cbs = bt_list_find(cbsl->list, remote_is_found, remote);
    else
        cbs = bt_list_find(cbsl->list, callback_is_found, callbacks);

    if (cbs) {
        pthread_mutex_unlock(&cbsl->lock);
        return NULL;
    }

    remote_cbk = malloc(sizeof(*remote_cbk));
    remote_cbk->remote = remote;
    remote_cbk->callbacks = callbacks;

    bt_list_add_tail(cbsl->list, remote_cbk);
    cbsl->registed++;
    pthread_mutex_unlock(&cbsl->lock);

    return remote_cbk;
}

bool bt_remote_callbacks_unregister(callbacks_list_t* cbsl, void** remote, remote_callback_t* rcbks)
{
    bt_list_node_t* node;
    bt_list_t* list;

    assert(cbsl);

    list = cbsl->list;
    pthread_mutex_lock(&cbsl->lock);
    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        remote_callback_t* cbs = (remote_callback_t*)bt_list_node(node);
        if (rcbks == cbs) {
            cbsl->registed--;
            if (remote)
                *remote = cbs->remote;
            bt_list_remove_node(list, node);
            pthread_mutex_unlock(&cbsl->lock);
            return true;
        }
    }
    pthread_mutex_unlock(&cbsl->lock);

    return false;
}

void bt_callbacks_foreach(callbacks_list_t* cbsl, void* context)
{
}

void bt_callbacks_list_free(void* data)
{
    callbacks_list_t* cbsl = data;
    if (!cbsl)
        return;

    pthread_mutex_lock(&cbsl->lock);
    bt_list_free(cbsl->list);
    pthread_mutex_unlock(&cbsl->lock);
    pthread_mutex_destroy(&cbsl->lock);
    free(cbsl);
}

uint8_t bt_callbacks_list_count(callbacks_list_t* cbsl)
{
    uint8_t registed;

    assert(cbsl);

    pthread_mutex_lock(&cbsl->lock);
    registed = cbsl->registed;
    pthread_mutex_unlock(&cbsl->lock);

    return registed;
}
