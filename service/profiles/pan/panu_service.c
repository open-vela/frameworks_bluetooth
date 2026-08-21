/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "panu"

#include <fcntl.h>
#include <net/if.h>
#include <nuttx/net/ethernet.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/tun.h>
#include <nuttx/net/dns.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "callbacks_list.h"
#include "netutils/netlib.h"
#include "netutils/dhcpc.h"
#include "power_manager.h"
#include "sal_pan_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"
#include "bt_adapter.h"

/* PAN auto-connect: ported from xiaozhi-sf32 state machine.
 * After BR/EDR bonding completes, automatically connect PAN with a
 * 3-second delay (to avoid SDP conflict, per xiaozhi experience).
 * On PAN connected, run DHCP on bt-pan. On disconnect, reconnect
 * with exponential backoff. */

#define PAN_LAST_NAP_FILE "/data/misc/bt/last_nap"
#define PAN_CONNECT_DELAY_MS 3000   /* xiaozhi: avoid SDP conflict */
#define PAN_RECONNECT_INIT_MS 5000  /* initial reconnect delay */
#define PAN_RECONNECT_MAX_MS 60000  /* max reconnect delay */
#define PAN_MAX_RECONNECT_ATTEMPTS 30
#define PAN_FIRST_CONNECT_MAX_RETRIES 3  /* xiaozhi: first PAN connect 3×3s */
#define PAN_ABNORMAL_RECONNECT_INTERVAL_MS 10000 /* xiaozhi: 10s period */
#define PAN_ABNORMAL_MAX_RECONNECT 30   /* xiaozhi: 30 attempts ≈ 5min */
#define BT_NAME_PREFIX "Agent-Watch"

typedef enum {
    PAN_AUTO_IDLE = 0,
    PAN_AUTO_WAITING_BOND,       /* waiting for bond to complete */
    PAN_AUTO_CONNECT_PENDING,    /* bond done, timer running */
    PAN_AUTO_CONNECTING,         /* PAN connect in progress */
    PAN_AUTO_CONNECTED,          /* PAN connected, DHCP running */
    PAN_AUTO_RECONNECTING,       /* PAN disconnected, retrying */
} pan_auto_state_t;

/* Disconnect classification (ported from xiaozhi-sf32):
 * - PHONE_ACTIVE: phone initiated clean disconnect → enter low power
 * - ABNORMAL: link lost / timeout → aggressive reconnect (10s×30)
 * - PAN_NEVER: PAN disconnected but was never successfully connected → 3×3s */
typedef enum {
    DISC_TYPE_NONE = 0,
    DISC_TYPE_PHONE_ACTIVE,    /* phone SCO_DISCONNECTED (clean) */
    DISC_TYPE_ABNORMAL,        /* other reasons (link loss, timeout) */
    DISC_TYPE_PAN_NEVER,       /* PAN disconnected, never was connected */
} pan_disc_type_t;

static pan_auto_state_t g_auto_state = PAN_AUTO_IDLE;
static bt_address_t g_last_nap_addr;
static bool g_has_last_nap;
static bool g_pan_ever_connected;  /* xiaozhi: first_pan_connected */
static int g_reconnect_attempts;
static int g_abnormal_reconnect_count;
static uint64_t g_last_disconnect_ms;
static service_timer_t* g_auto_connect_timer;
/* True exactly while a pan_dhcp_thread is alive, so pan_start_dhcp() can
 * trust it as an "already running" test.
 *
 * The epoch exists because a bool alone is not enough. A link drop clears
 * the flag to ask the worker to stop, but the worker is asleep between
 * retries and may not notice for a couple of seconds. If a reconnect starts
 * a fresh worker in that window, the old worker waking up would clear the
 * flag out from under the new one, and the next reconnect would then run
 * two workers at once. A worker only clears the flag if the epoch it
 * captured at startup is still current, which is exactly the condition
 * "nobody has asked me to stop, so I am still the live worker". */
static volatile bool g_dhcp_running;
static volatile uint32_t g_dhcp_epoch;
static void* g_dhcp_handle; /* for pan_stop_dhcp to close the socket */
static pan_disc_type_t g_last_disc_type;

#define PAN_MAX_CONNECTIONS 1
#define PAN_DEV_NAME "bt-pan"

/* DHCP worker tuning. 10 attempts x 2s covers the slowest phone NAP seen
 * (HyperOS can take ~12s to bring up its tethering DHCP server after the
 * BNEP link comes up); past that the link is more likely broken than slow. */
#define PAN_DHCP_MAX_ATTEMPTS      10
#define PAN_DHCP_RETRY_INTERVAL_S  2
#define PAN_DHCP_STACK_SIZE        4096

/* PAN lifecycle trace.
 *
 * Deliberately raw syslog and not BT_LOGI: CONFIG_BLUETOOTH_LOG is off in
 * the product config, which makes CONFIG_BLUETOOTH_SERVICE_LOG_LEVEL
 * undefined, which expands every BT_LOG* macro in utils/log.h to nothing.
 * All 51 BT_LOG* calls in this file therefore produce no output on the
 * board, and the PAN link would come up or fail in complete silence. The
 * SAL layer already uses raw syslog for the same reason.
 *
 * One fixed prefix so the whole lifecycle is `grep '\[pan\] state='`. */
#define PAN_STATE_LOG(level, fmt, ...) \
    syslog(level, "[pan] state=" fmt "\n", ##__VA_ARGS__)

/* Largest Ethernet frame the TAP device can hand us. Must equal
 * CONFIG_NET_ETH_PKTSIZE: netdev_register.c:318-323 sets d_pktsize to it
 * for NET_LL_ETHERNET, and tun_read() rejects (without dequeuing!) any
 * read whose buflen is smaller than the queued frame. Deriving this from
 * SIOCGIFMTU is what broke it - MTU is pktsize minus the 14-byte header,
 * so an MTU-sized buffer is always 14 bytes short. */
#define PAN_ETH_FRAME_MAX CONFIG_NET_ETH_PKTSIZE

#define PAN_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, pan_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node conn_list;
    bool enable;
    int tun_fd;
    int tun_packet_size;
    char tun_devname[16];
    int local_role;
    bt_address_t peer_addr;
    service_poll_t* poll_handle;
    pthread_mutex_t pan_lock;
    callbacks_list_t* callbacks;
} pan_global_t;

