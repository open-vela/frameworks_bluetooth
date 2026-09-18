/* SPDX-License-Identifier: Apache-2.0 */
#ifndef SAL_GATT_DB_OWNERSHIP_H
#define SAL_GATT_DB_OWNERSHIP_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static inline bool sal_gatt_db_user_data_is_owned(size_t copied_size,
                                                   bool take_borrowed)
{
    return copied_size != 0 || take_borrowed;
}

static inline void sal_gatt_db_ownership_remove(bool* ownership,
                                                size_t attr_count,
                                                size_t index,
                                                size_t count)
{
    if (index + count < attr_count) {
        memmove(&ownership[index], &ownership[index + count],
            (attr_count - index - count) * sizeof(ownership[0]));
    }

    memset(&ownership[attr_count - count], 0,
        count * sizeof(ownership[0]));
}

#endif
