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
#ifndef __BT_GATT_DEFS_H__
#define __BT_GATT_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @cond
 */

typedef enum {
    GATT_STATUS_SUCCESS,
    GATT_STATUS_FAILURE,
    GATT_STATUS_REQUEST_NOT_SUPPORTED,
    GATT_STATUS_INSUFFICIENT_AUTHENTICATION,
    GATT_STATUS_INSUFFICIENT_ENCRYPTION,
    GATT_STATUS_READ_NOT_PERMITTED,
    GATT_STATUS_WRITE_NOT_PERMITTED,
    GATT_STATUS_INVALID_ATTRIBUTE_LENGTH
} gatt_status_t;

typedef enum {
    GATT_PRIMARY_SERVICE,
    GATT_SECONDARY_SERVICE,
    GATT_INCLUDED_SERVICE,
    GATT_CHARACTERISTIC,
    GATT_DESCRIPTOR
} gatt_attr_type_t;

typedef enum {
    ATTR_AUTO_RSP,
    ATTR_RSP_BY_APP,
} gatt_attr_rsp_t;

typedef enum {
    ATT_BEAR_TYPE_NONE,
    ATT_BEAR_TYPE_LE_ATT,
    ATT_BEAR_TYPE_LE_EATT,
    ATT_BEAR_TYPE_BR_ATT,
    ATT_BEAR_TYPE_BR_EATT,
    ATT_BEAR_TYPE_ANY,
} gatt_bear_type_t;

#ifdef CONFIG_BLUETOOTH_GATTS_MAX_ATTRIBUTE_NUM
#define GATTS_MAX_ATTRIBUTE_NUM CONFIG_BLUETOOTH_GATTS_MAX_ATTRIBUTE_NUM
#else
#define GATTS_MAX_ATTRIBUTE_NUM 16
#endif

/* MAX GATT MTU size */
#define GATT_MAX_MTU_SIZE 517

/* Attribute permissions */
#define GATT_PERM_READ 0x01
#define GATT_PERM_WRITE 0x02
#define GATT_PERM_ENCRYPT_REQUIRED 0x04
#define GATT_PERM_AUTHEN_REQUIRED 0x08
#define GATT_PERM_MITM_REQUIRED 0x10

/* Characteristic Properties */
#define GATT_PROP_BROADCAST 0x01
#define GATT_PROP_READ 0x02
#define GATT_PROP_WRITE_NR 0x04
#define GATT_PROP_WRITE 0x08
#define GATT_PROP_NOTIFY 0x10
#define GATT_PROP_INDICATE 0x20
#define GATT_PROP_SIGNED_WRITE 0x40
#define GATT_PROP_EXTENDED_PROPS 0x80
#define GATT_PROP_EXPOSED_OVER_BREDR 0x1000 /* Applies to Primary/Secondary Service type only */

/* Client Characteristic Configuration Values */
#define GATT_CCC_NOTIFY 0x0001
#define GATT_CCC_INDICATE 0x0002

/* GATT Attribute Helper Macros */
#define GATT_H_ATTRIBUTE(_uuid, _type, _prop, _perm, _rsp, _read, _write, _value, _length, _handle) \
    {                                                                                               \
        .handle = _handle,                                                                          \
        .uuid = _uuid,                                                                              \
        .type = _type,                                                                              \
        .properties = _prop,                                                                        \
        .permissions = _perm,                                                                       \
        .rsp_type = _rsp,                                                                           \
        .read_cb = _read,                                                                           \
        .write_cb = _write,                                                                         \
        .attr_length = _length,                                                                     \
        .attr_value = _value                                                                        \
    }

/* GATT_H_PRIMARY_SERVICE */
#define GATT_H_PRIMARY_SERVICE(_service, _handle) \
    GATT_H_ATTRIBUTE(_service, GATT_PRIMARY_SERVICE, 0, GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, NULL, 0, _handle)