typedef struct {
    struct list_node node;
    bt_address_t addr;
    uint8_t local_role;
    uint8_t peer_role;
    uint8_t state;
} pan_conn_t;

typedef struct {
    /* role */
    pan_role_t remote_role;
    pan_role_t local_role;
    /* pan connection state */
    profile_connection_state_t state;
} pan_conn_evt_t;

typedef struct {
    uint16_t protocol;
    uint8_t* packet;
    uint16_t length;
} pan_data_evt_t;

typedef struct {
    enum {
        CONNECTION_EVT,
        DATA_IND_EVT,
    } evt_id;
    bt_address_t addr;
    union {
        pan_conn_evt_t conn_evt;
        pan_data_evt_t data_evt;
    };
} pan_msg_t;

typedef struct eth_hdr {
    uint8_t h_dest[6];
    uint8_t h_src[6];
    short h_proto;
} eth_hdr_t;

static pan_global_t g_pan = { 0 };
static uint8_t* pan_read_buf = NULL;

/* ── PAN auto-connect helpers (ported from xiaozhi-sf32) ──────── */

static uint64_t pan_auto_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000u);
}

static void pan_save_last_nap(const bt_address_t* addr)
{
    int fd = open(PAN_LAST_NAP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, addr, sizeof(bt_address_t));
        close(fd);
        BT_LOGI("Saved last NAP addr to %s", PAN_LAST_NAP_FILE);
    }
}

static bool pan_load_last_nap(bt_address_t* addr)
{
    int fd = open(PAN_LAST_NAP_FILE, O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, addr, sizeof(bt_address_t));
        close(fd);
        if (n == sizeof(bt_address_t)) {
            BT_LOGI("Loaded last NAP addr from %s", PAN_LAST_NAP_FILE);
            return true;
        }
    }
    return false;
}

static bool pan_addr_allocator(void** data, uint32_t size)
{
    *data = malloc(size);
    return *data != NULL;
}

/* Fall back to the bond list when last_nap is missing.
 *
 * last_nap is only written when a BOND_STATE_BONDED event arrives, so a watch
 * that was paired by an older firmware - or whose last_nap was wiped while
 * bt_storage.db survived - came up bonded but never auto-connected. The bond
 * list is the same information the phone side relies on, so take the most
 * recently bonded BR/EDR device, exactly like bt_cm_device_connect() does. */
static bool pan_pick_bonded_nap(bt_address_t* addr)
{
    bt_address_t* addrs = NULL;
    int num = 0;

    if (adapter_get_bonded_devices(BT_TRANSPORT_BREDR, &addrs, &num,
            pan_addr_allocator)
        != BT_STATUS_SUCCESS) {
        return false;
    }
    if (!addrs) {
        return false;
    }
    if (num > 0) {
        memcpy(addr, &addrs[num - 1], sizeof(bt_address_t));
    }
    free(addrs);
    return num > 0;
}

static void pan_auto_connect_timeout(service_timer_t* timer, void* data);

static void pan_auto_do_connect(void)
{
    if (!g_has_last_nap) {
        BT_LOGI("No last NAP address, skipping auto-connect");
        g_auto_state = PAN_AUTO_IDLE;
        return;
    }

    /* Check reconnect cooldown (xiaozhi: LCPU stale state after ACL teardown) */
    if (g_last_disconnect_ms) {
        uint64_t elapsed = pan_auto_now_ms() - g_last_disconnect_ms;
        if (elapsed < 5000) {
            BT_LOGI("Reconnect cooldown (%llu ms left), deferring",
                    (unsigned long long)(5000 - elapsed));
            /* Reschedule timer for remaining cooldown */
            if (g_auto_connect_timer) {
                service_loop_cancel_timer(g_auto_connect_timer);
                g_auto_connect_timer = NULL;
            }
            g_auto_connect_timer = service_loop_timer_no_repeating(
                5000 - elapsed, pan_auto_connect_timeout, NULL);
            return;
        }
    }

    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(&g_last_nap_addr, addr_str);
    BT_LOGI("Auto-connecting PAN to %s (attempt %d/%d)",
            addr_str, g_reconnect_attempts + 1, PAN_MAX_RECONNECT_ATTEMPTS);

    g_auto_state = PAN_AUTO_CONNECTING;
    bt_status_t ret = bt_sal_pan_connect(&g_last_nap_addr,
        1 /* dst_role=NAP */, 2 /* src_role=PANU */);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("Auto PAN connect failed: %d", (int)ret);
        /* Schedule reconnect with backoff */
        g_auto_state = PAN_AUTO_RECONNECTING;
        g_reconnect_attempts++;
        if (g_reconnect_attempts < PAN_MAX_RECONNECT_ATTEMPTS) {
            uint64_t delay = PAN_RECONNECT_INIT_MS * (1ULL << (g_reconnect_attempts < 4 ? g_reconnect_attempts : 4));
            if (delay > PAN_RECONNECT_MAX_MS) delay = PAN_RECONNECT_MAX_MS;
            BT_LOGI("Scheduling reconnect in %llu ms", (unsigned long long)delay);
            if (g_auto_connect_timer) {
                service_loop_cancel_timer(g_auto_connect_timer);
                g_auto_connect_timer = NULL;
            }
            g_auto_connect_timer = service_loop_timer_no_repeating(
                delay, pan_auto_connect_timeout, NULL);
        } else {
            BT_LOGE("Max reconnect attempts reached, giving up");
            g_auto_state = PAN_AUTO_IDLE;
        }
    }
}

static void pan_auto_connect_timeout(service_timer_t* timer, void* data)
{
    (void)timer;
    (void)data;
    pan_auto_do_connect();
}

static void pan_start_auto_connect(uint32_t delay_ms)
{
    if (g_auto_connect_timer) {
        service_loop_cancel_timer(g_auto_connect_timer);
        g_auto_connect_timer = NULL;
    }
    g_auto_state = PAN_AUTO_CONNECT_PENDING;
    g_reconnect_attempts = 0;
    g_auto_connect_timer = service_loop_timer_no_repeating(
        delay_ms, pan_auto_connect_timeout, NULL);
    if (g_auto_connect_timer) {
        BT_LOGI("Auto-connect timer started (%lu ms)", (unsigned long)delay_ms);
    } else {
        BT_LOGE("Failed to create auto-connect timer");
        g_auto_state = PAN_AUTO_IDLE;
    }
}

