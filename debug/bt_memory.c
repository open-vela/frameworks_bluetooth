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
#include "bt_memory.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define GUARD_SIZE 16
#define GUARD_HEAD 0xAA
#define GUARD_TAIL 0xBB

typedef struct bt_mem_node {
    void* addr;
    size_t size;
    const char* file;
    int line;
    unsigned char guard[GUARD_SIZE];
    struct bt_mem_node* next;
} bt_mem_node_t;

typedef struct bt_mem_manager {
    bt_mem_node_t* mem_list;
    pthread_mutex_t list_lock;
    size_t current_mem;
    size_t peak_mem;
} bt_mem_manager_t;

static bt_mem_manager_t g_mem_manager = {
    .mem_list = NULL,
    .list_lock = PTHREAD_MUTEX_INITIALIZER,
    .current_mem = 0,
    .peak_mem = 0,
};

void* bt_malloc_hook(size_t size, const char* file, int line)
{
    bt_mem_manager_t* mem_manager = &g_mem_manager;
    void* raw;
    bt_mem_node_t* head;

    raw = malloc(size + sizeof(bt_mem_node_t) + GUARD_SIZE);
    if (!raw) {
        syslog(LOG_ALERT, "[bt_memory] malloc failed, size: %zu, file: %s, line: %d", size, file, line);
        return NULL;
    }

    head = (bt_mem_node_t*)raw;
    *head = (bt_mem_node_t) {
        .addr = (char*)raw + sizeof(bt_mem_node_t),
        .size = size,
        .file = file,
        .line = line,
        .next = NULL
    };

    memset(head->guard, GUARD_HEAD, sizeof(head->guard));
    memset((char*)head->addr + size, GUARD_TAIL, GUARD_SIZE);

    pthread_mutex_lock(&mem_manager->list_lock);
    mem_manager->current_mem += size;
    if (mem_manager->current_mem > mem_manager->peak_mem) {
        mem_manager->peak_mem = mem_manager->current_mem;
    }

    head->next = mem_manager->mem_list;
    mem_manager->mem_list = head;
    pthread_mutex_unlock(&mem_manager->list_lock);

    return head->addr;
}

void* bt_calloc_hook(size_t num, size_t size, const char* file, int line)
{
    size_t total;
    void* ptr;

    total = num * size;
    ptr = bt_malloc_hook(total, file, line);
    if (!ptr) {
        syslog(LOG_ALERT, "[bt_memory] calloc failed, num: %zu, size: %zu, file: %s, line: %d", num, size, file, line);
        return NULL;
    }

    memset(ptr, 0, total);
    return ptr;
}

void bt_free_hook(void* ptr)
{
    bt_mem_manager_t* mem_manager = &g_mem_manager;
    bt_mem_node_t** pp;
    bool found = false;

    if (!ptr) {
        return;
    }

    pthread_mutex_lock(&mem_manager->list_lock);

    pp = &mem_manager->mem_list;
    while (*pp) {
        if ((*pp)->addr == ptr) {
            bt_mem_node_t* node = *pp;

            for (int i = 0; i < GUARD_SIZE; i++) {
                if (((uint8_t)node->guard[i] != GUARD_HEAD) || ((uint8_t)((char*)ptr + node->size)[i] != GUARD_TAIL)) {
                    syslog(LOG_ALERT, "[bt_memory] Buffer overflow at %s:%d", node->file, node->line);
                    assert(0);
                }
            }

            mem_manager->current_mem -= node->size;
            *pp = node->next;
            found = true;
            free(node);
            break;
        }
        pp = &(*pp)->next;
    }

    pthread_mutex_unlock(&mem_manager->list_lock);

    if (!found) {
        syslog(LOG_ALERT, "[bt_memory] Freeing unallocated memory %p", ptr);
        assert(0);
    }
}

void bt_report_leak(void)
{
    bt_mem_manager_t* mem_manager = &g_mem_manager;
    bt_mem_node_t* node;

    pthread_mutex_lock(&mem_manager->list_lock);

    syslog(LOG_ALERT, "[bt_memory] ===== Memory Leak Report =====");
    syslog(LOG_ALERT, "[bt_memory] Peak memory: %zu bytes", mem_manager->peak_mem);

    node = mem_manager->mem_list;
    while (node) {
        syslog(LOG_ALERT, "[bt_memory] Leak %p (%zu bytes) at %s:%d",
            node->addr, node->size, node->file, node->line);
        node = node->next;
    }
    syslog(LOG_ALERT, "[bt_memory] ===== End of Report =====");

    pthread_mutex_unlock(&mem_manager->list_lock);
}
