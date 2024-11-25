#ifndef __BT_DEBUG_H__
#define __BT_DEBUG_H__

#ifdef __cplusplus
extern "C" {
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