static void pan_dhcp_give_up_and_reconnect(void);

/* Posted to the service loop when DHCP has exhausted its retries.
 * bt_sal_pan_disconnect() and the reconnect timer both belong to the
 * service-loop thread, so the worker cannot call them directly.
 *
 * Dropping the link is deliberate rather than retrying here: the
 * PROFILE_STATE_DISCONNECTED handler already owns ifdown and the
 * three-class backoff, so routing through it leaves exactly one
 * reconnect path to reason about. A live BNEP link with no IP is the
 * worst state to sit in - it looks connected to every status check. */
static void pan_dhcp_give_up_cb(void* arg)
{
    (void)arg;

    if (g_auto_state != PAN_AUTO_CONNECTED) {
        return; /* link already went away on its own */
    }
    PAN_STATE_LOG(LOG_WARNING, "dhcp_failed dropping link to force reconnect");
    bt_sal_pan_disconnect(&g_pan.peer_addr);
}

/* DHCP worker thread: runs after PAN connects to get IP from phone NAP.
 *
 * Single exit through `out` on purpose. Every early return here used to
 * leave g_dhcp_running true, and pan_start_dhcp() refuses to start when
 * that flag is set, so one dhcpc_open() failure disabled DHCP for the
 * rest of the boot - the link would reconnect and never get an address. */
static void* pan_dhcp_thread(void* arg)
{
    const char* devname = (const char*)arg;
    uint32_t my_epoch = g_dhcp_epoch;
    struct dhcpc_state ds;
    void* handle = NULL;
    uint8_t mac[6];
    int retries = 0;
    bool got_lease = false;
    bool asked_to_stop = false;

    PAN_STATE_LOG(LOG_INFO, "dhcp_start dev=%s", devname);

    /* Get MAC address from the bt-pan interface */
    if (netlib_getmacaddr(devname, mac) != 0) {
        PAN_STATE_LOG(LOG_ERR, "dhcp_abort reason=getmacaddr dev=%s", devname);
        goto out;
    }

    handle = dhcpc_open(devname, mac, 6);
    if (!handle) {
        PAN_STATE_LOG(LOG_ERR, "dhcp_abort reason=dhcpc_open dev=%s", devname);
        goto out;
    }
    g_dhcp_handle = handle; /* so pan_stop_dhcp can close the socket */

    /* Retry DHCP request (phone NAP may take a moment) */
    while (retries < PAN_DHCP_MAX_ATTEMPTS) {
        if (g_dhcp_epoch != my_epoch) {
            asked_to_stop = true;
            break;
        }
        if (dhcpc_request(handle, &ds) == OK) {
            /* Apply the lease: set IP, netmask, default router, DNS */
            netlib_set_ipv4addr(devname, &ds.ipaddr);
            netlib_set_dripv4addr(devname, &ds.default_router);
            netlib_set_ipv4netmask(devname, &ds.netmask);

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &ds.ipaddr, ip_str, sizeof(ip_str));
            PAN_STATE_LOG(LOG_INFO, "dhcp_ok dev=%s ip=%s", devname, ip_str);

            if (ds.dnsaddr.s_addr != 0) {
                char dns_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &ds.dnsaddr, dns_str, sizeof(dns_str));
                /* Register DNS with the system resolver */
                struct sockaddr_in dns_addr;
                memset(&dns_addr, 0, sizeof(dns_addr));
                dns_addr.sin_family = AF_INET;
                dns_addr.sin_addr = ds.dnsaddr;
                dns_add_nameserver((FAR const struct sockaddr*)&dns_addr,
                    sizeof(dns_addr));
                PAN_STATE_LOG(LOG_INFO, "dhcp_dns dns=%s", dns_str);
            }

            got_lease = true;
            g_auto_state = PAN_AUTO_CONNECTED;
            break;
        }
        retries++;
        PAN_STATE_LOG(LOG_WARNING, "dhcp_retry attempt=%d/%d", retries,
            PAN_DHCP_MAX_ATTEMPTS);
        sleep(PAN_DHCP_RETRY_INTERVAL_S);
    }

out:
    if (handle && g_dhcp_epoch == my_epoch) {
        /* pan_stop_dhcp hasn't closed it yet — do it ourselves */
        dhcpc_close(handle);
    }
    g_dhcp_handle = NULL; /* either way, the handle is no longer valid */

    /* Only the still-current worker owns the flag; see g_dhcp_epoch. */
    if (g_dhcp_epoch == my_epoch) {
        g_dhcp_running = false;
    }

    /* A stop request means the link went away and the disconnect handler is
     * already driving the reconnect - giving up here as well could drop a
     * link that has since come back up. */
    if (!got_lease && !asked_to_stop) {
        PAN_STATE_LOG(LOG_ERR, "dhcp_giveup attempts=%d", retries);
        pan_dhcp_give_up_and_reconnect();
    }
    return NULL;
}

static void pan_dhcp_give_up_and_reconnect(void)
{
    do_in_service_loop(pan_dhcp_give_up_cb, NULL);
}

/* Ask a live DHCP worker to stop. Bumping the epoch is what makes this safe
 * to call from the service loop while the worker sleeps between retries:
 * the worker will not clear the flag or trigger a give-up reconnect once its
 * epoch is stale, so a fast reconnect can start a fresh worker immediately. */
static void pan_stop_dhcp(void)
{
    g_dhcp_epoch++;
    g_dhcp_running = false;
    /* Slam the DHCP socket shut to unblock recv() in the DHCP thread.
     * dhcpc_close() is NOT safe here — it frees the handle while the
     * thread still holds a local pointer to it.  Instead, reach into
     * the handle and close just the sockfd; the thread wakes up from
     * recv() with an error, sees the epoch mismatch, and calls
     * dhcpc_close() itself (the normal exit path). */
    if (g_dhcp_handle) {
        /* struct dhcpc_state_s starts with: interface, sockfd, ...
         * The sockfd field is at offset sizeof(const char*) = 4 on ARM. */
        typedef struct { const char *iface; int sockfd; } *dhcp_min_t;
        dhcp_min_t h = (dhcp_min_t)g_dhcp_handle;
        if (h->sockfd > 0) {
            close(h->sockfd);
            h->sockfd = -1;
        }
    }
}

