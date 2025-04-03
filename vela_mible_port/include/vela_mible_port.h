/***********************************************************************
 *
 * Copyright 2025 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#ifndef __VELA_MIBLE_PORT_H__
#define __VELA_MIBLE_PORT_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * @brief In the vela system, used to start Bluetooth Low Energy advertising
 *
 * This function is used to start a BLE advertising operation in the vela system, based on
 * the given configuration parameters.
 * It allows setting the duration of the advertisement, advertising data (Advertising Data),
 * service data (Service Data), and their respective lengths.
 *
 * @param param  A pointer to the Bluetooth Low Energy advertising parameters. These parameters
 *               control the settings for the advertising, including the advertising type, advertising
 *               interval, etc.
 * @param duration The duration of the advertisement in milliseconds. The advertisement will remain
 *                 active for this period. If a negative value or 0 is passed, it indicates the 
 *                 advertisement will continue indefinitely until explicitly stopped.
 * @param ad      A pointer to the advertising data. Advertising data typically contains information about
 *                the device, such as its name, service UUIDs, etc.
 * @param ad_len  The length of the advertising data in bytes. This value should match the actual length of
 *                the advertising data.
 * @param sd      A pointer to the service data. Service data is typically used to transmit specific 
 *                service-related information.
 * @param sd_len  The length of the service data in bytes. This value should match the actual length of the service data.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values may include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid parameters or insufficient resources.
 */
int vela_mible_adv_start(const struct bt_le_adv_param *param, int32_t duration,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len);

int vela_mible_adv_stop(void);

/**
 * @brief Start Bluetooth Low Energy scanning in the vela system
 *
 * This function is used to start a BLE scanning operation in the vela system. You can configure the scanning behavior
 * using the provided scan parameters and provide a callback function to handle scan results.
 *
 * @param param  A pointer to the Bluetooth scan parameters, which include configurations such as scan interval, window, etc.
 * @param cb     The scan callback function. This function will be called to handle scan results (e.g., when a device is found).
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid parameters or insufficient resources.
 */
int vela_mible_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb);

/**
 * @brief Stop Bluetooth Low Energy scanning in the vela system
 *
 * This function is used to stop an ongoing BLE scanning operation in the vela system. It terminates the scan
 * that was previously started using the corresponding start function.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as no scan in progress or an internal failure.
 */
int vela_mible_scan_stop(void);

/**
 * @brief Update the parameters of an ongoing Bluetooth Low Energy extended advertisement
 *
 * This function is used to update the parameters of an ongoing BLE extended advertisement. You can modify 
 * various parameters of the advertisement such as interval, type, and address. This allows adjusting the advertisement 
 * settings dynamically during the advertisement operation.
 *
 * @param adv    A pointer to the extended advertising object (`struct bt_le_ext_adv`) representing the ongoing advertisement.
 * @param param  A pointer to the new Bluetooth advertising parameters (`struct bt_le_adv_param`) that should be applied.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid parameters or the inability to update the advertisement.
 */
int vela_le_ext_adv_update_param(struct bt_le_ext_adv *adv,
			       const struct bt_le_adv_param *param);

/**
 * @brief Set advertising data for an ongoing Bluetooth Low Energy extended advertisement
 *
 * This function is used to set the advertising and service data for an ongoing BLE extended advertisement.
 * It allows updating the advertising data (`ad`) and service data (`sd`) dynamically during the advertisement.
 *
 * @param adv    A pointer to the extended advertising object (`struct bt_le_ext_adv`) representing the ongoing advertisement.
 * @param ad     A pointer to the advertising data (`struct bt_data`) to be set for the extended advertisement.
 * @param ad_len The length of the advertising data in bytes.
 * @param sd     A pointer to the service data (`struct bt_data`) to be set for the extended advertisement.
 * @param sd_len The length of the service data in bytes.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid data or the inability to update the advertisement.
 */
int vela_le_ext_adv_set_data(struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len);

/**
 * @brief Start a Bluetooth Low Energy extended advertisement
 *
 * This function is used to start an extended BLE advertisement with the specified parameters.
 * It initializes the extended advertisement using the given advertisement object and parameters, enabling
 * the advertisement to begin transmitting.
 *
 * @param adv    A pointer to the extended advertising object (`struct bt_le_ext_adv`) that represents the advertisement.
 * @param param  A pointer to the extended advertisement start parameters (`struct bt_le_ext_adv_start_param`), 
 *               which include settings such as duration, flags, and whether to start the advertisement immediately.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid parameters or the failure to start the advertisement.
 */
int vela_le_ext_adv_start(struct bt_le_ext_adv *adv,
			const struct bt_le_ext_adv_start_param *param);