/* GATT_H_SECONDARY_SERVICE */
#define GATT_H_SECONDARY_SERVICE(_service, _handle) \
    GATT_H_ATTRIBUTE(_service, GATT_SECONDARY_SERVICE, 0, GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, NULL, 0, _handle)

/* GATT_H_INCLUDE_SERVICE */
#define GATT_H_INCLUDE_SERVICE(_service, _handle) \
    GATT_H_ATTRIBUTE(_service, GATT_INCLUDED_SERVICE, 0, GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, NULL, 0, _handle)

/* GATT_H_PRIMARY_SERVICE_OVER_BREDR */
#define GATT_H_PRIMARY_SERVICE_OVER_BREDR(_service, _handle) \
    GATT_H_ATTRIBUTE(_service, GATT_PRIMARY_SERVICE, GATT_PROP_EXPOSED_OVER_BREDR, GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, NULL, 0, _handle)

/* GATT_H_SECONDARY_SERVICE_OVER_BREDR */
#define GATT_H_SECONDARY_SERVICE_OVER_BREDR(_service, _handle) \
    GATT_H_ATTRIBUTE(_service, GATT_SECONDARY_SERVICE, GATT_PROP_EXPOSED_OVER_BREDR, GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, NULL, 0, _handle)

/* GATT_H_CHARACTERISTIC */
#define GATT_H_CHARACTERISTIC(_uuid, _prop, _perm, _rsp, _read, _write, _value, _length, _handle) \
    GATT_H_ATTRIBUTE(_uuid, GATT_CHARACTERISTIC, _prop, _perm, _rsp, _read, _write, _value, _length, _handle)

/* GATT_H_CHARACTERISTIC */
#define GATT_H_CHARACTERISTIC_AUTO_RSP(_uuid, _prop, _perm, _value, _length, _handle) \
    GATT_H_ATTRIBUTE(_uuid, GATT_CHARACTERISTIC, _prop, _perm, ATTR_AUTO_RSP, NULL, NULL, _value, _length, _handle)

/* GATT_H_CHARACTERISTIC */
#define GATT_H_CHARACTERISTIC_USER_RSP(_uuid, _prop, _perm, _read, _write, _handle) \
    GATT_H_ATTRIBUTE(_uuid, GATT_CHARACTERISTIC, _prop, _perm, ATTR_RSP_BY_APP, _read, _write, NULL, 0, _handle)

/* GATT_H_DESCRIPTOR */
#define GATT_H_DESCRIPTOR(_uuid, _perm, _rsp, _read, _write, _value, _length, _handle) \
    GATT_H_ATTRIBUTE(_uuid, GATT_DESCRIPTOR, 0, _perm, _rsp, _read, _write, _value, _length, _handle)

/* GATT_H_DESCRIPTOR */
#define GATT_H_CEPD(_value, _length, _handle) \
    GATT_H_DESCRIPTOR(BT_UUID_DECLARE_16(0x2900), GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, _value, _length, _handle)

/* GATT_H_DESCRIPTOR */
#define GATT_H_CUDD(_value, _length, _handle) \
    GATT_H_DESCRIPTOR(BT_UUID_DECLARE_16(0x2901), GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, _value, _length, _handle)

/* GATT_H_DESCRIPTOR */
#define GATT_H_CCCD(_perm, _change, _handle) \
    GATT_H_DESCRIPTOR(BT_UUID_DECLARE_16(0x2902), _perm, ATTR_RSP_BY_APP, NULL, _change, NULL, 0, _handle)

/* GATT_H_DESCRIPTOR */
#define GATT_H_CPFD(_value, _length, _handle) \
    GATT_H_DESCRIPTOR(BT_UUID_DECLARE_16(0x2904), GATT_PERM_READ, ATTR_AUTO_RSP, NULL, NULL, _value, _length, _handle)

/**
 * @endcond
 */

#ifdef __cplusplus
}
#endif

#endif /* __BT_GATT_DEFS_H_ */