static void pan_start_dhcp(const char* devname)
{
    pthread_attr_t attr;
    pthread_t tid;

    if (g_dhcp_running) {
        PAN_STATE_LOG(LOG_WARNING, "dhcp_skip reason=already_running");
        return;
    }
    g_dhcp_running = true;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PAN_DHCP_STACK_SIZE);
    /* Detached: nothing ever joins this thread, so a joinable one leaks
     * its TCB and stack on every PAN reconnect. devname is always a
     * string literal, so passing it across the thread boundary is safe. */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, pan_dhcp_thread, (void*)devname) == 0) {
        pthread_setname_np(tid, "pan_dhcp");
    } else {
        PAN_STATE_LOG(LOG_ERR, "dhcp_abort reason=pthread_create");
        g_dhcp_running = false;
    }
    pthread_attr_destroy(&attr);
}

/* Bond state change callback: triggers auto-connect after bonding */
static void pan_on_bond_state(void* cookie, bt_address_t* addr,
    bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    (void)cookie;
    if (transport != BT_TRANSPORT_BREDR) {
        return;
    }

    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);

    if (state == BOND_STATE_BONDED) {
        BT_LOGI("BR/EDR bonded: %s, scheduling PAN auto-connect", addr_str);
        memcpy(&g_last_nap_addr, addr, sizeof(bt_address_t));
        g_has_last_nap = true;
        pan_save_last_nap(addr);
        /* xiaozhi: 3-second delay to avoid SDP conflict */
        pan_start_auto_connect(PAN_CONNECT_DELAY_MS);
    } else if (state == BOND_STATE_NONE) {
        BT_LOGI("Bond removed: %s", addr_str);
    }
}

/* Adapter state callback: set local name with MAC suffix when BT is ON.
 * xiaozhi-sf32 does this at BT_APP_READY: bt_interface_set_local_name
 * with "小智-XX:XX:XX:XX:XX:XX". We do "Agent-Watch-XX:XX:XX:XX:XX:XX". */

static void pan_set_local_name_with_mac(void)
{
    bt_address_t addr;
    char name[48];

    adapter_get_address(&addr);
    snprintf(name, sizeof(name), "%s-%02x:%02x:%02x:%02x:%02x:%02x",
        BT_NAME_PREFIX,
        addr.addr[5], addr.addr[4], addr.addr[3],
        addr.addr[2], addr.addr[1], addr.addr[0]);

    bt_status_t ret = adapter_set_name(name);
    if (ret == BT_STATUS_SUCCESS) {
        BT_LOGI("Local name set to: %s", name);
    } else {
        BT_LOGW("Failed to set local name: %d", (int)ret);
    }
}

static void pan_on_adapter_state_changed(void* cookie, bt_adapter_state_t state)
{
    (void)cookie;
    if (state == BT_ADAPTER_STATE_ON) {
        BT_LOGI("Adapter ON, setting local name with MAC suffix");
        pan_set_local_name_with_mac();

        /* The bond and the NAP address both survive a reset (bt_storage.db
         * and last_nap under /data/misc/bt), so on a normal boot nothing
         * bonds again and pan_on_bond_state() never fires. Without this the
         * watch came up paired but with no network until someone typed
         * "pan connect" on the console. pan_service_init() has already read
         * last_nap into g_last_nap_addr; the IDLE test keeps this from
         * disturbing a connect that a bond or a reconnect already started. */
        if (g_has_last_nap && g_auto_state == PAN_AUTO_IDLE) {
            PAN_STATE_LOG(LOG_INFO, "adapter-on-auto-connect");
            pan_start_auto_connect(PAN_CONNECT_DELAY_MS);
        } else if (!g_has_last_nap && g_auto_state == PAN_AUTO_IDLE
                   && pan_pick_bonded_nap(&g_last_nap_addr)) {
            g_has_last_nap = true;
            pan_save_last_nap(&g_last_nap_addr);
            PAN_STATE_LOG(LOG_INFO, "adapter-on-auto-connect source=bondlist");
            pan_start_auto_connect(PAN_CONNECT_DELAY_MS);
        }
    }
}

/* Device identity: get bt-pan MAC address for Device-Id/chip_id.
 * Compatible with xiaozhi's get_mac_address() / get_client_id(). */
int pan_get_bt_mac_address(uint8_t* mac_out, int len)
{
    if (!mac_out || len < 6) return -1;
    bt_address_t addr;
    adapter_get_address(&addr);
    /* Convert to standard MAC byte order (big-endian) */
    for (int i = 0; i < 6; i++) {
        mac_out[i] = addr.addr[5 - i];
    }
    return 0;
}

/* Device identity: SHA256(MAC)[0:16] formatted as UUID.
 * Matches xiaozhi's get_client_id():
 *   hash = SHA256(ether_addr)[0:16]
 *   format: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
 * Used for Device-Id/chip_id HTTP headers and OTA registration. */
#include <mbedtls/sha256.h>
int pan_get_device_id(char* uuid_out, int len)
{
    if (!uuid_out || len < 37) return -1;

    bt_address_t addr;
    adapter_get_address(&addr);

    /* SHA256 hash of the MAC address (6 bytes) */
    uint8_t hash[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, addr.addr, 6);
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    /* Format first 16 bytes as UUID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
    static const char hex[] = "0123456789abcdef";
    int pos = 0;
    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            uuid_out[pos++] = '-';
        }
        uuid_out[pos++] = hex[(hash[i] >> 4) & 0x0f];
        uuid_out[pos++] = hex[hash[i] & 0x0f];
    }
    uuid_out[pos] = '\0';

    BT_LOGI("Device ID: %s", uuid_out);
    return 0;
}

static pan_conn_t* pan_find_conn(bt_address_t* addr);
static void pan_conn_close(pan_conn_t* conn);

static uint8_t pan_conns(void)
{
    return list_length(&g_pan.conn_list);
}

static void pan_free_conn(pan_conn_t* conn);

static pan_conn_t* pan_new_conn(bt_address_t* addr)
{
    pan_conn_t* conn;

    /* A stale entry for the same address (previous attempt that failed
     * before a DISCONNECTED callback reached the framework, e.g. BR
     * encryption/pairing failure) must not block reconnects: drop it
     * BEFORE the max-connections check (a stale entry may be the one
     * occupying the single slot). */
    conn = pan_find_conn(addr);
    if (conn) {
        syslog(LOG_WARNING, "[panu] new_conn: dropping stale conn for %02x:%02x:%02x:%02x:%02x:%02x\n",
            addr->addr[5], addr->addr[4], addr->addr[3], addr->addr[2], addr->addr[1], addr->addr[0]);
        pan_free_conn(conn);
    }

    if (pan_conns() == PAN_MAX_CONNECTIONS) {
        BT_LOGD("%s, PAN_MAX_CONNECTIONS", __func__);
        return NULL;
    }

    conn = malloc(sizeof(pan_conn_t));
    memcpy(&conn->addr, addr, sizeof(bt_address_t));
    list_add_tail(&g_pan.conn_list, &conn->node);

    return conn;
}

