/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#ifndef _CS_RAS_UTIL_H_
#define _CS_RAS_UTIL_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef MIN
#define MIN(a, b) (a) < (b) ? (a) : (b)
#endif /* MIN */

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *)((uintptr_t)(ptr) - offsetof(type, member)))
#endif /* container_of */

/** @cond INTERNAL_HIDDEN */
struct _cs_node {
    struct _cs_node* next;
};
/** @endcond */

/** Single-linked list node structure. */
typedef struct _cs_node cs_node_t;

/** @cond INTERNAL_HIDDEN */
struct _cs_list {
    cs_node_t* head;
    cs_node_t* tail;
};
/** @endcond */

/** Single-linked list structure. */
typedef struct _cs_list cs_list_t;

/**
 * @brief Initialize a list
 *
 * @param list A pointer on the list to initialize
 */
static inline void cs_list_init(cs_list_t* list)
{
    list->head = NULL;
    list->tail = NULL;
}

static inline cs_node_t* cs_node_next_peek(cs_node_t* node)
{
    return node->next;
}

static inline void cs_node_next_set(cs_node_t* parent, cs_node_t* child)
{
    parent->next = child;
}

static inline void cs_list_head_set(cs_list_t* list, cs_node_t* node)
{
    list->head = node;
}

static inline void cs_list_tail_set(cs_list_t* list, cs_node_t* node)
{
    list->tail = node;
}

/**
 * @brief Peek the first node from the list
 *
 * @param list A point to the list to peek the first node from
 *
 * @return A pointer on the first node of the list (or NULL if none)
 */
static inline cs_node_t* cs_list_peek_head(cs_list_t* list)
{
    return list->head;
}

/**
 * @brief Peek the last node from the list
 *
 * @param list A point to the list to peek the last node from
 *
 * @return A pointer on the last node of the list (or NULL if none)
 */
static inline cs_node_t* cs_list_peek_tail(cs_list_t* list)
{
    return list->tail;
}

static inline cs_node_t* cs_list_peek_next_no_check(cs_node_t* node)
{
    return cs_node_next_peek(node);
}

static inline cs_node_t* cs_list_peek_next(cs_node_t* node)
{
    return (node != NULL) ? cs_list_peek_next_no_check(node) : NULL;
}

/**
 * @brief Remove a node
 *
 * This and other sys_list_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param prev_node A pointer on the previous node
 *        (can be NULL, which means the node is the list's head)
 * @param node A pointer on the node to remove
 */
static inline void cs_list_remove(cs_list_t* list,
    cs_node_t* prev_node,
    cs_node_t* node)
{
    if (prev_node == NULL) {
        cs_list_head_set(list, cs_node_next_peek(node));
        if (cs_list_peek_tail(list) == node) {
            cs_list_tail_set(list, cs_list_peek_head(list));
        }
    } else {
        cs_node_next_set(prev_node, cs_node_next_peek(node));
        if (cs_list_peek_tail(list) == node) {
            cs_list_tail_set(list, prev_node);
        }
    }

    cs_node_next_set(node, NULL);
}

static inline void cs_list_append(cs_list_t* list, cs_node_t* node)
{
    cs_node_next_set(node, NULL);

    if (cs_list_peek_tail(list) == NULL) {
        cs_list_tail_set(list, node);
        cs_list_head_set(list, node);
    } else {
        cs_node_next_set(cs_list_peek_tail(list), node);
        cs_list_tail_set(list, node);
    }
}

#define CS_GENLIST_FOR_EACH_NODE(__lname, __l, __sn)             \
    for ((__sn) = cs_##__lname##_peek_head(__l); (__sn) != NULL; \
         (__sn) = cs_##__lname##_peek_next(__sn))

#define CS_GENLIST_ITERATE_FROM_NODE(__lname, __l, __sn)           \
    for ((__sn) = (__sn) ? cs_##__lname##_peek_next_no_check(__sn) \
                         : cs_##__lname##_peek_head(__l);          \
         (__sn) != NULL;                                           \
         (__sn) = cs_##__lname##_peek_next(__sn))

#define CS_GENLIST_FOR_EACH_NODE_SAFE(__lname, __l, __sn, __sns) \
    for ((__sn) = cs_##__lname##_peek_head(__l),                 \
        (__sns) = cs_##__lname##_peek_next(__sn);                \
         (__sn) != NULL; (__sn) = (__sns),                       \
        (__sns) = cs_##__lname##_peek_next(__sn))

#define CS_GENLIST_CONTAINER(__ln, __cn, __n) \
    ((__ln) ? container_of((__ln), __typeof__(*(__cn)), __n) : NULL)

#define CS_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, __cn, __n) \
    CS_GENLIST_CONTAINER(cs_##__lname##_peek_head(__l), __cn, __n)

#define CS_GENLIST_PEEK_TAIL_CONTAINER(__lname, __l, __cn, __n) \
    CS_GENLIST_CONTAINER(cs_##__lname##_peek_tail(__l), __cn, __n)

#define CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n) \
    ((__cn) ? CS_GENLIST_CONTAINER(                        \
         cs_##__lname##_peek_next(&((__cn)->__n)),         \
         __cn, __n)                                        \
            : NULL)

#define CS_GENLIST_FOR_EACH_CONTAINER(__lname, __l, __cn, __n)       \
    for ((__cn) = CS_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, __cn, \
             __n);                                                   \
         (__cn) != NULL;                                             \
         (__cn) = CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n))

#define CS_GENLIST_FOR_EACH_CONTAINER_SAFE(__lname, __l, __cn, __cns, __n) \
    for ((__cn) = CS_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, __cn, __n), \
        (__cns) = CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n);      \
         (__cn) != NULL; (__cn) = (__cns),                                 \
        (__cns) = CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n))

#define CS_LIST_FOR_EACH_CONTAINER_SAFE(__sl, __cn, __cns, __n) \
    CS_GENLIST_FOR_EACH_CONTAINER_SAFE(list, __sl, __cn, __cns, __n)

static inline void ras_state_set_bit(uint32_t* state, uint8_t bit)
{
    *state |= (1U << bit);
}

static inline void ras_state_clear_bit(uint32_t* state, uint8_t bit)
{
    *state &= ~(1U << bit);
}

static inline bool ras_state_get_bit(uint32_t* state, uint8_t bit)
{
    return ((*state >> bit) & 1U);
}

static inline uint16_t ras_get_uint16_from_ptr(const uint8_t src[2])
{
    return ((uint16_t)src[1] << 8) | src[0];
}

static inline void ras_put_uint16_to_ptr(uint16_t val, uint8_t dst[2])
{
    dst[0] = val;
    dst[1] = val >> 8;
}

#endif /* _CS_RAS_UTIL_H_ */