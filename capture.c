#include <pcap.h>
#include <arpa/inet.h>
#include "capture.h"

struct _cap_ctx {
    char ebuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
} static ctx;

msg_t cap_init(const char *dev) {
    int ret = 0;
    if ((ret = pcap_init(0, ctx.ebuf)) != 0) {
        return RET_ERR(ret, ctx.ebuf);
    }
    if ((ctx.handle = pcap_create(dev, ctx.ebuf)) == NULL) {
        return RET_ERR(ret, ctx.ebuf);
    }

	bpf_u_int32 mask;
	bpf_u_int32 net; 
	if (pcap_lookupnet(dev, &net, &mask, ctx.ebuf) == -1) {
        return RET_ERR(-1, ctx.ebuf);
    }

	pcap_set_snaplen(ctx.handle, 65535);
	pcap_set_promisc(ctx.handle, 1);
	pcap_set_immediate_mode(ctx.handle, 1);
    pcap_setnonblock(ctx.handle, 1, ctx.ebuf);
	if (pcap_activate(ctx.handle) != 0) {
        return RET_ERR(-1, ctx.ebuf);
    }

	struct bpf_program bpf;
	ret = pcap_compile(ctx.handle, &bpf, "udp", 0, net);
    if (ret != 0) {
        return RET_ERR(-1, pcap_geterr(ctx.handle));
    }
	pcap_setfilter(ctx.handle, &bpf);
    return RET_OK();
}

msg_t cap_next(pkt_t *pkt, mavlink_message_t *msg) {
	struct pcap_pkthdr *header = NULL;
    const u8_t *packet = NULL;

    int ret = pcap_next_ex(ctx.handle, &header, &packet);
    if (ret != 1) {
        return RET_ERR(-1, "null");
    }

    const pkt_t *_pkt = (const pkt_t*)packet;
    mavlink_status_t status = {0};
    int res = 0;

    if (_pkt->payload[0] != 0xFE && 
            _pkt->payload[0] != 0xFD) 
        return RET_ERR(-1, "not an msg");

    for (size_t i = 0; i < htons(_pkt->udp.uh_ulen)-8; i++) {
        res = mavlink_parse_char(0, _pkt->payload[i], 
                msg, &status);
		if (res != MAVLINK_FRAMING_INCOMPLETE) {
			break;
		}
    }
    if (res == MAVLINK_FRAMING_OK) {
        memcpy(pkt, packet, sizeof(eth_t)+sizeof(ip_t)+sizeof(udp_t));

        pkt->udp.source = htons(pkt->udp.source);
        pkt->udp.dest = htons(pkt->udp.dest);
        pkt->udp.uh_ulen = htons(pkt->udp.uh_ulen)-8;
        return RET_OK();
    } 
    return RET_ERR(-1, "Invalid message");
}

void cap_deinit(void) {
    pcap_close(ctx.handle);
}

pkt_list_t *pkt_push(pkt_list_t *p, pkt_t *pkt,
        mavlink_message_t *msg) {
    p->next = malloc(sizeof(pkt_list_t));
    if (p->next) {
        memcpy(&p->next->pkt, pkt, sizeof(pkt_t));
        memcpy(&p->next->msg, msg, sizeof(mavlink_message_t)); 
    } 
    p->next->next = NULL;
    return p->next;
}

int cap_dev_next(char **dev) {
    static char **_dev = NULL;
    static pcap_if_t *head, *ptr;
    if (dev) {
        _dev = dev; 
        pcap_findalldevs(&head, ctx.ebuf);
        ptr = head;
        return 0;
    } else {
        if (ptr) {
            *_dev = ptr->name;
            ptr = ptr->next;
            return 0;
        } 
        return 1;
    }
}