static void pan_free_conn(pan_conn_t* conn)
{
    list_delete(&conn->node);
    free(conn);
}

static pan_conn_t* pan_find_conn(bt_address_t* addr)
{
    pan_conn_t* conn;
    struct list_node* node;

    list_for_every(&g_pan.conn_list, node)
    {
        conn = (pan_conn_t*)node;
        if (!memcmp(addr, &conn->addr, sizeof(bt_address_t)))
            return conn;
    }

    return NULL;
}

static void pan_close_all_conn(void)
{
    pan_conn_t* conn;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_pan.conn_list, node, tmp)
    {
        conn = (pan_conn_t*)node;
        bt_pm_conn_close(PROFILE_PANU, &conn->addr);
        pan_conn_close(conn);
    }
}

/* NuttX SIOCSIFMTU writes d_pktsize = ifr_mtu + d_llhdrlen
 * (netdev_ioctl.c:1023-1025), and TCP_MSS/UDP_MSS are derived from
 * d_pktsize (netconfig.h:325,494). Clamping the MTU here is therefore
 * what stops the IP stack from ever generating a frame that BNEP cannot
 * carry - it is the load-bearing half of the MTU derivation chain. */
static int pan_set_tap_mtu(const char* devname, uint16_t mtu)
{
    struct ifreq ifr = { 0 };
    int sockfd, ret;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        BT_LOGE("%s socket failed %d", __func__, errno);
        return -errno;
    }
    strlcpy(ifr.ifr_name, devname, IFNAMSIZ);
    ifr.ifr_mtu = mtu;
    ret = ioctl(sockfd, SIOCSIFMTU, (unsigned long)&ifr);
    if (ret < 0) {
        ret = -errno;
        BT_LOGE("%s SIOCSIFMTU %u failed %d", __func__, mtu, errno);
    } else {
        BT_LOGI("%s MTU set to %u", devname, mtu);
    }
    close(sockfd);
    return ret;
}

