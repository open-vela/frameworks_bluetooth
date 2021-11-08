
#include <stddef.h>

#include "bts_gatt.h"
#include "log.h"

static bt_result_code gatt_init();
static void gatt_cleanup();

static gatt_interface_t gatt_if = {
    .size = sizeof(gatt_if),

    .init = gatt_init,
    .cleanup = gatt_cleanup,

    .client = NULL,
    .server = NULL,
    .scanner = NULL,
    .advertiser = NULL,
};

static bt_result_code gatt_init()
{
    if (gatt_if.server) {
        bt_result_code ret = gatt_if.server->init();
        if (ret != BT_RESULT_SUCCESS) {
            BT_LOGE("fai, gatt server init, err:%d", ret);
            return ret;
        }
    }

    return BT_RESULT_SUCCESS;
}

static void gatt_cleanup()
{
    if (gatt_if.server) {
        gatt_if.server->clean_up();
    }
}

const gatt_interface_t* gatt_get_interface(void)
{
    gatt_if.client = get_bts_gattc_instance();
    gatt_if.server = get_gatt_server_instance();
    gatt_if.scanner = get_bts_lescan_instance();
    gatt_if.advertiser = get_ble_advertise_instance();
    return &gatt_if;
}