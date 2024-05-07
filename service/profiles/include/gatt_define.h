/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef __GATT_DEFINE_H__
#define __GATT_DEFINE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_gatt_defs.h"
#include "bt_gattc.h"
#include "bt_gatts.h"
#include "bt_uuid.h"
#include <stdint.h>

/**
 * \def GATT_CBACK(P_CB, P_CBACK) Description
 */
#define GATT_CBACK(P_CB, P_CBACK, ...)                \
    do {                                              \
        if ((P_CB) && (P_CB)->P_CBACK) {              \
            (P_CB)->P_CBACK(__VA_ARGS__);             \
        } else {                                      \
            BT_LOGD("%s Callback is NULL", __func__); \
        }                                             \
    } while (0)

/**
 * @brief Gatt write type
 */
typedef enum {
    GATT_WRITE_TYPE_NO_RSP = 0, /*!< Gatt write attribute need no response */
    GATT_WRITE_TYPE_RSP, /*!< Gatt write attribute need remote response */
} gatt_write_type_t;

/**
 * @brief Gatt change type
 */
typedef enum {
    GATT_CHANGE_TYPE_NOTIFY = 0,
    GATT_CHANGE_TYPE_INDICATE,
} gatt_change_type_t;

/**
 * @brief Gatt discover type
 */
typedef enum {
    GATT_DISCOVER_ALL = 0,
    GATT_DISCOVER_SERVICE,
} gatt_discover_type_t;

/**
 * @brief Gatt element content
 */
typedef struct {
    uint16_t handle; /* Attribute handle */
    bt_uuid_t uuid; /* Attribute uuid */
    gatt_attr_type_t type; /* Attribute type */
    uint16_t properties; /* Attribute properties */
    uint16_t permissions; /* Attribute access permissions */

    union {
        /* gatt server content */
        struct {
            gatt_attr_rsp_t rsp_type;

            attribute_read_cb_t read_cb;
            attribute_written_cb_t write_cb;

            uint16_t attr_length; /** attr data length */
            void* attr_data; /** attr data array */
        };

        /* gatt client content */
        struct {

            bool notify_enable;
        };
    };

} gatt_element_t;

#endif /* __GATT_DEFINE_H__ */