static int pan_tap_bridge_open(const char* devname)
{
    struct ifreq ifr;
    bt_address_t local_addr, ethaddr;
    int errcode;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    int ret;

    g_pan.tun_fd = open("/dev/tun", O_RDWR | O_CLOEXEC);
    if (g_pan.tun_fd < 0) {
        errcode = errno;
        BT_LOGE("ERROR: Failed to open /dev/tun: %d\n", errcode);
        return -errcode;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strlcpy(ifr.ifr_name, devname, IFNAMSIZ);
    ret = ioctl(g_pan.tun_fd, TUNSETIFF, (unsigned long)&ifr);
    if (ret < 0) {
        errcode = errno;
        BT_LOGE("ERROR: ioctl TUNSETIFF failed: %d\n", errcode);
        close(g_pan.tun_fd);
        return -errcode;
    }

    memset(g_pan.tun_devname, 0, sizeof(g_pan.tun_devname));
    strncpy(g_pan.tun_devname, ifr.ifr_name, IFNAMSIZ);
    adapter_get_address(&local_addr);
    bt_addr_swap(&local_addr, &ethaddr);
    netlib_setmacaddr(ifr.ifr_name, ethaddr.addr);
    /* No ifup here: the interface MTU has to be clamped to the negotiated
     * BNEP MTU first, and that value only exists after the handshake.
     * pan_ifup_and_dhcp() does both, in order. The MAC must still be set
     * before ifup - SIOCSIFHWADDR does not take effect until then
     * (netdev_ioctl.c:1117). */

    bt_addr_ba2str(&ethaddr, addr_str);
    BT_LOGI("Created Tap device: %s, Mac address: %s", ifr.ifr_name, addr_str);

    return 0;
}

static void pan_tap_bridge_close(void)
{
    if (g_pan.tun_fd) {
        BT_LOGD("TUN device: %s Closing", g_pan.tun_devname);
        netlib_ifdown(g_pan.tun_devname);
        close(g_pan.tun_fd);
        g_pan.tun_fd = -1;
    }
}

static void pan_ifup_and_dhcp(void)
{
    uint16_t tx_mtu = bt_sal_pan_get_tx_mtu(&g_pan.peer_addr);
    uint16_t if_mtu;

    /* tx.mtu is whatever the peer agreed to in its L2CAP CONFIG_REQ:
     * 1691 from Android/HyperOS NAP, or the 672 fallback if it sent no MTU
     * option (zblue classic/l2cap_br.c:1289,1448). Either is usable - we
     * just have to tell the IP stack which one it is. 14 bytes is the
     * worst-case BNEP header (General Ethernet). */
    if (tx_mtu <= BNEP_ETH_HDR_LEN) {
        PAN_STATE_LOG(LOG_ERR, "ifup_abort reason=no_negotiated_mtu tx_mtu=%u",
            tx_mtu);
        return;
    }
    if_mtu = (uint16_t)(tx_mtu - BNEP_ETH_HDR_LEN);
    if (if_mtu > 1500) {
        if_mtu = 1500;
    }
    pan_set_tap_mtu(PAN_DEV_NAME, if_mtu);

    if (netlib_ifup(PAN_DEV_NAME) < 0) {
        PAN_STATE_LOG(LOG_ERR, "ifup_abort reason=ifup_failed dev=%s",
            PAN_DEV_NAME);
        return;
    }
    PAN_STATE_LOG(LOG_INFO, "ifup dev=%s tx_mtu=%u if_mtu=%u",
        PAN_DEV_NAME, tx_mtu, if_mtu);
    pan_start_dhcp(PAN_DEV_NAME);
}

static void pan_tap_poll_data(service_poll_t* poll, int revent, void* userdata)
{
    if (revent & POLL_READABLE) {
        int ret = read(g_pan.tun_fd, pan_read_buf, PAN_ETH_FRAME_MAX);

        if (ret < (int)sizeof(eth_hdr_t)) {
            if (ret < 0 && errno != EAGAIN && errno != EINTR) {
                BT_LOGE("%s tap read failed %d", __func__, errno);
            }
            return;
        }
        bt_pm_busy(PROFILE_PANU, &g_pan.peer_addr);
        bt_sal_pan_write_eth(&g_pan.peer_addr, pan_read_buf, (uint16_t)ret);
        bt_pm_idle(PROFILE_PANU, &g_pan.peer_addr);
        return;
    }

    if (revent & POLL_WRITABLE) {
        return;
    }

    BT_LOGE("%s poll disconnected", __func__);
    pan_stop_dhcp(); /* must stop DHCP before tearing down connections */
    pan_close_all_conn();
}

static pan_conn_t* pan_new_conn_open(bt_address_t* addr, uint8_t local, uint8_t remote)
{
    pan_conn_t* conn;
    int ret;

    memcpy(&g_pan.peer_addr, addr, sizeof(bt_address_t));
    conn = pan_find_conn(addr);
    if (!conn) {
        conn = pan_new_conn(addr);
        if (!conn)
            goto open_fail;
    }

    conn->local_role = local;
    conn->peer_role = remote;
    conn->state = PROFILE_STATE_CONNECTED;
    if (g_pan.tun_fd < 0) {
        ret = pan_tap_bridge_open(PAN_DEV_NAME);
        if (ret < 0)
            goto open_fail;

        g_pan.tun_packet_size = PAN_ETH_FRAME_MAX;
        pan_read_buf = malloc(PAN_ETH_FRAME_MAX);
        if (pan_read_buf == NULL) {
            BT_LOGE("%s packet malloc failed", __func__);
            goto open_fail;
        }

        g_pan.poll_handle = service_loop_poll_fd(g_pan.tun_fd,
            POLL_DISCONNECT | POLL_READABLE,
            pan_tap_poll_data, NULL);
        if (!g_pan.poll_handle)
            goto open_fail;

        PAN_CALLBACK_FOREACH(g_pan.callbacks, netif_state_cb, PAN_STATE_ENABLED, g_pan.local_role, g_pan.tun_devname);
    }

    return conn;

open_fail:
    if (conn)
        pan_conn_close(conn);
    else
        bt_sal_pan_disconnect(addr);
    return NULL;
}

static void pan_conn_close(pan_conn_t* conn)
{
    if (conn == NULL)
        return;

    if (conn->state == PROFILE_STATE_CONNECTED)
        bt_sal_pan_disconnect(&conn->addr);

    pan_free_conn(conn);
    if (pan_conns() == 0) {
        pan_stop_dhcp(); /* stop DHCP before tearing down TAP/poll */
        if (g_pan.poll_handle) {
            service_loop_remove_poll(g_pan.poll_handle);
            g_pan.poll_handle = NULL;
        }

        if (pan_read_buf) {
            free(pan_read_buf);
            pan_read_buf = NULL;
        }

        if (g_pan.tun_fd) {
            pan_tap_bridge_close();
            PAN_CALLBACK_FOREACH(g_pan.callbacks, netif_state_cb, PAN_STATE_DISABLED, g_pan.local_role, g_pan.tun_devname);
        }
    }
}

/* Fixed, greppable name for each profile_connection_state_t value. The whole
 * PAN lifecycle is traceable with a single `grep '\[pan\] state='` because the
 * DHCP worker logs in the same form. */
static const char* pan_state_name(profile_connection_state_t state)
{
    switch (state) {
    case PROFILE_STATE_DISCONNECTED:
        return "disconnected";
    case PROFILE_STATE_CONNECTING:
        return "connecting";
    case PROFILE_STATE_CONNECTED:
        return "connected";
    case PROFILE_STATE_DISCONNECTING:
        return "disconnecting";
    default:
        return "unknown";
    }
}

static void on_pan_connection_state_changed(bt_address_t* addr, pan_conn_evt_t* evt)
{
    pan_conn_t* conn;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PAN_STATE_LOG(LOG_INFO, "%s addr=%s remote_role=%d local_role=%d",
        pan_state_name(evt->state), addr_str,
        (int)evt->remote_role, (int)evt->local_role);

    switch (evt->state) {
    case PROFILE_STATE_DISCONNECTED: {
        conn = pan_find_conn(addr);
        pan_conn_close(conn);
        bt_pm_conn_close(PROFILE_PANU, addr);
        pan_stop_dhcp();
        /* pan_tap_bridge_close() only runs on PAN disable, so a plain
         * link drop would leave the interface up with the previous
         * negotiated MTU still applied to the next handshake. */
        netlib_ifdown(PAN_DEV_NAME);

        /* Three-class disconnect logic (ported from xiaozhi-sf32):
         * 1. PAN never connected: 3×3s retry, then prompt user
         * 2. Abnormal disconnect: 10s period × 30 retries
         * 3. Phone active disconnect: enter low-power wait */
        if (g_auto_state == PAN_AUTO_CONNECTED || g_auto_state == PAN_AUTO_CONNECTING) {
            g_last_disconnect_ms = pan_auto_now_ms();

            if (!g_pan_ever_connected) {
                /* Class 1: PAN disconnected but was never successfully connected.
                 * xiaozhi: 3×3s retry, then "请确保手机开了网络共享" */
                g_last_disc_type = DISC_TYPE_PAN_NEVER;
                g_reconnect_attempts++;
                if (g_reconnect_attempts <= PAN_FIRST_CONNECT_MAX_RETRIES) {
                    BT_LOGI("PAN never connected, retry %d/%d in 3s",
                            g_reconnect_attempts, PAN_FIRST_CONNECT_MAX_RETRIES);
                    g_auto_state = PAN_AUTO_RECONNECTING;
                    pan_start_auto_connect(PAN_CONNECT_DELAY_MS);
                } else {
                    BT_LOGW("PAN first-connect retries exhausted, "
                            "ensure phone has Bluetooth tethering enabled");
                    g_auto_state = PAN_AUTO_IDLE;
                }
            } else {
                /* Class 2 or 3: PAN was previously connected.
                 * For now, treat all post-connect disconnects as abnormal
                 * (Class 2). Class 3 (phone active) requires ACL disconnect
                 * reason from the SAL layer which is not yet plumbed here. */
                g_last_disc_type = DISC_TYPE_ABNORMAL;
                g_abnormal_reconnect_count++;
                if (g_abnormal_reconnect_count <= PAN_ABNORMAL_MAX_RECONNECT) {
                    BT_LOGI("Abnormal PAN disconnect, reconnect %d/%d in %ds",
                            g_abnormal_reconnect_count, PAN_ABNORMAL_MAX_RECONNECT,
                            PAN_ABNORMAL_RECONNECT_INTERVAL_MS / 1000);
                    g_auto_state = PAN_AUTO_RECONNECTING;
                    pan_start_auto_connect(PAN_ABNORMAL_RECONNECT_INTERVAL_MS);
                } else {
                    BT_LOGE("Abnormal reconnect limit reached (%d), giving up",
                            PAN_ABNORMAL_MAX_RECONNECT);
                    g_auto_state = PAN_AUTO_IDLE;
                }
            }
        }
        break;
    }
    case PROFILE_STATE_CONNECTED:
        bt_pm_conn_open(PROFILE_PANU, addr);
        conn = pan_new_conn_open(addr, evt->local_role, evt->remote_role);
        if (conn) {
            g_auto_state = PAN_AUTO_CONNECTED;
            g_pan_ever_connected = true;  /* xiaozhi: first_pan_connected = TRUE */
            g_reconnect_attempts = 0;
            g_abnormal_reconnect_count = 0;
            g_last_disc_type = DISC_TYPE_NONE;
            /* Remember the NAP we actually reached, not just the one we bonded
             * with: a console "pan connect" or the bond-list fallback must also
             * make the next boot auto-connect. */
            if (!g_has_last_nap
                || memcmp(&g_last_nap_addr, addr, sizeof(bt_address_t)) != 0) {
                memcpy(&g_last_nap_addr, addr, sizeof(bt_address_t));
                g_has_last_nap = true;
                pan_save_last_nap(addr);
            }
            pan_ifup_and_dhcp();
        }
        break;
    case PROFILE_STATE_CONNECTING:
    case PROFILE_STATE_DISCONNECTING:
    default:
        break;
    }

    PAN_CALLBACK_FOREACH(g_pan.callbacks, connection_state_cb, evt->state, addr, evt->local_role, evt->remote_role);
}

static int on_pan_data_incoming(bt_address_t* addr, uint16_t protocol,
    uint8_t* packet, uint16_t length)
{
    if (g_pan.tun_fd > 0) {
        /* Send data to network interface */
        ssize_t ret;
        do {
            ret = write(g_pan.tun_fd, packet, length);
        } while (ret == -1 && errno == EINTR);

        /* Only DHCP/ARP and failures are logged: everything else on a tethered
         * link is mDNS/ND chatter that would drown the console. */
        if (ret != (ssize_t)length) {
            PAN_STATE_LOG(LOG_ERR, "rx_tun_write_fail len=%u ret=%d errno=%d",
                length, (int)ret, errno);
        } else if (protocol == 0x0806) {
            PAN_STATE_LOG(LOG_INFO, "rx_arp len=%u", length);
        } else if (protocol == 0x0800 && length >= 42
                   && packet[23] == 17
                   && (packet[35] == 0x43 || packet[37] == 0x44)) {
            PAN_STATE_LOG(LOG_INFO, "rx_dhcp len=%u sport=%u dport=%u",
                length, (packet[34] << 8) | packet[35],
                (packet[36] << 8) | packet[37]);
        }

        return (int)ret;
    }

    PAN_STATE_LOG(LOG_ERR, "rx_drop reason=no_tun len=%u", length);
    return -1;
}

static void pan_service_event_process(void* data)
{
    pan_msg_t* msg = data;

    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        return;
    }

    switch (msg->evt_id) {
    case CONNECTION_EVT:
        on_pan_connection_state_changed(&msg->addr, &msg->conn_evt);
        break;
    case DATA_IND_EVT: {
        pan_data_evt_t* evt = &msg->data_evt;

        bt_pm_busy(PROFILE_PANU, &msg->addr);
        on_pan_data_incoming(&msg->addr, evt->protocol,
            evt->packet, evt->length);
        bt_pm_idle(PROFILE_PANU, &msg->addr);
        free(evt->packet);
        break;
    }
    default:
        break;
    }
    pthread_mutex_unlock(&g_pan.pan_lock);

    free(data);
}

