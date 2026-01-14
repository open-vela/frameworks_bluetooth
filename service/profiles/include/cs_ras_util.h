/****************************************************************************
 *
 *   Copyright (C) 2025 Xiaomi InC. All rights reserved.
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

#ifndef _CS_RAS_UTIL_H_
#define _CS_RAS_UTIL_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef MIN
#define MIN(a, b)    (a) < (b) ? (a) : (b)
#endif /* MIN */

#define CS_CONTAINER_OF(ptr, type, member) ({                      \
    const typeof(((type *)0)->member) *__mptr = (ptr);          \
    (type *)((char *)__mptr - offsetof(type, member));          \
})

#define CS_UINT32_TO_BYTE_STREAM(p, u32) \
    {                                       \
        *(p)++ = (uint8_t)((u32) >> 24);    \
        *(p)++ = (uint8_t)((u32) >> 16);    \
        *(p)++ = (uint8_t)((u32) >> 8);     \
        *(p)++ = (uint8_t)(u32);            \
    }

#define CS_UINT16_TO_BYTE_STREAM(p, u16) \
    {                                       \
        *(p)++ = (uint8_t)((u16) >> 8);     \
        *(p)++ = (uint8_t)(u16);            \
    }

#define CS_UINT8_TO_BYTE_STREAM(p, u8) \
    {                                     \
        *(p)++ = (uint8_t)(u8);           \
    }

#define CS_INT8_TO_BYTE_STREAM(p, u8) \
    {                                    \
        *(p)++ = (int8_t)(u8);           \
    }

#define CS_ARRAY16_TO_BYTE_STREAM(p, a)   \
    {                                        \
        int ijk;                             \
        for (ijk = 0; ijk < 16; ijk++)       \
            *(p)++ = (uint8_t)(a)[15 - ijk]; \
    }

#define CS_ARRAY8_TO_BYTE_STREAM(p, a)   \
    {                                       \
        int ijk;                            \
        for (ijk = 0; ijk < 8; ijk++)       \
            *(p)++ = (uint8_t)(a)[7 - ijk]; \
    }

#define CS_ARRAY_TO_BYTE_STREAM(p, a, len) \
    {                                         \
        int ijk;                              \
        for (ijk = 0; ijk < (len); ijk++)     \
            *(p)++ = (uint8_t)(a)[ijk];       \
    }

#define CS_BYTE_STREAM_TO_INT8(u8, p) \
    {                                    \
        (u8) = (*((int8_t*)(p)));        \
        (p) += 1;                        \
    }

#define CS_BYTE_STREAM_TO_UINT8(u8, p) \
    {                                     \
        (u8) = (uint8_t)(*(p));           \
        (p) += 1;                         \
    }

#define CS_BYTE_STREAM_TO_UINT16(u16, p)                           \
    {                                                                 \
        (u16) = ((uint16_t)(*((p) + 1)) + (((uint16_t)(*(p))) << 8)); \
        (p) += 2;                                                     \
    }

#define CS_BYTE_STREAM_TO_UINT32(u32, p)                                       \
    {                                                                             \
        (u32) = (((uint32_t)(*((p) + 3))) + ((((uint32_t)(*((p) + 2)))) << 8)     \
            + ((((uint32_t)(*((p) + 1)))) << 16) + ((((uint32_t)(*(p))) << 24))); \
        (p) += 4;                                                                 \
    }

#define CS_BYTE_STREAM_TO_ARRAY16(a, p) \
    {                                      \
        int ijk;                           \
        uint8_t* _pa = (uint8_t*)(a) + 15; \
        for (ijk = 0; ijk < 16; ijk++)     \
            *_pa-- = *(p)++;               \
    }

#define CS_BYTE_STREAM_TO_ARRAY8(a, p) \
    {                                     \
        int ijk;                          \
        uint8_t* _pa = (uint8_t*)(a) + 7; \
        for (ijk = 0; ijk < 8; ijk++)     \
            *_pa-- = *(p)++;              \
    }

#define CS_BYTE_STREAM_TO_ARRAY(a, p, len) \
    {                                         \
        int ijk;                              \
        for (ijk = 0; ijk < (len); ijk++)     \
            ((uint8_t*)(a))[ijk] = *(p)++;    \
    }

/** @cond INTERNAL_HIDDEN */
struct _cs_node {
	struct _cs_node *next;
};
/** @endcond */

/** Single-linked list node structure. */
typedef struct _cs_node cs_node_t;

/** @cond INTERNAL_HIDDEN */
struct _cs_list {
	cs_node_t *head;
	cs_node_t *tail;
};
/** @endcond */

/** Single-linked list structure. */
typedef struct _cs_list cs_list_t;

/**
 * @brief Initialize a list
 *
 * @param list A pointer on the list to initialize
 */
static inline void cs_list_init(cs_list_t *list)
{
	list->head = NULL;
	list->tail = NULL;
}

