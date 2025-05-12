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
#include <stdio.h>
#include <stdlib.h>

#include "discovery.h"

static bt_instance_t* g_bt_ins = NULL;
static void* adapter_callback = NULL;
static app_demo_t app_demo;

/**
 * @brief Block the current thread and wait to be woken up.
 */
static void wait_awakened(void)
{
    sem_wait(&app_demo.sem);
}

/**
 * @brief Wake up the main thread of the app.
 */
static void wakeup_thread(void)
{
    sem_post(&app_demo.sem);
}

/**
 * @brief Add a node to the message queue.
 */
static void app_list_add_tail(struct list_node* node)
{
    pthread_mutex_lock(&app_demo.mutex);
    list_add_tail(&app_demo.message_queue, node);
    pthread_mutex_unlock(&app_demo.mutex);
    wakeup_thread();
}

/**
 * @brief Remove the head node of the message queue.
 */
static node_t* app_list_remove_head(void)
{
    node_t* node_data;

    wait_awakened();

    pthread_mutex_lock(&app_demo.mutex);
    struct list_node* node = list_remove_head(&app_demo.message_queue);
    pthread_mutex_unlock(&app_demo.mutex);
    if (node == NULL) {
        return NULL;
    }

    node_data = list_entry(node, node_t, node);
    return node_data;
}

/**
 * @brief Discover nearby Bluetooth devices.
 */
static void app_bt_discovery(void)
{
    node_t* node = (node_t*)malloc(sizeof(node_t));
    if (node == NULL) {
        LOGE("malloc failed.");
        return;
    }

    node->data.msg_type = APP_BT_GAP_START_DISCOVERY;
    node->data.gap_req._bt_adapter_start_discovery.timeout = 2;
    app_list_add_tail(&node->node);
}

/**
 * @brief  Adapter state change callback.
 *
 * This function is executed in the bt_client thread, the app needs to handle the callback
 * in another thread.
 */
static void gap_adapter_state_changed_callback(void* cookie, bt_adapter_state_t state)
{
    if (state != BT_ADAPTER_STATE_ON && state != BT_ADAPTER_STATE_OFF)
        return;

    node_t* node = (node_t*)malloc(sizeof(node_t));
    if (node == NULL) {
        LOGE("malloc failed.");
        return;
    }

    node->data.msg_type = APP_BT_GAP_ON_GAP_STATE_CHANGED;
    node->data.gap_cb._on_adapter_state_changed.state = state;
    app_list_add_tail(&node->node);

    if (state == BT_ADAPTER_STATE_ON)
        app_bt_discovery();
    else if (state == BT_ADAPTER_STATE_OFF)
        app_demo.running = 0;
}

/**
 * @brief  Discovery result callback.
 *
 * This function is executed in the bt_client thread, the app needs to handle the callback
 * in another thread.
 */
static void gap_discovery_result_callback(void* cookie, bt_discovery_result_t* remote)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(&remote->addr, addr_str);

    LOGI("Discovery result: device [%s], name: %s, code: %08x, is HEADSET: %s, rssi: %d\n",
        addr_str, remote->name, remote->cod,
        IS_HEADSET(remote->cod) ? "true" : "false",
        remote->rssi);
}

/**
 * @brief  Discovery state change callback.
 *
 * This function is executed in the bt_client thread, the app needs to handle the callback
 * in another thread.
 */
static void gap_discovery_state_changed_callback(void* cookie, bt_discovery_state_t state)
{
    node_t* node = (node_t*)malloc(sizeof(node_t));
    if (node == NULL) {
        LOGE("malloc failed.");
        return;
    }

    node->data.msg_type = APP_BT_GAP_ON_DISCOVERY_STATE_CHANGED;
    node->data.gap_cb._on_discovery_state_changed.state = state;

    app_list_add_tail(&node->node);
}

