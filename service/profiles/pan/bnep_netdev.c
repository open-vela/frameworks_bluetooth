/* bnep_netdev.c -- BNEP <-> NuttX IP stack bridge + DHCP.
 *
 * Minimal classic netdev (CONFIG_NET_TUN is not enabled in the current
 * config).  IPv4 single address: d_ipaddr / d_draddr / d_netmask.
 */
#include <nuttx/config.h>

#include <nuttx/net/netdev.h>
#include <nuttx/net/net.h>
#include <nuttx/net/ip.h>
#include <nuttx/net/ethernet.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "netutils/netlib.h"
#include "netutils/dhcpc.h"
#include "utils/log.h"
#include "bnep_netdev.h"

#define LOG_TAG "bnep"

#define BTPAN_IFNAME   "pan0"
#define BTPAN_ETH_HDR  14
/* MTU bounded by CONFIG_NET_ETH_PKTSIZE; see config notes. */
#define BTPAN_MTU      (CONFIG_NET_ETH_PKTSIZE - BTPAN_ETH_HDR)

static struct net_driver_s g_dev;
static uint8_t      g_mac[6];
static char         g_ifname[IFNAMSIZ] = BTPAN_IFNAME;
static bool         g_registered;
static bnep_tx_cb_t g_tx;
static void        *g_tx_arg;
static uint32_t     g_tx_frames, g_rx_frames, g_rx_drops;

static void *g_dhcpc;
static bool  g_dhcp_busy;

/* ---------------- netdev callbacks ---------------- */

static void bnep_send_frame(struct net_driver_s *dev)
{
    uint8_t  *p   = (uint8_t *)dev->d_buf;
    uint16_t  len = dev->d_len;

    if (g_tx == NULL || p == NULL || len < BTPAN_ETH_HDR) return;

    g_tx((uint16_t)(((uint16_t)p[12] << 8) | p[13]),
         p, p + 6, p + BTPAN_ETH_HDR, len - BTPAN_ETH_HDR, g_tx_arg);
    g_tx_frames++;
#ifdef NETDEV_TXPACKETS
    NETDEV_TXPACKETS(dev);
#endif
}

static int bnep_txpoll(struct net_driver_s *dev)
{
    bnep_send_frame(dev);
    return OK;
}

static int bnep_ifup(struct net_driver_s *dev)
{
    BT_LOGI("[pan] netdev up dev=%s mtu=%d", g_ifname, BTPAN_MTU);
    return OK;
}

static int bnep_ifdown(struct net_driver_s *dev)
{
    BT_LOGI("[pan] netdev down dev=%s", g_ifname);
    return OK;
}

static int bnep_transmit(struct net_driver_s *dev)
{
    bnep_send_frame(dev);
    return OK;
}

static int bnep_txavail(struct net_driver_s *dev)
{
    if ((dev->d_flags & IFF_UP) != 0) {
        net_lock();
        devif_poll(dev, bnep_txpoll);
        net_unlock();
    }
    return OK;
}

#ifdef CONFIG_NETDEV_IOCTL
static int bnep_ioctl(struct net_driver_s *dev, int cmd, unsigned long arg)
{
    return -ENOTTY;
}
#endif

#ifdef CONFIG_NETDEV_STATISTICS
static struct netdev_statistics_s g_stats;
#endif

/* ---------------- lifecycle ---------------- */

/* Old-style NuttX netdev: stack pulls packets via devif_poll() */
static int bnep_txpoll_cb(struct net_driver_s *dev)
{
    if (dev->d_len > 0) {
        bnep_transmit(dev);
    }
    return 0;
}


int bnep_netdev_init(const char *ifname, const uint8_t mac[6])
{
    if (g_registered) return 0;

    if (ifname) strlcpy(g_ifname, ifname, sizeof(g_ifname));
    if (mac)    memcpy(g_mac, mac, 6);

    memset(&g_dev, 0, sizeof(g_dev));
    g_dev.d_ifup     = bnep_ifup;
    g_dev.d_ifdown   = bnep_ifdown;
    g_dev.d_txavail  = bnep_txavail;
#ifdef CONFIG_NETDEV_IOCTL
    g_dev.d_ioctl    = bnep_ioctl;
#endif
#ifdef CONFIG_NETDEV_STATISTICS
    g_dev.d_statistics = &g_stats;
#endif
    g_dev.d_buf      = NULL;                 /* allocated by the stack */

    if (netdev_register(&g_dev, NET_LL_ETHERNET) < 0) {
        BT_LOGE("[pan] netdev_register failed errno=%d", errno);
        return -errno;
    }
    g_registered = true;

    netlib_setmacaddr(g_ifname, g_mac);
    BT_LOGI("[pan] netdev registered dev=%s mtu=%d (CONFIG_NET_ETH_PKTSIZE=%d)",
            g_ifname, BTPAN_MTU, CONFIG_NET_ETH_PKTSIZE);
    return 0;
}