void pan_on_connection_state_changed(bt_address_t* addr, pan_role_t remote_role,
    pan_role_t local_role, profile_connection_state_t state)
{
    pan_msg_t* pan_msg = (pan_msg_t*)malloc(sizeof(pan_msg_t));
    if (pan_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    pan_msg->evt_id = CONNECTION_EVT;
    pan_msg->conn_evt.state = state;
    pan_msg->conn_evt.remote_role = remote_role;
    pan_msg->conn_evt.local_role = local_role;
    memcpy(&pan_msg->addr, addr, sizeof(bt_address_t));

    do_in_service_loop(pan_service_event_process, pan_msg);
}

void pan_on_eth_received(bt_address_t* addr, const uint8_t* eth_frame,
    uint16_t eth_len)
{
    pan_msg_t* pan_msg;
    uint8_t* packet;

    if (!eth_frame || eth_len < sizeof(eth_hdr_t)
        || eth_len > PAN_ETH_FRAME_MAX) {
        BT_LOGE("%s bad frame len %u", __func__, eth_len);
        return;
    }

    pan_msg = (pan_msg_t*)malloc(sizeof(pan_msg_t));
    if (pan_msg == NULL) {
        BT_LOGE("%s msg malloc failed", __func__);
        return;
    }
    packet = malloc(eth_len);
    if (packet == NULL) {
        free(pan_msg);
        BT_LOGE("%s packet malloc failed", __func__);
        return;
    }
    memcpy(packet, eth_frame, eth_len);

    pan_msg->evt_id = DATA_IND_EVT;
    memcpy(&pan_msg->addr, addr, sizeof(bt_address_t));
    pan_msg->data_evt.protocol = (uint16_t)((eth_frame[12] << 8)
                                            | eth_frame[13]);
    pan_msg->data_evt.length = eth_len;
    pan_msg->data_evt.packet = packet;

    do_in_service_loop(pan_service_event_process, pan_msg);
}

static bt_status_t pan_init(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_pan.pan_lock, &attr) < 0)
        return BT_STATUS_FAIL;

    g_pan.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    return BT_STATUS_SUCCESS;
}

