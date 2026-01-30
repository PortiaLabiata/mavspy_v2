#ifndef CAPTURE_H
#define CAPTURE_H

#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/if_ether.h>

#include "mavlink/all/mavlink.h"
#include "common.h"
#define MAVLINK_MAX_MSG_LEN 280

typedef struct ethhdr eth_t;
typedef struct iphdr ip_t;
typedef struct udphdr udp_t;

typedef struct {
    eth_t eth;
    ip_t ip;
    udp_t udp;
    u8_t payload[];
} __attribute__((packed)) pkt_t;

struct _pkt_list_t {
    pkt_t pkt;
    mavlink_message_t msg;
    struct _pkt_list_t *next;
};
typedef struct _pkt_list_t pkt_list_t;

pkt_list_t *pkt_push(pkt_list_t *p, pkt_t *pkt, mavlink_message_t *msg);

msg_t cap_init(const char *dev);
void cap_deinit(void);
msg_t cap_next(pkt_t *pkt, mavlink_message_t *msg);
int cap_dev_next(char **dev);

#endif