/**
 * @brief Stop an ongoing Bluetooth Low Energy extended advertisement
 *
 * This function is used to stop an ongoing BLE extended advertisement that was previously started.
 * It terminates the advertisement and releases the associated resources.
 *
 * @param adv    A pointer to the extended advertising object (`struct bt_le_ext_adv`) representing the ongoing advertisement.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as no advertisement in progress or a failure to stop the advertisement.
 */
int vela_le_ext_adv_stop(struct bt_le_ext_adv *adv);

/**
 * @brief Delete a Bluetooth Low Energy extended advertisement object
 *
 * This function is used to delete an extended BLE advertisement object that was previously created.
 * It cleans up the resources associated with the advertisement and removes the object from the system.
 *
 * @param adv    A pointer to the extended advertising object (`struct bt_le_ext_adv`) that represents the advertisement to be deleted.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as the advertisement object not being valid or failure to delete the object.
 */
int vela_le_ext_adv_delete(struct bt_le_ext_adv *adv);

/**
 * @brief Create a Bluetooth Low Energy extended advertisement object
 *
 * This function creates an extended BLE advertisement object based on the given advertisement parameters.
 * It initializes the advertisement object and provides a callback to handle events related to the advertisement.
 *
 * @param param   A pointer to the Bluetooth advertising parameters (`struct bt_le_adv_param`), which define the configuration for the advertisement.
 * @param cb      A pointer to the callback functions (`struct bt_le_ext_adv_cb`) that will handle events related to the advertisement (such as start, stop, or error events).
 * @param out_adv A pointer to a pointer (`struct bt_le_ext_adv **`) where the created extended advertisement object will be stored upon success.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid parameters or failure to create the advertisement object.
 */
int vela_le_ext_adv_create(const struct bt_le_adv_param *param,
			 const struct bt_le_ext_adv_cb *cb,
			 struct bt_le_ext_adv **out_adv);

/**
 * @brief Enable or disable an ongoing Bluetooth Low Energy extended advertisement
 *
 * This function is used to enable or disable a BLE extended advertisement. It starts or stops the advertisement based
 * on the `enable` flag and applies the provided start parameters when enabling the advertisement.
 *
 * @param adv     A pointer to the extended advertising object (`struct bt_le_ext_adv`), which represents the advertisement to be controlled.
 * @param enable  A boolean value indicating whether to enable or disable the advertisement:
 *                - `true` to enable the advertisement.
 *                - `false` to disable the advertisement.
 * @param param   A pointer to the extended advertisement start parameters (`struct bt_le_ext_adv_start_param`), which define
 *                the configuration for starting the advertisement. This is only used when enabling the advertisement.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid parameters or failure to enable/disable the advertisement.
 */
int vela_le_adv_set_enable_ext(struct bt_le_ext_adv *adv,
			 bool enable,
			 const struct bt_le_ext_adv_start_param *param);

int vela_mible_gap_init(void);

int vela_mible_gap_deinit(void);

/**
 * @brief Register a Bluetooth Low Energy GATT service
 *
 * This function is used to register a GATT (Generic Attribute Profile) service with the BLE stack in the vela system.
 * Once registered, the service becomes available for interactions with remote Bluetooth devices.
 *
 * @param svc    A pointer to the GATT service structure (`struct bt_gatt_service`) that defines the service to be registered.
 *               This structure includes information such as the service UUID, characteristics, and descriptors.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid service definition or failure to register the service.
 */
int vela_le_gatt_service_register(struct bt_gatt_service *svc);

/**
 * @brief Disconnect a Bluetooth Low Energy connection
 *
 * This function is used to disconnect an active BLE connection. It terminates the connection to the specified peer device
 * and provides a reason code for the disconnection.
 *
 * @param conn   A pointer to the Bluetooth connection object (`struct bt_conn`) representing the active connection to be disconnected.
 * @param reason A uint8_t value representing the reason for the disconnection. It follows the Bluetooth specification for disconnection reasons,
 *               and the specific reason code will provide information on why the connection was terminated.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as an invalid connection object or failure to disconnect the connection.
 */
int vela_le_conn_disconnect(struct bt_conn *conn, uint8_t reason);

/**
 * @brief Unregister a Bluetooth Low Energy GATT service
 *
 * This function is used to unregister a previously registered GATT (Generic Attribute Profile) service from the BLE stack in the vela system.
 * After unregistration, the service will no longer be available for interactions with remote Bluetooth devices.
 *
 * @param svc    A pointer to the GATT service structure (`struct bt_gatt_service`) that defines the service to be unregistered.
 *               This structure includes information such as the service UUID, characteristics, and descriptors.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as an invalid service definition or failure to unregister the service.
 */
int vela_le_gatt_service_unregister(struct bt_gatt_service *svc);