static void pan_cleanup(void)
{
    bt_callbacks_list_free(g_pan.callbacks);
    g_pan.callbacks = NULL;
    pthread_mutex_destroy(&g_pan.pan_lock);
}

static bt_status_t pan_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_pan.pan_lock);
    if (g_pan.enable) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        cb(PROFILE_PANU, true);
        return BT_STATUS_NOT_ENABLED;
    }

    g_pan.tun_fd = -1;
    g_pan.local_role = PAN_ROLE_PANU;
    list_initialize(&g_pan.conn_list);
    if (bt_sal_pan_init(PAN_MAX_CONNECTIONS, PAN_ROLE_PANU) != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        list_delete(&g_pan.conn_list);
        cb(PROFILE_PANU, false);
        return BT_STATUS_FAIL;
    }

    /* Auto-connect: load last NAP address for reconnection */
    g_has_last_nap = pan_load_last_nap(&g_last_nap_addr);
    g_auto_state = PAN_AUTO_IDLE;
    g_pan_ever_connected = false;
    g_abnormal_reconnect_count = 0;
    g_last_disc_type = DISC_TYPE_NONE;
    /* Retires any worker left over from a previous enable/disable cycle
     * rather than just clearing the flag under it. */
    pan_stop_dhcp();

    /* Register adapter + bond state callbacks for auto-connect */
    {
        static adapter_callbacks_t s_pan_adapter_cbs = { 0 };
        static void* s_pan_cb_cookie = NULL;
        s_pan_adapter_cbs.on_bond_state_changed = pan_on_bond_state;
        s_pan_adapter_cbs.on_adapter_state_changed = pan_on_adapter_state_changed;
        if (s_pan_cb_cookie) {
            adapter_unregister_callback(NULL, &s_pan_cb_cookie);
        }
        s_pan_cb_cookie = adapter_register_callback(NULL, &s_pan_adapter_cbs);
        BT_LOGI("Adapter+bond callbacks registered for PAN auto-connect");

        /* If adapter is already ON, set local name now */
        pan_set_local_name_with_mac();
    }

    g_pan.enable = true;
    pthread_mutex_unlock(&g_pan.pan_lock);
    cb(PROFILE_PANU, true);

    return BT_STATUS_SUCCESS;
}

static bt_status_t pan_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        cb(PROFILE_PANU, true);
        return BT_STATUS_SUCCESS;
    }

    g_pan.enable = false;
    pan_stop_dhcp();
    g_auto_state = PAN_AUTO_IDLE;
    if (g_auto_connect_timer) {
        service_loop_cancel_timer(g_auto_connect_timer);
        g_auto_connect_timer = NULL;
    }
    pan_close_all_conn();
    list_delete(&g_pan.conn_list);
    pthread_mutex_unlock(&g_pan.pan_lock);
    bt_sal_pan_cleanup();
    cb(PROFILE_PANU, true);

    return BT_STATUS_SUCCESS;
}

static int pan_get_state(void)
{
    return 1;
}

static void* pan_register_callbacks(void* remote, const pan_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_pan.callbacks, remote, (void*)callbacks);
}

static bool pan_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_pan.callbacks, remote, cookie);
}

static bt_status_t pan_connect(bt_address_t* addr, uint8_t dst_role, uint8_t src_role)
{
    pan_conn_t* conn;
    bt_status_t status;

    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        syslog(LOG_ERR, "[panu] connect: not enabled\n");
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    conn = pan_new_conn(addr);
    if (!conn) {
        syslog(LOG_ERR, "[panu] connect: no resources\n");
        status = BT_STATUS_NO_RESOURCES;
        goto exit;
    }

    status = bt_sal_pan_connect(addr, dst_role, src_role);
    if (status != BT_STATUS_SUCCESS) {
        syslog(LOG_ERR, "[panu] connect: sal failed %d\n", (int)status);
        pan_free_conn(conn);
        goto exit;
    }

    conn->state = PROFILE_STATE_CONNECTING;

exit:
    pthread_mutex_unlock(&g_pan.pan_lock);
    return status;
}

static bt_status_t pan_disconnect(bt_address_t* addr)
{
    pan_conn_t* conn;
    bt_status_t status;

    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    conn = pan_find_conn(addr);
    if (!conn) {
        status = BT_STATUS_DEVICE_NOT_FOUND;
        goto exit;
    }

    status = bt_sal_pan_disconnect(addr);
    if (status != BT_STATUS_SUCCESS)
        goto exit;

    conn->state = PROFILE_STATE_DISCONNECTING;

exit:
    pthread_mutex_unlock(&g_pan.pan_lock);
    return status;
}

static const pan_interface_t panInterface = {
    .size = sizeof(panInterface),
    .register_callbacks = pan_register_callbacks,
    .unregister_callbacks = pan_unregister_callbacks,
    .connect = pan_connect,
    .disconnect = pan_disconnect,
};

static const void* get_pan_profile_interface(void)
{
    return (void*)&panInterface;
}

static int pan_dump(void)
{
    BT_LOGD("%s", __func__);

    return 0;
}

static const profile_service_t pan_service = {
    .auto_start = true,
    .name = PROFILE_PANU_NAME,
    .id = PROFILE_PANU,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = BT_UUID_DECLARE_16(BT_UUID_PANU),
    .init = pan_init,
    .startup = pan_startup,
    .shutdown = pan_shutdown,
    .process_msg = NULL,
    .get_state = pan_get_state,
    .get_profile_interface = get_pan_profile_interface,
    .cleanup = pan_cleanup,
    .dump = pan_dump,
};

void register_pan_service(void)
{
    register_service(&pan_service);
}
