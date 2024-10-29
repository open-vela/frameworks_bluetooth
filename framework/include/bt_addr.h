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
#ifndef __BT_ADDR_H__
#define __BT_ADDR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BT_ADDR_LENGTH 6 /*define the address length*/
#define BT_ADDR_STR_LENGTH 18

/**
 * @cond
 */
typedef struct bt_addr {
    uint8_t addr[BT_ADDR_LENGTH];
} bt_address_t;
/**
 * @endcond
 */

/**
 * @cond
 */
typedef struct bt_le_addr {
    uint8_t addr[BT_ADDR_LENGTH];
    uint8_t addr_type;
} bt_le_address_t;
/**
 * @endcond
 */

/**
 * @brief Check if a Bluetooth address is empty.
 *
 * Determines if the given Bluetooth address is all zeros.
 *
 * @param addr - Pointer to the Bluetooth address to check.
 *               This should point to a valid `bt_address_t` structure that contains a 6-byte Bluetooth address.
 * @return true - The address is empty (all zeros).
 * @return false - The address is not empty.
 *
 * **Example:**
 * @code
 * bt_address_t addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Example empty address
 * if (bt_addr_is_empty(&addr)) {
 *     // Address is empty
 * }
 * @endcode
 */
bool bt_addr_is_empty(const bt_address_t* addr);


/**
 * @brief Set a Bluetooth address to empty.
 *
 * Sets the given Bluetooth address to all zeros.
 *
 * @param addr - Pointer to the Bluetooth address to set to empty.
 *
 * **Example:**
 * @code
 * bt_address_t addr;
 * bt_addr_set_empty(&addr);  // Set the address to all zeros
 * @endcode
 */
void bt_addr_set_empty(bt_address_t* addr);


/**
 * @brief Compare two Bluetooth addresses.
 *
 * Compares two Bluetooth addresses.
 *
 * @param a - Pointer to the first Bluetooth address (type `bt_address_t`).
 * @param b - Pointer to the second Bluetooth address (type `bt_address_t`).
 * @return int - Returns zero if the addresses are equal, non-zero otherwise.
 *
 * **Example:**
 * @code
 * bt_address_t addr1 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
 * bt_address_t addr2 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
 * if (bt_addr_compare(&addr1, &addr2) == 0) {
 *     // Addresses are equal
 * }
 * @endcode
 */
int bt_addr_compare(const bt_address_t* a, const bt_address_t* b);


/**
 * @brief Convert a Bluetooth address to a string.
 *
 * Converts a Bluetooth address to its string representation in the format "XX:XX:XX:XX:XX:XX".
 *
 * @param addr - Pointer to the Bluetooth address.
 *               This should point to a valid `bt_address_t` structure.
 * @param str - Pointer to a buffer to store the string representation. The buffer must be at least `BT_ADDR_STR_LENGTH` bytes long.
 * @return int - Returns zero on success, negative value on failure.
 *
 * **Example:**
 * @code
 * bt_address_t addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
 * char str[BT_ADDR_STR_LENGTH];
 * if (bt_addr_ba2str(&addr, str) == 0) {
 *     // str now contains the address string in the format "00:11:22:33:44:55"
 * }
 * @endcode
 */
int bt_addr_ba2str(const bt_address_t* addr, char* str);


/**
 * @brief Convert a string to a Bluetooth address.
 *
 * Parses a string representation of a Bluetooth address and stores it in a `bt_address_t` structure.
 *
 * @param str - Pointer to the string containing the Bluetooth address (format "XX:XX:XX:XX:XX:XX").
 * @param addr - Pointer to the Bluetooth address structure to store the result.
 *               After calling this function, `addr` will contain the parsed Bluetooth address.
 * @return int - Returns zero on success, negative value on failure.
 *
 * **Example:**
 * @code
 * bt_address_t addr;
 * if (bt_addr_str2ba("00:11:22:33:44:55", &addr) == 0) {
 *     // addr now contains the parsed address in the format {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
 * }
 * @endcode
 */
int bt_addr_str2ba(const char* str, bt_address_t* addr);


/**
 * @brief Get the string representation of a Bluetooth address.
 *
 * Returns a string representation of the Bluetooth address.
 *
 * @param addr - Pointer to the Bluetooth address.
 *               This should point to a valid `bt_address_t` structure.
 * @return char* - Pointer to a static string containing the address in "XX:XX:XX:XX:XX:XX" format.
 *
 * **Note:** The returned string is stored in a static buffer and may be overwritten by subsequent calls.
 *
 * **Example:**
 * @code
 * bt_address_t addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
 * printf("Address: %s\n", bt_addr_str(&addr));
 * // Output: Address: 00:11:22:33:44:55
 * @endcode
 */
char* bt_addr_str(const bt_address_t* addr);


/**
 * @brief Set a Bluetooth address.
 *
 * Sets the Bluetooth address from a byte array.
 *
 * @param addr - Pointer to the Bluetooth address structure to set.
 *               This should point to a valid `bt_address_t` structure that will be updated with the new address.
 * @param bd - Pointer to a byte array containing the address bytes (of length `BT_ADDR_LENGTH`).
 *             The array should contain exactly 6 bytes representing the Bluetooth address.
 *
 * **Example:**
 * @code
 * bt_address_t addr;
 * uint8_t bd[BT_ADDR_LENGTH] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
 * bt_addr_set(&addr, bd);
 * // addr now contains the address {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
 * @endcode
 */
void bt_addr_set(bt_address_t* addr, const uint8_t* bd);


/**
 * @brief Swap byte order of a Bluetooth address.
 *
 * Swaps the byte order of a Bluetooth address (e.g., from little-endian to big-endian).
 *
 * @param src - Pointer to the source Bluetooth address (type `bt_address_t`).
 * @param dest - Pointer to the destination Bluetooth address where the swapped address will be stored.
 *               This should point to a valid `bt_address_t` structure.
 *
 * **Example:**
 * @code
 * bt_address_t addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
 * bt_address_t swapped_addr;
 * bt_addr_swap(&addr, &swapped_addr);
 * // swapped_addr now contains the address with byte order swapped
 * @endcode
 */
void bt_addr_swap(const bt_address_t* src, bt_address_t* dest);

#ifdef __cplusplus
}
#endif

#endif /* __BT_ADDR_H__ */
