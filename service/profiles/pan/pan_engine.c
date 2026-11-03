/* pan_engine.c -- auto pair / auto PAN / reconnect / persistence. */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

#include "bt_addr.h"
#include "sal_pan_interface.h"

#define PAN_LOG(fmt, ...) syslog(LOG_INFO,  "[pan] " fmt "\n", ##__VA_ARGS__)
#define PAN_ERR(fmt, ...) syslog(LOG_ERR,   "[pan] " fmt "\n", ##__VA_ARGS__)

#define LAST_NAP_PATH  "/data/misc/bt/last_nap"
#define RECONNECT_MAX  10
#define BACKOFF_MIN_MS 1000
#define BACKOFF_MAX_MS 30000

enum { ENG_WAIT_BOND, ENG_PAN_CONNECT, ENG_ONLINE, ENG_RECONNECT, ENG_STOPPED };

static int              g_state = ENG_WAIT_BOND;
static int              g_failures;
static bt_address_t     g_nap;
static bool             g_have_nap;
static pthread_t        g_thread;
static pthread_mutex_t  g_lock = PTHREAD_MUTEX_INITIALIZER;

static void set_state(int s, const char *why)
{
    static const char *n[] = { "wait-bond", "pan-connect", "online",
                               "reconnect", "stopped" };
    if (s != g_state) PAN_LOG("state=%s (%s)", n[s], why ? why : "");
    g_state = s;
}

/* ---------------- persistence: 18-byte MAC string ---------------- */
static int nap_save(const bt_address_t *a)
{
    char s[18];
    int fd;

    snprintf(s, sizeof(s), "%02x:%02x:%02x:%02x:%02x:%02x",
             a->addr[5], a->addr[4], a->addr[3],
             a->addr[2], a->addr[1], a->addr[0]);
    fd = open(LAST_NAP_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) { PAN_ERR("save last_nap failed errno=%d", errno); return -errno; }
    write(fd, s, 18);
    close(fd);
    PAN_LOG("persisted last_nap=%s", s);
    return 0;
}

static int nap_load(bt_address_t *out)
{
    char s[19] = { 0 };
    unsigned b[6];
    int fd = open(LAST_NAP_PATH, O_RDONLY);
    if (fd < 0) return -ENOENT;
    if (read(fd, s, 18) != 18) { close(fd); return -EIO; }
    close(fd);
    if (sscanf(s, "%02x:%02x:%02x:%02x:%02x:%02x",
               &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) return -EIO;
    for (int i = 0; i < 6; i++) out->addr[5 - i] = (uint8_t)b[i];
    PAN_LOG("loaded last_nap=%s", s);
    return 0;
}

/* ---------------- reconnect with exponential backoff ---------------- */
static void try_connect(void)
{
    set_state(ENG_PAN_CONNECT, "connect");
    /* dst_role=NAP, src_role=PANU -- per pan_interface_t doc */
    if (bt_sal_pan_connect(&g_nap, PAN_ROLE_NAP, PAN_ROLE_PANU) != BT_STATUS_SUCCESS) {
        PAN_ERR("pan connect failed -> backoff");
        set_state(ENG_RECONNECT, "connect failed");
    }
}

static void *engine_thread(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t ms;
        int st, i;

        usleep(200 * 1000);
        pthread_mutex_lock(&g_lock);
        st = g_state;
        pthread_mutex_unlock(&g_lock);
        if (st != ENG_RECONNECT) continue;

        if (g_failures >= RECONNECT_MAX) {
            PAN_ERR("give up after %d attempts, wait for phone", g_failures);
            g_failures = 0;
            set_state(ENG_WAIT_BOND, "reconnect exhausted");
            continue;
        }
        g_failures++;

        ms = BACKOFF_MIN_MS;
        for (i = 1; i < g_failures && ms < BACKOFF_MAX_MS; i++) ms *= 2;
        if (ms > BACKOFF_MAX_MS) ms = BACKOFF_MAX_MS;

        PAN_LOG("reconnect attempt %d/%d, backoff %u ms",
                g_failures, RECONNECT_MAX, ms);
        usleep(ms * 1000);
        try_connect();
    }
    return NULL;
}

static void schedule_reconnect(void)
{
    set_state(ENG_RECONNECT, "link lost");
}

/* ---------------- hooks ---------------- */

/* 1) adapter service 的 bond 通知处调用 */
void pan_engine_on_bond_state(bt_address_t *addr, int bonded)
{
    if (!bonded) return;
    memcpy(&g_nap, addr, sizeof(g_nap));
    g_have_nap = true;
    g_failures = 0;
    nap_save(addr);
    PAN_LOG("state=adapter-on-auto-connect");
    try_connect();
}

/* 2) panu_service.c 的 pan_on_connection_state_changed() 分发后调用 */
void pan_engine_on_pan_state(profile_connection_state_t st)
{
    if (st == PROFILE_STATE_CONNECTED) {
        g_failures = 0;
        set_state(ENG_ONLINE, "bnep ready");
    } else if (st == PROFILE_STATE_DISCONNECTED) {
        if (g_have_nap && g_state != ENG_STOPPED) schedule_reconnect();
    }
}

/* 3) bt_service 起来后调用一次 */
void pan_engine_start(void)
{
    PAN_LOG("engine start");

    /* BOARD: enable / set scanmode 2 / pair auto 1 由 rcS + bttool 完成。
     * 若要在 service 侧做，这里调 sal_adapter_interface.c 的
     *   bt_sal_adapter_set_scan_mode(2)
     *   bt_sal_adapter_set_pair_mode(AUTO_ACCEPT)
     * （只调 setter，绝不要碰 set_name / bt_set_name）
     */

    if (nap_load(&g_nap) == 0) {
        g_have_nap = true;
        set_state(ENG_PAN_CONNECT, "boot with stored nap");
        try_connect();
    } else {
        set_state(ENG_WAIT_BOND, "no stored nap");
    }
    pthread_create(&g_thread, NULL, engine_thread, NULL);
}
