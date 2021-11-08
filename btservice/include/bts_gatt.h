

#ifndef _SRV_INC_GATT_SERVICE_H
#define _SRV_INC_GATT_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "bts_gatt_client.h"
#include "bts_gatts.h"
#include "bts_leadv.h"
#include "bts_lescan.h"

typedef struct {
    size_t size;

    const bts_ble_scanner_callbacks* scanner;
    const ble_advertiser_callbacks advertiser;
    const bts_gatt_client_callbacks* client;
    const ble_gatt_server_callbacks* server;
} btgatt_callbacks;

typedef struct {
    size_t size;

    bt_result_code (*init)();
    void (*cleanup)(void);

    const gatt_scan_interface_t* scanner;
    const gatt_advertise_interface_t* advertiser;
    const bts_gattc_interface_t* client;
    const gatt_server_interface_t* server;
} gatt_interface_t;

const gatt_interface_t* gatt_get_interface(void);
#endif