int bnep_netdev_up(bnep_tx_cb_t tx, void *arg)
{
    static const uint8_t default_mac[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };

    if (!g_registered) {
        if (bnep_netdev_init(BTPAN_IFNAME, default_mac) < 0) return -ENODEV;
    }
    g_tx     = tx;
    g_tx_arg = arg;
    return netlib_ifup(g_ifname);
}

void bnep_netdev_down(void)
{
    bnep_netdev_dhcp_stop();
    if (g_registered) netlib_ifdown(g_ifname);
    g_tx = NULL; g_tx_arg = NULL;
}

int bnep_netdev_rx(const uint8_t dst[6], const uint8_t src[6], uint16_t proto,
                   const uint8_t *payload, size_t len)
{
    uint8_t *p;

    if (len + BTPAN_ETH_HDR > CONFIG_NET_ETH_PKTSIZE) {
        g_rx_drops++;
        BT_LOGW("[pan] rx: frame %u > %d, dropped (raise CONFIG_NET_ETH_PKTSIZE)",
                (unsigned)(len + BTPAN_ETH_HDR), CONFIG_NET_ETH_PKTSIZE);
        return -EMSGSIZE;
    }

    net_lock();
    p = (uint8_t *)g_dev.d_buf;
    if (p == NULL || (g_dev.d_flags & IFF_UP) == 0) {
        net_unlock();
        return -ENETDOWN;
    }
    memcpy(p, dst, 6);
    memcpy(p + 6, src, 6);
    p[12] = (uint8_t)(proto >> 8);
    p[13] = (uint8_t)proto;
    if (len) memcpy(p + BTPAN_ETH_HDR, payload, len);
    g_dev.d_len = (uint16_t)(BTPAN_ETH_HDR + len);

#ifdef NETDEV_RXPACKETS
    NETDEV_RXPACKETS(&g_dev);
#endif

    /* VERIFY-RX: pick the one your NuttX has:
     *   ipv4_input(&g_dev);   <-- default below
     *   ipv4_input(&g_dev);   <-- netdev lower-half builds
     *   ipv4_input(&g_dev);             <-- some legacy drivers
     * confirm: grep -rn "rxready\|ipv4_input" nuttx/drivers/net nuttx/net | head
     */
    ipv4_input(&g_dev);
    net_unlock();

    g_rx_frames++;
    return 0;
}

/* ---------------- DHCP ---------------- */

static void bnep_dhcp_done(struct dhcpc_state *st)
{
    struct in_addr ip, mask, gw;
    char buf[INET_ADDRSTRLEN];

    g_dhcp_busy = false;
    if (st == NULL) {
        BT_LOGE("[pan] dhcp failed on %s -> will retry on reconnect", g_ifname);
        return;
    }

    /* VERIFY-DHCP: field names of struct dhcpc_state in <netutils/dhcpc.h> */
    ip   = st->ipaddr;
    mask = st->netmask;
    gw   = st->default_router;

    netlib_set_ipv4addr(g_ifname, &ip);
    netlib_set_ipv4netmask(g_ifname, &mask);
    netlib_set_dripv4addr(g_ifname, &gw);

    inet_ntop(AF_INET, &ip, buf, sizeof(buf));
    BT_LOGI("[pan] state=dhcp_ok dev=%s ip=%s", g_ifname, buf);
    inet_ntop(AF_INET, &gw, buf, sizeof(buf));
    BT_LOGI("[pan] gateway=%s netmask=%08x tx=%u rx=%u drops=%u",
            buf, (unsigned)mask.s_addr, g_tx_frames, g_rx_frames, g_rx_drops);
}

int bnep_netdev_dhcp_start(void)
{
    if (g_dhcpc == NULL) {
        g_dhcpc = dhcpc_open(g_ifname, g_mac, 6);
        if (g_dhcpc == NULL) {
            BT_LOGE("[pan] dhcpc_open(%s) failed errno=%d", g_ifname, errno);
            return -errno;
        }
    }
    if (g_dhcp_busy) return -EBUSY;
    g_dhcp_busy = true;

    BT_LOGI("[pan] dhcp starting on %s", g_ifname);
    if (dhcpc_request_async(g_dhcpc, bnep_dhcp_done) != OK) {
        g_dhcp_busy = false;
        BT_LOGE("[pan] dhcpc_request_async failed errno=%d", errno);
        return -errno;
    }
    return 0;
}

void bnep_netdev_dhcp_stop(void)
{
    if (g_dhcpc) {
        dhcpc_close(g_dhcpc);
        g_dhcpc = NULL;
    }
    g_dhcp_busy = false;
}
