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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bt_list.h"
#include "bt_status.h"
#include "connection_manager.h"
#include "index_allocator.h"
#include "manager_service.h"
#include "power_manager.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#define BT_INST_HOST_NAME_LEN 64
typedef struct bt_instance {
    struct list_node node;
    pid_t pid;
    uid_t uid;
    uint32_t app_id;
    uint64_t handle;
    uint8_t ins_type;
    uint8_t host_name[BT_INST_HOST_NAME_LEN + 1];
    uint32_t remote;
    void* usr_data;
} bt_instance_impl_t;

static struct list_node g_instances = LIST_INITIAL_VALUE(g_instances);
static index_allocator_t* g_instance_id = NULL;
static uv_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static bt_instance_impl_t* manager_find_instance(const char* name, pid_t pid)
{
    struct list_node* node;

    uv_mutex_lock(&g_mutex);

    list_for_every(&g_instances, node)
    {
        bt_instance_impl_t* ins = (bt_instance_impl_t*)node;
        size_t name_len = strlen(name);
        name_len = name_len > BT_INST_HOST_NAME_LEN ? BT_INST_HOST_NAME_LEN : name_len;
        if (strncmp((char*)ins->host_name, name, name_len) == 0 && ins->pid == pid) {
            uv_mutex_unlock(&g_mutex);
            return ins;
        }
    }

    uv_mutex_unlock(&g_mutex);

    return NULL;
}

static bt_instance_impl_t* manager_find_instance_by_appid(uint32_t app_id)
{
    struct list_node* node;

    uv_mutex_lock(&g_mutex);

    list_for_every(&g_instances, node)
    {
        bt_instance_impl_t* ins = (bt_instance_impl_t*)node;
        if (ins->app_id == app_id) {
            uv_mutex_unlock(&g_mutex);
            return ins;
        }
    }

    uv_mutex_unlock(&g_mutex);

    return NULL;
}

static void instance_release(bt_instance_impl_t* ins)
{
    list_delete(&ins->node);
    index_free(g_instance_id, ins->app_id);
    free(ins);
}

bt_status_t manager_create_instance(uint64_t handle, uint32_t type,
    const char* name, pid_t pid, uid_t uid,
    uint32_t* app_id)
{
    bt_instance_impl_t* ins = manager_find_instance(name, pid);
    if (ins)
        return BT_STATUS_FAIL;

    uv_mutex_lock(&g_mutex);

    if (g_instance_id == NULL)
        g_instance_id = index_allocator_create(10);

    ins = malloc(sizeof(bt_instance_impl_t));
    if (!ins) {
        uv_mutex_unlock(&g_mutex);
        return BT_STATUS_NOMEM;
    }

    ins->pid = pid;
    ins->uid = uid;
    int idx = index_alloc(g_instance_id);
    if (idx < 0) {
        free(ins);
        uv_mutex_unlock(&g_mutex);
        return BT_STATUS_NO_RESOURCES;
    }
    *app_id = idx;
    ins->app_id = idx;
    ins->handle = handle;
    ins->ins_type = type;
    snprintf((char*)ins->host_name, BT_INST_HOST_NAME_LEN, "%s", name);

    list_add_tail(&g_instances, &ins->node);

    uv_mutex_unlock(&g_mutex);

    return BT_STATUS_SUCCESS;
}

bt_status_t manager_get_instance(const char* name, pid_t pid, uint64_t* handle)
{
    bt_instance_impl_t* ins = manager_find_instance(name, pid);
    if (ins == NULL) {
        *handle = 0;
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    *handle = ins->handle;

    return BT_STATUS_SUCCESS;
}

bt_status_t manager_delete_instance(uint32_t app_id)
{
    bt_instance_impl_t* ins = manager_find_instance_by_appid(app_id);
    if (!ins)
        return BT_STATUS_NOT_FOUND;

    uv_mutex_lock(&g_mutex);

    list_delete(&ins->node);
    index_free(g_instance_id, ins->app_id);
    free(ins);

    uv_mutex_unlock(&g_mutex);

    return BT_STATUS_SUCCESS;
}

#if defined(CONFIG_BLUETOOTH_SERVICE) && defined(__NuttX__)
bt_status_t manager_start_service(uint32_t app_id, enum profile_id profile)
{
    bt_instance_impl_t* ins = manager_find_instance_by_appid(app_id);
    if (!ins)
        return BT_STATUS_NOT_FOUND;

    return service_manager_control(profile, CONTROL_CMD_START);
}

bt_status_t manager_stop_service(uint32_t app_id, enum profile_id profile)
{
    bt_instance_impl_t* ins = manager_find_instance_by_appid(app_id);
    if (!ins)
        return BT_STATUS_NOT_FOUND;

    return service_manager_control(profile, CONTROL_CMD_STOP);
}
#endif

void bluetooth_permission_check(uint32_t app_id)
{
}

void manager_init(void)
{
    if (g_instance_id == NULL)
        g_instance_id = index_allocator_create(10);
#if defined(CONFIG_BLUETOOTH_SERVICE) && defined(__NuttX__)
    service_manager_init();
    bt_pm_init();
    bt_cm_init();
#endif
}

void manager_cleanup(void)
{
    struct list_node* node;
    struct list_node* tmp;

    uv_mutex_lock(&g_mutex);

    list_for_every_safe(&g_instances, node, tmp)
    {
        list_delete(node);
        instance_release((bt_instance_impl_t*)node);
    }

    index_allocator_delete(&g_instance_id);

    uv_mutex_unlock(&g_mutex);

#if defined(CONFIG_BLUETOOTH_SERVICE) && defined(__NuttX__)
    service_manager_cleanup();
    bt_pm_cleanup();
    bt_cm_cleanup();
#endif
    uv_mutex_destroy(&g_mutex);
}