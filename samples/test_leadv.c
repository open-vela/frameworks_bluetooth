
#include <stdio.h>
#include <stdlib.h>

#include "btm_le_advertise.h"
#include "btm_manager.h"
#include "btm_spp.h"

#define LOG_TAG "btsample_adv"
#include "log.h"

gatt_advertiser_t* adv_handle;

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

int main(int argc, FAR char* argv[])
{
    btm_interface_t* manager = get_bt_manager_interface();
    manager->init(NULL);
    manager->enable();

    gatt_advertise_callbacks cb = {
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

    btm_gatt_advertise_interface_t* adv_interface = get_le_advertise_interface(manager);
    bt_result_code ret = adv_interface->start_advertising(&adv_handle, &adv_para, &cb);

    getchar();
    BT_LOGD("sample adv stop ...");
    adv_interface->stop_advertising(adv_handle);

    getchar();
    BT_LOGD("sample adv exit ...");
    while (1)
        sleep(10000);
    return 0;
}