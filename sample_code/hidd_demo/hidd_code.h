#include <nuttx/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "app_bt_message_gap.h"
#include "app_bt_message_hidd.h"
#include "bluetooth.h"
#include "bt_adapter.h"

typedef enum {
#define __APP_BT_MESSAGE_CODE__
    APP_BT_MESSAGE_START,
#include "app_bt_message_gap.h"
#include "app_bt_message_hidd.h"
    APP_BT_MESSAGE_END,
#undef __APP_BT_MESSAGE_CODE__
#define __APP_BT_CALLBACK_CODE__
    APP_BT_CALLBACK_START,
#include "app_bt_message_gap.h"
#include "app_bt_message_hidd.h"
    APP_BT_CALLBACK_END,
#undef __APP_BT_MESSAGE_CODE__
} app_bt_message_type_t;

typedef struct {
    uint32_t msg_type;
    union {
        app_bt_message_gap_t gap_req;
        app_bt_message_hidd_t hidd_req;
    };
    union {
        app_bt_message_gap_callbacks_t gap_cb;
        app_bt_message_hidd_callbacks_t hidd_cb;
    };
} app_demo_message_t;

typedef struct {
    struct list_node node;
    app_demo_message_t data;
} node_t;

typedef struct {
    uint8_t quit;
    sem_t sem;
    pthread_mutex_t mutex;
    struct list_node list;
} app_demo_t;

void bt_msg_list_add_tail(struct list_node* node);
void app_bt_gap_handle_message(bt_instance_t* g_bt_ins, node_t* node);
void app_bt_gap_handle_callback(bt_instance_t* g_bt_ins, node_t* node);

void bt_hid_device_init(bt_instance_t* g_bt_ins);
void app_bt_hidd_handle_message(bt_instance_t* g_bt_ins, node_t* node);
void app_bt_hidd_handle_callback(bt_instance_t* g_bt_ins, node_t* node);
