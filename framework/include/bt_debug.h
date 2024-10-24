#ifndef __BT_DEBUG_H__
#define __BT_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_time.h"

/* Time value structure for Bluetooth debug timing */
typedef struct timeval_s {
    uint64_t entry_time;
    uint64_t exit_time;
    uint64_t max_timeval;
    uint64_t last_timeval;
} bt_timeval_t;

/* Define macro to create a timeval structure for debugging */
#ifdef CONFIG_BLUETOOTH_DEBUG_TIMEVAL
#define BT_DEBUG_MKTIMEVAL_S(name_s) \
    bt_timeval_t g_timeval_##name_s = { 0, 0, 0, 0 }
#else
#define BT_DEBUG_MKTIMEVAL_S(name_s) /* No operation */
#endif

/* Macro to get current timestamp */
#ifdef CONFIG_BLUETOOTH_DEBUG_TIMEVAL
#ifdef CONFIG_BLUETOOTH_DEBUG_TIME_UNIT_US
#define _GetCurrTime() ((uint64_t)get_os_timestamp_us())
#else
#define _GetCurrTime() ((uint64_t)get_os_timestamp_ms())
#endif
#else
#define _GetCurrTime() 0
#endif

/* Macro to mark entry of a time section */
#ifdef CONFIG_BLUETOOTH_DEBUG_TIMEVAL
#define BT_DEBUG_ENTER_TIME_SECTION(name_s)             \
    do {                                                \
        g_timeval_##name_s.entry_time = _GetCurrTime(); \
    } while (0)
#else
#define BT_DEBUG_ENTER_TIME_SECTION(name_s) /* No operation */
#endif

/* Macro to mark exit of a time section */
#ifdef CONFIG_BLUETOOTH_DEBUG_TIMEVAL
#define BT_DEBUG_EXIT_TIME_SECTION(name_s)                                                                      \
    do {                                                                                                        \
        if (g_timeval_##name_s.entry_time != 0) {                                                               \
            g_timeval_##name_s.exit_time = _GetCurrTime();                                                      \
            if (g_timeval_##name_s.exit_time >= g_timeval_##name_s.entry_time) {                                \
                g_timeval_##name_s.last_timeval = g_timeval_##name_s.exit_time - g_timeval_##name_s.entry_time; \
                if (g_timeval_##name_s.last_timeval > g_timeval_##name_s.max_timeval) {                         \
                    g_timeval_##name_s.max_timeval = g_timeval_##name_s.last_timeval;                           \
                }                                                                                               \
            } else {                                                                                            \
                /* if exit_time < entry_time, reset all to zero */                                              \
                g_timeval_##name_s.entry_time = 0;                                                              \
                g_timeval_##name_s.exit_time = 0;                                                               \
                g_timeval_##name_s.last_timeval = 0;                                                            \
                g_timeval_##name_s.max_timeval = 0;                                                             \
            }                                                                                                   \
        }                                                                                                       \
    } while (0)
#else
#define BT_DEBUG_EXIT_TIME_SECTION(name_s) /* No operation */
#endif

#if defined(__NuttX__) // Vela

#include <debug.h>
#include <syslog.h>

#include "utils/log.h"

#elif defined(ANDROID) // Android

#include <android/log.h>
#include <assert.h>

#include "utils/log.h"

#ifndef LOG_TAG
#define LOG_TAG "VELA-BT"
#endif
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define syslog(x, ...) \
    do {               \
    } while (0)

#define lib_dumpbuffer(x, y, z) \
    do {                        \
    } while (0)

#define zalloc(x) calloc(x, 1)

#define OK 0

#define UNUSED(x) ((void)(x))

#endif // End of Android

#ifdef __cplusplus
}
#endif

#endif /* __BT_DEBUG_H__ */
