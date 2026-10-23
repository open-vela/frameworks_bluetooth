/* bnep_netdev.h -- BNEP <-> NuttX IP stack bridge. */
#ifndef BNEP_NETDEV_H
#define BNEP_NETDEV_H

#include <stdint.h>
#include <stddef.h>

/* IP stack -> BNEP: one Ethernet frame, split form */
typedef void (*bnep_tx_cb_t)(uint16_t proto, const uint8_t dst[6],
                             const uint8_t src[6], const uint8_t *payload,
                             size_t len, void *arg);

int  bnep_netdev_init(const char *ifname, const uint8_t mac[6]);
int  bnep_netdev_up(bnep_tx_cb_t tx, void *arg);
void bnep_netdev_down(void);

/* BNEP -> IP stack (called from the zblue bt thread) */
int  bnep_netdev_rx(const uint8_t dst[6], const uint8_t src[6], uint16_t proto,
                    const uint8_t *payload, size_t len);

int  bnep_netdev_dhcp_start(void);
void bnep_netdev_dhcp_stop(void);

#endif /* BNEP_NETDEV_H */
