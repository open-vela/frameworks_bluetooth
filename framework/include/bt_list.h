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
#ifndef __BT_LIST_H__
#define __BT_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "bt_list_internal.h"

typedef void (*bt_list_free_cb_t)(void* data);
typedef struct _bt_list bt_list_t;
typedef struct _bt_list_node bt_list_node_t;
typedef void (*bt_list_iter_cb)(void* data, void* context);
typedef bool (*bt_list_find_cb)(void* data, void* context);

bt_list_t* bt_list_new(bt_list_free_cb_t cb);
void bt_list_free(bt_list_t* list);
void bt_list_clear(bt_list_t* list);
bool bt_list_is_empty(bt_list_t* list);
size_t bt_list_length(bt_list_t* list);
bt_list_node_t* bt_list_head(bt_list_t* list);
bt_list_node_t* bt_list_tail(bt_list_t* list);
bt_list_node_t* bt_list_next(bt_list_t* list, bt_list_node_t* bt_node);
void* bt_list_node(bt_list_node_t* bt_node);
void bt_list_add_head(bt_list_t* list, void* data);
void bt_list_add_tail(bt_list_t* list, void* data);
void bt_list_remove_node(bt_list_t* list, bt_list_node_t* node);
void bt_list_remove(bt_list_t* list, void* data);
void bt_list_move(bt_list_t* src, bt_list_t* dst, void* data, bool move_to_head);
void bt_list_foreach(bt_list_t* list, bt_list_iter_cb cb, void* context);
void* bt_list_find(bt_list_t* list, bt_list_find_cb cb, void* context);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LIST_H__ */
