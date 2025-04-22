#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>

#include "hidd_code.h"

bt_instance_t* g_bt_ins = NULL;
static void* adapter_callback = NULL;
app_demo_t app_demo;

/**
 * @brief Block the current thread and wait to be woken up.
 */
static void block_thread(void)
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
void bt_msg_list_add_tail(struct list_node* node)
{
    pthread_mutex_lock(&app_demo.mutex);
    list_add_tail(&app_demo.list, node);
    pthread_mutex_unlock(&app_demo.mutex);
    wakeup_thread();
}

/**
 * @brief Remove the head node of the message queue.
 */
static node_t* bt_msg_list_rm_head(void)
{
    block_thread();
    node_t* node_data;
    pthread_mutex_lock(&app_demo.mutex);
    struct list_node* node = list_remove_head(&app_demo.list);
    pthread_mutex_unlock(&app_demo.mutex);
    if (node == NULL) {
        return NULL;
    }

    node_data = list_entry(node, node_t, node);
    return node_data;
}

/**
 * @brief  Adapter state change callback.
 * 
 * This function is executed in the bt_client thread, the app needs to handle the callback 
 * in another thread.
 */
static void gap_adapter_state_changed_callback(void* cookie, bt_adapter_state_t state)
{
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_GAP_ON_GAP_STATE_CHANGED;
    node_data->data.gap_cb._on_adapter_state_changed.state = state;

    bt_msg_list_add_tail(&node_data->node);

    if (state == BT_ADAPTER_STATE_OFF) {
        app_demo.quit = 1;
    }
}

/**
 * @brief  Connection request callback.
 * 
 * This function is executed in the bt_client thread, the app needs to handle the callback 
 * in another thread.
 */
static void gap_connection_request_callback(void* cookie, bt_address_t* addr)
{
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_GAP_ON_CONNECT_REQUEST;
    memcpy(&node_data->data.gap_cb._on_connect_request.addr, addr, sizeof(bt_address_t));

    bt_msg_list_add_tail(&node_data->node);
}

/**
 * @brief  Connection state change callback.
 * 
 * This function is executed in the bt_client thread, the app needs to handle the callback 
 * in another thread.
 */
static void gap_connection_state_changed_callback(void* cookie, bt_address_t* addr, bt_transport_t transport, connection_state_t state)
{

    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_GAP_ON_CONNECTION_STATE_CHANGED;
    node_data->data.gap_cb._on_connection_state_changed.state = state;
    node_data->data.gap_cb._on_connection_state_changed.transport = transport;
    memcpy(&node_data->data.gap_cb._on_connection_state_changed.addr, addr, sizeof(bt_address_t));

    bt_msg_list_add_tail(&node_data->node);
}

/**
 * @brief  Bond state change callback.
 * 
 * This function is executed in the bt_client thread, the app needs to handle the callback 
 * in another thread.
 */
static void gap_bond_state_changed_callback(void* cookie, bt_address_t* addr, bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_GAP_ON_BOND_STATE_CHANGED;
    node_data->data.gap_cb._on_bond_state_changed.state = state;
    node_data->data.gap_cb._on_bond_state_changed.transport = transport;
    node_data->data.gap_cb._on_bond_state_changed.is_ctkd = is_ctkd;
    memcpy(&node_data->data.gap_cb._on_bond_state_changed.addr, addr, sizeof(bt_address_t));

    bt_msg_list_add_tail(&node_data->node);
}

//gap callback
const static adapter_callbacks_t app_gap_cbs = {
    .on_adapter_state_changed = gap_adapter_state_changed_callback,
    .on_connection_state_changed = gap_connection_state_changed_callback,
    .on_connect_request = gap_connection_request_callback,
    .on_bond_state_changed = gap_bond_state_changed_callback,
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
    app_demo.quit = 0;
    sem_init(&app_demo.sem, 0, 1);
    pthread_mutex_init(&app_demo.mutex, NULL);
    list_initialize(&app_demo.list);
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
    list_for_every_entry_safe(&app_demo.list, entry, temp_entry, node_t, node)
    {
        list_delete(&entry->node);
        free(entry);
    }
}

