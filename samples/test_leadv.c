/****************************************************************************
 * frameworks/bluetooth/samples/test_leadv.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "btsample_adv"

#include "btm_le_advertise.h"
#include "btm_manager.h"
#include "bts_spp.h"
#include "log.h"
#include <pthread.h>
#include <stdio.h>
#include "log.h"

btm_leadv_hdl_t* adv_handle;

static void le_adv_started_callback(void* handle)
{
    BT_LOGD("%s", __func__);
    adv_handle = handle;
}

static void le_adv_stopped_callback(void* handle)
{
    BT_LOGD(" %s", __func__);
}

static void le_adv_failed_callback(void* handle, int error)
{
    BT_LOGD(" %s:err:%d", __func__, error);
}

void manager_init_status_changed_callback(bt_result_code status)
{
}

void manager_state_changed_callback(bt_manager_bt_state state)
{
}

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
    .init_status_changed_callback_cb = manager_init_status_changed_callback,
};

int main(int argc, FAR char* argv[])
{
    void* p;
    btm_interface_t* manager = get_bt_manager_interface();

    manager->init(&p, &mgt_cb);
    pthread_t message_tid;
    pthread_attr_t message_attr;
    pthread_attr_init(&message_attr);
    pthread_attr_setstacksize(&message_attr, 40960);
    pthread_create(&message_tid, &message_attr, manager->enable, p);
    //manager->enable();

    //spp_service_start();

    btm_le_advertise_callbacks cb = {
        .le_advertise_started_cb = le_adv_started_callback,
        .le_advertise_stopped_cb = le_adv_stopped_callback,
        .le_advertise_failed_cb = le_adv_failed_callback,
    };
#ifdef CONFIG_BLUETOOTH_SPP_TEST
    btm_spp_test();
#endif
    const uint8_t s_adv_data[]
        = { 0x02, 0x01, 0x08, 0x08, 0x09, 0x42, 0x52, 0x54, 0x20, 0x59, 0x61, 0x6F, 0x03, 0x02, 0x00, 0xFF };
    advertise_param_t adv_para;
    memset(&adv_para, 0, sizeof(SERVICE_SCAN_ADV_PARAMS_S));
    adv_para.params.adv_type = BLE_ADV_IND;
    adv_para.params.channel_map = ADV_CHANNEL_DEFAULT;
    adv_para.params.interval = 48;
    adv_para.params.tx_power = -10;
    adv_para.adv_length = sizeof(s_adv_data);
    adv_para.adv_data = (char*)s_adv_data;
    adv_para.scan_rsp_data = (char*)s_adv_data;
    adv_para.scan_rsp_length = sizeof(s_adv_data);

    btm_le_advertise_interface_t* adv_interface = get_btm_leadv_interface(manager);
    bt_result_code ret = adv_interface->start_advertising(&adv_handle, &adv_para, &cb);

    while (1) {
        usleep(1000);
    }
    getchar();
    BT_LOGD("sample adv stop ...");
    //adv_interface->stop_advertising(adv_handle);

    getchar();
    BT_LOGD("sample adv exit ...");
    while (1)
        sleep(10000);
    return 0;
}