static inline cs_node_t *cs_node_next_peek(cs_node_t *node)
{
	return node->next;
}

static inline void cs_node_next_set(cs_node_t *parent, cs_node_t *child)
{
	parent->next = child;
}

static inline void cs_list_head_set(cs_list_t *list, cs_node_t *node)
{
	list->head = node;
}

static inline void cs_list_tail_set(cs_list_t *list, cs_node_t *node)
{
	list->tail = node;
}

/**
 * @brief Peek the first node from the list
 *
 * @param list A point on the list to peek the first node from
 *
 * @return A pointer on the first node of the list (or NULL if none)
 */
static inline cs_node_t *cs_list_peek_head(cs_list_t *list)
{
	return list->head;
}

/**
 * @brief Peek the last node from the list
 *
 * @param list A point on the list to peek the last node from
 *
 * @return A pointer on the last node of the list (or NULL if none)
 */
static inline cs_node_t *cs_list_peek_tail(cs_list_t *list)
{
	return list->tail;
}

static inline cs_node_t *cs_list_peek_next_no_check(cs_node_t *node)
{
    return cs_node_next_peek(node);
}

static inline cs_node_t *cs_list_peek_next(cs_node_t *node)
{
    return (node != NULL) ? cs_list_peek_next_no_check(node) :  NULL;	
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
static inline void	cs_list_remove(cs_list_t *list,
				   cs_node_t *prev_node,
				   cs_node_t *node)
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

static inline void	cs_list_append(cs_list_t *list, cs_node_t *node)
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

#define CS_GENLIST_FOR_EACH_NODE(__lname, __l, __sn)			\
	for ((__sn) = cs_ ## __lname ## _peek_head(__l); (__sn) != NULL;	\
	     (__sn) = cs_ ## __lname ## _peek_next(__sn))

#define CS_GENLIST_ITERATE_FROM_NODE(__lname, __l, __sn)			\
	for ((__sn) = (__sn) ? cs_ ## __lname ## _peek_next_no_check(__sn)	\
			 : cs_ ## __lname ## _peek_head(__l);		\
	     (__sn) != NULL;						\
	     (__sn) = cs_ ## __lname ## _peek_next(__sn))

#define CS_GENLIST_FOR_EACH_NODE_SAFE(__lname, __l, __sn, __sns)		\
	for ((__sn) = cs_ ## __lname ## _peek_head(__l),			\
		     (__sns) = cs_ ## __lname ## _peek_next(__sn);	\
	     (__sn) != NULL ; (__sn) = (__sns),				\
		     (__sns) = cs_ ## __lname ## _peek_next(__sn))

#define CS_GENLIST_CONTAINER(__ln, __cn, __n)				\
	((__ln) ? CS_CONTAINER_OF((__ln), __typeof__(*(__cn)), __n) : NULL)

#define CS_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, __cn, __n)		\
	CS_GENLIST_CONTAINER(cs_ ## __lname ## _peek_head(__l), __cn, __n)

#define CS_GENLIST_PEEK_TAIL_CONTAINER(__lname, __l, __cn, __n)		\
	CS_GENLIST_CONTAINER(cs_ ## __lname ## _peek_tail(__l), __cn, __n)

#define CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n)		\
	((__cn) ? CS_GENLIST_CONTAINER(					\
			cs_ ## __lname ## _peek_next(&((__cn)->__n)),	\
			__cn, __n) : NULL)

#define CS_GENLIST_FOR_EACH_CONTAINER(__lname, __l, __cn, __n)		\
	for ((__cn) = CS_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, __cn,	\
						  __n);			\
	     (__cn) != NULL;						\
	     (__cn) = CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n))

#define CS_GENLIST_FOR_EACH_CONTAINER_SAFE(__lname, __l, __cn, __cns, __n)     \
	for ((__cn) = CS_GENLIST_PEEK_HEAD_CONTAINER(__lname, __l, __cn, __n),   \
	     (__cns) = CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n); \
	     (__cn) != NULL; (__cn) = (__cns),				\
	     (__cns) = CS_GENLIST_PEEK_NEXT_CONTAINER(__lname, __cn, __n))

#define CS_LIST_FOR_EACH_CONTAINER_SAFE(__sl, __cn, __cns, __n)	\
	CS_GENLIST_FOR_EACH_CONTAINER_SAFE(list, __sl, __cn, __cns, __n)

static inline void ras_state_set_bit(uint32_t *state, uint8_t bit)
{
    *state |= (1U << bit);
}

static inline void ras_state_clear_bit(uint32_t *state, uint8_t bit)
{
    *state &= ~(1U << bit);
}

static inline bool ras_state_get_bit(uint32_t *state, uint8_t bit)
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

const char *cs_log_to_hex_str(const void *buf, size_t len);

#endif /* _CS_RAS_GATTS_H_ */