/**
 * @brief App calls the API of the Bluetooth module.
 */
static void app_handle_bt_message(node_t* node_data)
{
    if (node_data == NULL) {
        return;
    }

    if (node_data->data.msg_type > APP_BT_GAP_MESSAGE_START && node_data->data.msg_type < APP_BT_GAP_MESSAGE_END)
        app_bt_gap_handle_message(g_bt_ins, node_data);
    if (node_data->data.msg_type > APP_BT_HID_DEVICE_MESSAGE_START && node_data->data.msg_type < APP_BT_HID_DEVICE_MESSAGE_END)
        app_bt_hidd_handle_message(g_bt_ins, node_data);
}

/**
 * @brief The app processes the callback reported by the Bluetooth module.
 */
static void app_handle_bt_callback(node_t* node_data)
{
    if (node_data == NULL) {
        return;
    }

    if (node_data->data.msg_type > APP_BT_GAP_CALLBACK_START && node_data->data.msg_type < APP_BT_GAP_CALLBACK_END)
        app_bt_gap_handle_callback(g_bt_ins, node_data);
    if (node_data->data.msg_type > APP_BT_HID_DEVICE_CALLBACK_START && node_data->data.msg_type < APP_BT_HID_DEVICE_CALLBACK_END)
        app_bt_hidd_handle_callback(g_bt_ins, node_data);
}

/**
 * @brief The main thread processes events.
 */
static void app_handle_message(node_t* node_data)
{
    if (node_data == NULL) {
        return;
    }

    if (node_data->data.msg_type > APP_BT_MESSAGE_START && node_data->data.msg_type < APP_BT_MESSAGE_END)
        app_handle_bt_message(node_data);
    else if (node_data->data.msg_type > APP_BT_CALLBACK_START && node_data->data.msg_type < APP_BT_CALLBACK_END)
        app_handle_bt_callback(node_data);
}

/**
 * @brief Check the exit condition of the while loop in the main function.
 * 
 * The condition for exiting the while loop can be multiple, but in this demo, 
 * only one scenario is provided: Bluetooth is turned off.
 */
static bool app_check_if_quit(void)
{
    //Developers can add additional exit condition checks.

    return app_demo.quit;
}

int main(int argc, char *argv[])
{
    node_t* node_data = NULL;

    // 1. Initialize semaphore;
    // 2. Initialize mutex;
    // 3. Initialize message queue.
    app_demo_init();

    // Create bluetooth client instance.
    g_bt_ins = bluetooth_create_instance();
    if (g_bt_ins == NULL) {
        syslog(LOG_INFO, "[app_demo] create instance error\n");
        goto instance_error;
    }

    // Register gap callback.
    adapter_callback = bt_adapter_register_callback(g_bt_ins, &app_gap_cbs);
    if (adapter_callback == NULL) {
        syslog(LOG_INFO, "[app_demo] register callback error\n");
        goto error;
    }

    // Enable bluetooth.
    if (bt_adapter_enable(g_bt_ins) != BT_STATUS_SUCCESS) {
        syslog(LOG_INFO, "[app_demo] enable adapter error\n");
        goto error;
    }

    // The app main thread，is used to handle bluetooth events.
    while (app_check_if_quit() == 0) {
        // Obtain the msg to be processed.
        node_data = bt_msg_list_rm_head();

        // The main thread processes events.
        app_handle_message(node_data);
    }

error:
    // Unregister gap callback;
    bt_adapter_unregister_callback(g_bt_ins, adapter_callback);
    adapter_callback = NULL;
    
instance_error:
    // 1. Destroy semaphore;
    // 2. Destroy mutex;
    // 3. clean up message queue.
    app_demo_deinit();

    // Delete bluetooth client instance;
    bluetooth_delete_instance(g_bt_ins);
    g_bt_ins = NULL;
    syslog(LOG_INFO, "[app_demo] Bluetooth closed.\n");

    return 0;
}