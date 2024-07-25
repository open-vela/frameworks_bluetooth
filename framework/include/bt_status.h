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

#ifndef ANDROID_LIBBLUETOOTH
/*
 * hardware/libhardware/include/hardware/bluetooth.h
 * This definition was copied from Android 13.
 * DON NOT MODIFY IT !!!
 */
typedef enum {
    BT_STATUS_SUCCESS, /* Success */
    BT_STATUS_FAIL, /* Failure */
    BT_STATUS_NOT_READY,
    BT_STATUS_NOMEM, /* Heap not enough */
    BT_STATUS_BUSY, /* Busy */
    BT_STATUS_DONE, /* request already completed */
    BT_STATUS_UNSUPPORTED,
    BT_STATUS_PARM_INVALID, /* Invalid parameter */
    BT_STATUS_UNHANDLED,
    BT_STATUS_AUTH_FAILURE, /* remote accepts AUTH request, but AUTH failure */
    BT_STATUS_RMT_DEV_DOWN, /* remote device not in BT range */
    BT_STATUS_AUTH_REJECTED, /* remote rejects AUTH request */
    BT_STATUS_JNI_ENVIRONMENT_ERROR,
    BT_STATUS_JNI_THREAD_ATTACH_ERROR,
    BT_STATUS_WAKELOCK_ERROR
} bt_status_t;
#endif

// Rename the status
#define BT_STATUS_NOT_ENABLED BT_STATUS_NOT_READY /* Bluetooth service not enabled */
#define BT_STATUS_NOT_SUPPORTED BT_STATUS_UNSUPPORTED /* Profile or festure not enable, should set related config */

// !!! If Android add more definitioin of status, please don't forget to modify this definition!!!
#define BT_STATUS_EXT_START (BT_STATUS_WAKELOCK_ERROR + 0x20)

// Add new status
#define BT_STATUS_ERROR_BUT_UNKNOWN (BT_STATUS_EXT_START + 0x01) /* Unknown error */
#define BT_STATUS_NOT_FOUND (BT_STATUS_EXT_START + 0x02) /* Can't get what you expect */
#define BT_STATUS_DEVICE_NOT_FOUND (BT_STATUS_EXT_START + 0x03) /* Device not found */
#define BT_STATUS_SERVICE_NOT_FOUND (BT_STATUS_EXT_START + 0x04) /* Profile service not enable */
#define BT_STATUS_NO_RESOURCES (BT_STATUS_EXT_START + 0x05) /* Can't alloc resource in manager */
#define BT_STATUS_IPC_ERROR (BT_STATUS_EXT_START + 0x06) /* IPC communication error */
#define BT_STATUS_PAGE_TIMEOUT (BT_STATUS_EXT_START + 0x07)
#define BT_STATUS_RMT_DEV_TERMINATE (BT_STATUS_EXT_START + 0x08) /* remote disconnect the link actively */
#define BT_STATUS_LOCAL_TERMINATED (BT_STATUS_EXT_START + 0x09) /* local disconnect the link or cancel connecting */

#ifdef __cplusplus
}
#endif

#endif /* _BT_STATUS_H__ */
