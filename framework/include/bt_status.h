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

#ifndef _BT_STATUS_H__
#define _BT_STATUS_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BT_STATUS_SUCCESS = 0, /* Success */
    BT_STATUS_FAIL, /* Failure */
    BT_STATUS_NOT_ENABLED, /* Bluetooth service not enabled */
    BT_STATUS_NOMEM, /* Heap not enough */
    BT_STATUS_BUSY, /* Busy */
    BT_STATUS_PARM_INVALID, /* Invalid parameter */
    BT_STATUS_NOT_SUPPORTED, /* Profile or festure not enable, should set related config */
    BT_STATUS_DEVICE_NOT_FOUND, /* Device not found */
    BT_STATUS_SERVICE_NOT_FOUND, /* Profile service not enable */
    BT_STATUS_NOT_FOUND, /* Can't get what you expect */
    BT_STATUS_NO_RESOURCES, /* Can't alloc resource in manager */
    BT_STATUS_ERROR_BUT_UNKNOWN, /* Unknown error */
    BT_STATUS_IPC_ERROR, /* IPC communication error */

    /* for acl connection status */
    BT_STATUS_PAGE_TIMEOUT = 0x20,
    BT_STATUS_AUTH_FAILURE, /* remote accepts AUTH request, but AUTH failure */
    BT_STATUS_AUTH_REJECTED, /* remote rejects AUTH request */
    BT_STATUS_RMT_DEV_DOWN, /* remote device not in BT range */
    BT_STATUS_RMT_DEV_TERMINATE, /* remote disconnect the link actively */
    BT_STATUS_LOCAL_TERMINATED, /* local disconnect the link or cancel connecting */
} bt_status_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_STATUS_H__ */
