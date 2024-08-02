/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "bt_list.h"

typedef struct _bt_list {
    struct list_node list;
    size_t length;
    bt_list_free_cb_t free_cb;
} bt_list_t;

typedef struct _bt_list_node {
    struct list_node node;
    void* data;
} bt_list_node_t;

bt_list_t* bt_list_new(bt_list_free_cb_t cb)
{
    bt_list_t* list = malloc(sizeof(bt_list_t));
    if (!list)
        return NULL;

    list->length = 0;
    list->free_cb = cb;
    list_initialize(&list->list);

    return list;
}

void bt_list_free(bt_list_t* list)
{
    if (!list)
        return;

    bt_list_clear(list);
    list_delete(&list->list);
    free(list);
}

void bt_list_clear(bt_list_t* list)
{
    assert(list);
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&list->list, node, tmp)
    {
        bt_list_node_t* bt_node = (bt_list_node_t*)node;
        list_delete(&bt_node->node);
        if (list->free_cb)
            list->free_cb(bt_node->data);
        free(bt_node);
        list->length--;
    }
}

bool bt_list_is_empty(bt_list_t* list)
{
    assert(list);

    return list->length == 0;
}

size_t bt_list_length(bt_list_t* list)
{
    assert(list);

    return list->length;
}

bt_list_node_t* bt_list_head(bt_list_t* list)
{
    assert(list);

    return (bt_list_node_t*)list_peek_head(&list->list);
}

bt_list_node_t* bt_list_tail(bt_list_t* list)
{
    assert(list);

    return (bt_list_node_t*)list_peek_tail(&list->list);
}

bt_list_node_t* bt_list_next(bt_list_t* list, bt_list_node_t* bt_node)
{
    assert(list);
    if (!bt_node)
        return NULL;

    return (bt_list_node_t*)list_next(&list->list, &bt_node->node);
}

void* bt_list_node(bt_list_node_t* bt_node)
{
    assert(bt_node);
    if (!bt_node)
        return NULL;

    return bt_node->data;
}

void bt_list_add_head(bt_list_t* list, void* data)
{
    assert(list);
    bt_list_node_t* node = malloc(sizeof(bt_list_node_t));
    assert(node);

    node->data = data;
    list_add_head(&list->list, &node->node);
    list->length++;
}

void bt_list_add_tail(bt_list_t* list, void* data)
{
    assert(list);
    bt_list_node_t* node = malloc(sizeof(bt_list_node_t));
    assert(node);

    node->data = data;
    list_add_tail(&list->list, &node->node);
    list->length++;
}

void bt_list_remove_node(bt_list_t* list, bt_list_node_t* node)
{
    list_delete(&node->node);
    list->length--;
    if (list->free_cb)
        list->free_cb(node->data);
    free(node);
}

void bt_list_remove(bt_list_t* list, void* data)
{
    assert(list);
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&list->list, node, tmp)
    {
        bt_list_node_t* bt_node = (bt_list_node_t*)node;
        if (bt_node->data == data) {
            list_delete(&bt_node->node);
            if (list->free_cb)
                list->free_cb(bt_node->data);
            free(bt_node);
            list->length--;
            break;
        }
    }
}

void bt_list_move(bt_list_t* src, bt_list_t* dst, void* data, bool move_to_head)
{
    assert(src);
    assert(dst);
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&src->list, node, tmp)
    {
        bt_list_node_t* bt_node = (bt_list_node_t*)node;
        if (bt_node->data != data)
            continue;

        list_delete(&bt_node->node);
        src->length--;

        if (move_to_head) {
            list_add_head(&dst->list, &bt_node->node);
        } else {
            list_add_tail(&dst->list, &bt_node->node);
        }
        dst->length++;

        break;
    }
}

void bt_list_foreach(bt_list_t* list, bt_list_iter_cb cb, void* context)
{
    assert(list);
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&list->list, node, tmp)
    {
        bt_list_node_t* bt_node = (bt_list_node_t*)node;
        cb(bt_node->data, context);
    }
}

void* bt_list_find(bt_list_t* list, bt_list_find_cb cb, void* context)
{
    assert(list);
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&list->list, node, tmp)
    {
        bt_list_node_t* bt_node = (bt_list_node_t*)node;
        if (cb(bt_node->data, context))
            return bt_node->data;
    }

    return NULL;
}