// gap callback
const static adapter_callbacks_t app_gap_cbs = {
    .on_adapter_state_changed = gap_adapter_state_changed_callback,
    .on_discovery_result = gap_discovery_result_callback,
    .on_discovery_state_changed = gap_discovery_state_changed_callback,
};

/**
 * @brief  Initialize semaphore, mutex, message queue.
 *
 * @note   Semaphores are used to control the number of concurrently executing threads.
 *         A semaphore has a counter, and threads need to acquire the semaphore before
 *         accessing a resource. When the semaphore counter is greater than 0, the thread
 *         can continue executing. When the semaphore counter is equal to 0, the thread
 *         needs to wait for other threads to release resources so that the semaphore
 *         counter can increase before it can continue executing.
 *
 * @note   Mutex locks are used to protect shared resources, ensuring that only one thread
 *         can access the shared resource at a time, while other threads must wait until
 *         the lock is released by that thread before they can access it.
 *
 * @note   Message queues is used to store events to be processed. When calling the Bluetooth
 *         synchronization interface, receiving and sending Bluetooth messages from the Bluetooth
 *         module should be done in different threads.
 */
static void app_demo_init(void)
{
    app_demo.running = 1;
    sem_init(&app_demo.sem, 0, 1);
    pthread_mutex_init(&app_demo.mutex, NULL);
    list_initialize(&app_demo.message_queue);
}

/**
 * @brief Destroy semaphore, mutex, clear up message queue.
 */
static void app_demo_deinit(void)
{
    sem_destroy(&app_demo.sem);
    pthread_mutex_destroy(&app_demo.mutex);

    node_t* entry = NULL;
    node_t* temp_entry = NULL;
    list_for_every_entry_safe(&app_demo.message_queue, entry, temp_entry, node_t, node)
    {
        list_delete(&entry->node);
        free(entry);
    }
}

/**
 * @brief The main thread processes events.
 */
static void app_handle_message(node_t* node)
{
    if (node == NULL) {
        return;
    }

    if (node->data.msg_type > APP_BT_GAP_MESSAGE_START && node->data.msg_type < APP_BT_GAP_MESSAGE_END)
        demo_discovery_handle_gap_message(g_bt_ins, node);
}

/**
 * @brief Check the exit condition of the while loop in the main function.
 *
 * The condition for exiting the while loop can be multiple, but in this demo,
 * only one scenario is provided: Bluetooth is turned off.
 */
static bool app_if_running(void)
{
    // Developers can add additional exit condition checks.

    return app_demo.running;
}

int main(int argc, char* argv[])
{
    node_t* node = NULL;

    // 1. Initialize semaphore;
    // 2. Initialize mutex;
    // 3. Initialize message queue.
    app_demo_init();

    // Create bluetooth client instance.
    g_bt_ins = bluetooth_create_instance();
    if (g_bt_ins == NULL) {
        LOGE("create instance error");
        goto error;
    }

    // Register gap callback.
    adapter_callback = bt_adapter_register_callback(g_bt_ins, &app_gap_cbs);
    if (adapter_callback == NULL) {
        LOGE("register callback error.");
        goto error;
    }

    // Enable bluetooth.
    if (bt_adapter_enable(g_bt_ins) != BT_STATUS_SUCCESS) {
        LOGE("enable adapter error.");
        goto error;
    }

    // The app main thread，is used to handle bluetooth events.
    while (app_if_running()) {
        // Obtain the msg to be processed.
        node = app_list_remove_head();

        // The main thread processes events.
        app_handle_message(node);
    }

error:
    // Unregister gap callback;
    if (adapter_callback) {
        bt_adapter_unregister_callback(g_bt_ins, adapter_callback);
        adapter_callback = NULL;
    }

    if (g_bt_ins) {
        // Delete bluetooth client instance;
        bluetooth_delete_instance(g_bt_ins);
        g_bt_ins = NULL;
    }

    // 1. Destroy semaphore;
    // 2. Destroy mutex;
    // 3. clean up message queue.
    app_demo_deinit();

    LOGI("Bluetooth closed.");

    return 0;
}