/**
 * @brief Create a Bluetooth Low Energy connection
 *
 * This function is used to create a BLE connection to a remote device, initiating a connection process to the specified peer device.
 * It provides parameters to control the connection process, including connection settings and connection parameters.
 *
 * @param peer          A pointer to the Bluetooth address of the peer device (`bt_addr_le_t`) that you want to connect to.
 * @param create_param A pointer to the connection creation parameters (`struct bt_conn_le_create_param`) that define settings for the connection process.
 * @param conn_param   A pointer to the connection parameters (`struct bt_le_conn_param`), such as connection interval, latency, and timeout.
 * @param ret_conn     A pointer to a pointer (`struct bt_conn **`) that will store the newly created connection object upon success.
 *
 * @return Returns an integer value, typically the status code of the operation. Common return values include:
 *         - 0 indicates success.
 *         - A negative value indicates an error, such as invalid parameters or failure to create the connection.
 */
int vela_le_conn_le_create(const bt_addr_le_t *peer, const struct bt_conn_le_create_param *create_param,
		      const struct bt_le_conn_param *conn_param, struct bt_conn **ret_conn);

/**
 * @brief Initiates a GATT discovery procedure on a Bluetooth connection.
 *
 * This function is used to start the discovery of GATT services and characteristics on a given
 * Bluetooth connection. The parameters passed allow the function to configure the type of
 * discovery to be performed and handle the response from the GATT server.
 *
 * @param conn A pointer to the Bluetooth connection object. This represents the connection
 *             over which the GATT discovery is to be performed.
 * @param params A pointer to the GATT discovery parameters. This structure contains the 
 *               settings and callback functions for the discovery procedure.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 *
 * @note This function is typically called as part of a larger GATT service discovery process 
 *       within a Bluetooth Low Energy (BLE) application.
 */
int vela_le_gatt_discover(struct bt_conn *conn,
		     struct bt_gatt_discover_params *params);

/**
 * @brief Subscribes to notifications or indications for a GATT characteristic.
 *
 * This function is used to subscribe to receive notifications or indications from a GATT 
 * characteristic on a Bluetooth connection. The parameters provided allow the function to
 * configure the subscription and handle incoming notifications or indications from the GATT server.
 *
 * @param conn A pointer to the Bluetooth connection object. This represents the connection
 *             over which the GATT subscription is to be made.
 * @param params A pointer to the GATT subscription parameters. This structure defines the
 *               subscription settings, including the characteristic to subscribe to, the
 *               callback functions for handling notifications/indications, and other options.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 *
 * @note This function is typically used in scenarios where the application needs to be notified
 *       when a GATT characteristic value changes, such as receiving sensor data or other updates.
 */
int vela_le_gatt_subscribe(struct bt_conn *conn,
		      struct bt_gatt_subscribe_params *params);

/**
 * @brief Writes data to a GATT characteristic without expecting a response.
 *
 * This function writes data to a specified GATT characteristic on a Bluetooth connection. It
 * is used when the application does not require a response from the GATT server after the write
 * operation, allowing for faster communication. The function can also support signing the 
 * write request if required.
 *
 * @param conn A pointer to the Bluetooth connection object. This represents the connection
 *             over which the GATT write operation will be performed.
 * @param handle The handle of the GATT characteristic to which the data will be written.
 * @param data A pointer to the data to be written to the GATT characteristic.
 * @param length The length of the data to be written.
 * @param sign A boolean indicating whether the write operation should be signed. If `true`,
 *             the data will be signed before being sent.
 * @param func A callback function to be invoked when the write operation is complete.
 *             The callback function should match the `bt_gatt_complete_func_t` type.
 * @param user_data A pointer to user data that will be passed to the callback function when
 *                  the write operation completes.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 *
 * @note This function is used when the application does not need to wait for a response from
 *       the GATT server after the write operation. It can be useful for cases where the server
 *       is expected to receive data but does not need to acknowledge it, such as sending
 *       control commands or updating sensor data.
 */
int vela_le_gatt_write_without_response_cb(struct bt_conn *conn, uint16_t handle,
				      const void *data, uint16_t length, bool sign,
				      bt_gatt_complete_func_t func,
				      void *user_data);

/**
 * @brief Handles GATT notifications on a Bluetooth connection.
 *
 * This function is used to handle incoming notifications from a GATT characteristic on a
 * Bluetooth connection. It is called when the GATT server sends a notification to the client
 * regarding a characteristic's value change. The parameters passed to this function contain
 * the details of the notification, including the characteristic's handle and the data being
 * notified.
 *
 * @param conn A pointer to the Bluetooth connection object. This represents the connection
 *             on which the GATT notification was received.
 * @param params A pointer to the GATT notification parameters. This structure contains the
 *               handle of the characteristic being notified and the data received in the
 *               notification.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 * 
 * @note This function is typically used in applications that need to handle incoming
 *       notifications, such as monitoring sensor data or receiving control updates from a
 *       remote device. The callback should be registered as part of a GATT subscription process.
 */
int vela_le_gatt_notify_cb(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__VELA_MIBLE_PORT_H__
