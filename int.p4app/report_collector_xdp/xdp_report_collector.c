#define KBUILD_MODNAME "xdp_report_collector"
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#define ETHERTYPE_IP 0x0800
#define INT_DST_PORT _INT_DST_PORT
#define MAX_INT_HOP _MAX_INT_HOP

#define FLOW_LATENCY_THRESHOLD _FLOW_LATENCY_THRESHOLD 
#define HOP_LATENCY_THRESHOLD _HOP_LATENCY_THRESHOLD
#define LINK_LATENCY_THRESHOLD _LINK_LATENCY_THRESHOLD
#define QUEUE_OCCUPANCY_THRESHOLD _QUEUE_OCCUPANCY_THRESHOLD

#define ETH_SIZE 14
#define TCPHDR_SIZE 20
#define UDPHDR_SIZE 8
#define INT_SHIM_SIZE 4

#define CURSOR_ADVANCE(_target, _cursor, _len,_data_end) \
    ({  _target = _cursor; _cursor += _len; \
        if(unlikely(_cursor > _data_end)) return XDP_DROP; })

#define CURSOR_ADVANCE_NO_PARSE(_cursor, _len, _data_end) \
	({  _cursor += _len; \
  		if(unlikely(_cursor > _data_end)) return XDP_DROP; })

#define ABS(a,b) ((a>b)? (a-b):(b-a))


struct ports_t {
    u16 source;
    u16 dest;
} __attribute__((packed));

struct eth_tp {
    u64 dst:48;
    u64 src:48;
    u16 type;
} __attribute__((packed));

struct telemetry_report_t {
    u8  ver:4,
        length:4;
    u16 nprot:3,
        rep_md_bits:6,
        rsvd:6,
        d:1;
    u8  q:1,
        f:1,
        hw_id:6;
    u32 switch_id;
    u32 seq_num;
    u32 ingress_tstamp;
} __attribute__((packed));

struct INT_shim_t {
    u8  type;
    u8  rsvd1;
    u8  length;
    u8  dscp:6, 
        rsvd2:2;
} __attribute__((packed));

struct INT_metadata_hdr_t {
    u8  ver:4,
        rep:2,
        c:1,
        e:1;
    u16 m:1,
        rsvd1:10,
        hop_ml:5;
    u8  rem_hop_cnt;
    u16 ins_map;
    u16 rsvd2;
} __attribute__((packed));

struct INT_metadata_t {
    u32 data;
} __attribute__((packed));

struct flow_id_t {
    u32 src_ip;
    u32 dst_ip;
    u16 src_port;
    u16 dst_port;
    u16 ip_proto;
};

struct flow_info_t {
    // flow id
    u32 src_ip;
    u32 dst_ip;
    u16 src_port;
    u16 dst_port;
    u16 ip_proto;
    
    u8 hop_cnt;
    // flow info
    u32 flow_latency;
    u32 switch_ids[MAX_INT_HOP];
    // events
    u8 e_new_flow;
    u8 e_flow_latency;
    u8 e_sw_latency;
    u8 e_link_latency;
    u8 e_q_occupancy;
};

struct switch_id_t {
    u32 switch_id;
};

struct switch_info_t {
    u32 hop_latency;
};

struct link_id_t {
    u32 egress_switch_id;
    u32 egress_port_id;
    u32 ingress_switch_id;
    u32 ingress_port_id;
};

struct link_info_t {
    u32 link_latency;
    //u32 egress_port_tx_util;
};

struct queue_id_t {
    u32 switch_id;
    u32 q_id;
};

struct queue_info_t {
    u32 q_occupancy;
};

struct INT_hop_metadata_t {
    u32 switch_id;
    u16 ingress_port_id;
    u16 egress_port_id;
    u32 hop_latency;
    u8  q_id;
    u32 q_occupancy;
    u32 ingress_tstamp;
    u32 egress_tstamp;
    u32 l2_ingress_port_id;
    u32 l2_egress_port_id;
    u32 egress_port_tx_util;
};

BPF_PERF_OUTPUT(events);

// BPF TABLES
BPF_TABLE("lru_hash", struct flow_id_t, struct flow_info_t, tb_flow, 100000);
BPF_TABLE("lru_hash", struct switch_id_t, struct switch_info_t, tb_switch, 100);
BPF_TABLE("lru_hash", struct link_id_t, struct link_info_t, tb_link, 1000);
BPF_TABLE("lru_hash", struct queue_id_t, struct queue_info_t, tb_queue, 10000);

int report_collector(struct xdp_md *ctx) {
    
    void* data_end = (void*)(long)ctx->data_end;
    void* cursor = (void*)(long)ctx->data; 

    struct eth_tp *eth;
    CURSOR_ADVANCE(eth, cursor, sizeof(*eth), data_end);
    if (unlikely(ntohs(eth->type) != ETHERTYPE_IP)) return XDP_PASS;

    struct iphdr *ip;
    CURSOR_ADVANCE(ip, cursor, sizeof(*ip), data_end);
    if (unlikely(ip->protocol != IPPROTO_UDP)) return XDP_PASS;

    struct udphdr *udp;
    CURSOR_ADVANCE(udp, cursor, sizeof(*udp), data_end);
    if (unlikely(ntohs(udp->dest) != INT_DST_PORT)) return XDP_PASS;

    struct telemetry_report_t *report;
    CURSOR_ADVANCE(report, cursor, sizeof(*report), data_end);

    // Inner packet
    CURSOR_ADVANCE_NO_PARSE(cursor, ETH_SIZE, data_end);

    struct iphdr *in_ip;
    CURSOR_ADVANCE(in_ip, cursor, sizeof(*in_ip), data_end);

    struct ports_t *in_ports;
    CURSOR_ADVANCE(in_ports, cursor, sizeof(*in_ports), data_end);

    u8 remain_size = (in_ip->protocol == IPPROTO_UDP)? 
                    (UDPHDR_SIZE - sizeof(*in_ports)) : 
                    (TCPHDR_SIZE - sizeof(*in_ports));
    CURSOR_ADVANCE_NO_PARSE(cursor, remain_size, data_end);
    CURSOR_ADVANCE_NO_PARSE(cursor, INT_SHIM_SIZE, data_end);

    struct INT_metadata_hdr_t *int_metadata_hdr;
    CURSOR_ADVANCE(int_metadata_hdr, cursor, sizeof(*int_metadata_hdr), data_end);

    struct INT_metadata_t *int_data;

    struct flow_info_t flow_info = {};
    flow_info.src_ip = ntohl(in_ip->saddr);
    flow_info.dst_ip = ntohl(in_ip->daddr);
    flow_info.src_port = ntohs(in_ports->source);
    flow_info.dst_port = ntohs(in_ports->dest);
    flow_info.ip_proto = in_ip->protocol;
    flow_info.hop_cnt = 0;
    flow_info.e_new_flow = 0;
    flow_info.e_flow_latency = 0;
    flow_info.e_sw_latency = 0;
    flow_info.e_link_latency = 0;
    flow_info.e_q_occupancy = 0;

    // each hop metadata
    struct INT_hop_metadata_t hop_metadata[MAX_INT_HOP] = {};

    /*
        PARSE INT METADATA
        - switch id 32b
        - ingress port 16b + egress port 16b
        - hop latency 32b
        - q_id 8b + q_occupancy 24b
        - ingress tstamp 32b
        - egress tstamp 32b
        - l2 ports 32b + 32b UNUSED
        - egress tx port util 32b
    */
    #pragma unroll
    for (u8 i = 0; (i < MAX_INT_HOP)/* && (i < flow_info.hop_cnt)*/; i++) {
        CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
        flow_info.switch_ids[i] =  ntohl(int_data->data);
        flow_info.hop_cnt += 1;
        u8 update = 0;

        if (int_metadata_hdr->ins_map & 0x4000) {
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].ingress_port_id = ntohs((u16)(int_data->data >> 16));
            hop_metadata[i].egress_port_id = ntohs((u16)(int_data->data & 0x0000ffff));
            
            // UPDATE LINK TABLE
            if ((i-1) >= 0) {
                struct link_id_t link_id = {};
                link_id.egress_switch_id = hop_metadata[i].switch_id;
                link_id.egress_port_id = hop_metadata[i].egress_port_id;
                link_id.ingress_switch_id = hop_metadata[i-1].switch_id;
                link_id.ingress_port_id = hop_metadata[i-1].ingress_port_id;

                struct link_info_t link_info = {};
                link_info.link_latency = hop_metadata[i].ingress_tstamp
                                         - hop_metadata[i-1].egress_tstamp;

                struct link_info_t *link_info_p = tb_link.lookup(&link_id);
                if (unlikely(!link_info_p)) {
                    update = 1;
                } else if ( ABS(link_info.link_latency, link_info_p->link_latency)
                            > LINK_LATENCY_THRESHOLD) {
                    update = 1;
                }

                if (update) {
                    tb_link.update(&link_id, &link_info);
                    flow_info.e_link_latency = 1;
                    update = 0;
                }
            }
        }

        if (int_metadata_hdr->ins_map & 0x2000) {
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].hop_latency = ntohl(int_data->data);
            flow_info.flow_latency += ntohl(int_data->data);
            
            // UPDATE SWITCH TABLE
            struct switch_id_t sw_id = {};
            sw_id.switch_id = hop_metadata[i].switch_id;
            struct switch_info_t switch_info = {};
            switch_info.hop_latency = hop_metadata[i].hop_latency;
            
            struct switch_info_t *switch_info_p = tb_switch.lookup(&sw_id);

            if (unlikely(!switch_info_p)) {
                update = 1;
            } else if(  ABS(switch_info.hop_latency, switch_info_p->hop_latency)
                        > HOP_LATENCY_THRESHOLD) {
                update = 1;
            }

            if (update) {
                tb_switch.update(&sw_id, &switch_info);
                flow_info.e_sw_latency = 1;
                update = 0;
            }
        }
        if (int_metadata_hdr->ins_map & 0x1000) {
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].q_id = (u8)(int_data->data >> 24);
            hop_metadata[i].q_occupancy =
                (int_data->data & 0x00ff0000) + ntohl(int_data->data & 0x0000ffff);

            // UPDATE QUEUE TABLE
            struct queue_id_t queue_id = {};
            queue_id.switch_id = hop_metadata[i].switch_id;
            queue_id.q_id = hop_metadata[i].q_id;
            
            struct queue_info_t queue_info = {};
            queue_info.q_occupancy = hop_metadata[i].q_occupancy;

            struct queue_info_t *queue_info_p = tb_queue.lookup(&queue_id);
            if (unlikely(!queue_info_p)) {
                update = 1;
            } else if ( ABS(queue_info.q_occupancy, queue_info_p->q_occupancy)
                            > QUEUE_OCCUPANCY_THRESHOLD) {
                update = 1;
            }

            if (update) {
                tb_queue.update(&queue_id, &queue_info);
                flow_info.e_q_occupancy = 1;
                update = 0;
            }
        }
        if (int_metadata_hdr->ins_map & 0x0800) {
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].ingress_tstamp = ntohl(int_data->data);
        }
        if (int_metadata_hdr->ins_map & 0x0400) {
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].egress_tstamp = ntohl(int_data->data);
        }
        if (int_metadata_hdr->ins_map & 0x0200) {
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].l2_ingress_port_id = ntohl(int_data->data);
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].l2_egress_port_id = ntohl(int_data->data);
        }
        if (int_metadata_hdr->ins_map & 0x0100) {
            CURSOR_ADVANCE(int_data, cursor, sizeof(*int_data), data_end);
            hop_metadata[i].egress_port_tx_util = ntohl(int_data->data);
        }

        if (cursor >= data_end) {
            break;
        }
    }

    struct flow_id_t flow_id = {};
    flow_id.src_ip = flow_info.src_ip;
    flow_id.dst_ip = flow_info.dst_ip;
    flow_id.src_port = flow_info.src_port;
    flow_id.dst_port = flow_info.dst_port;
    flow_id.ip_proto = flow_info.ip_proto;
    

    u8 flow_update = 0;
    struct flow_info_t *flow_info_p = tb_flow.lookup(&flow_id);
    if (unlikely(!flow_info_p)) {
        // NEW FLOW
        flow_info.e_new_flow = 1;
        flow_update = 1;
    } else if (ABS(flow_info.flow_latency, flow_info_p->flow_latency)
                > FLOW_LATENCY_THRESHOLD) {
        flow_info.e_flow_latency = 1;
        flow_update = 1;
    }
    
    if (flow_update) {
        tb_flow.update(&flow_id, &flow_info);
    }

    // submit event
    if (unlikely(
        flow_info.e_flow_latency | flow_info.e_link_latency |
        flow_info.e_new_flow | flow_info.e_sw_latency |
        flow_info.e_q_occupancy
    )) {
        events.perf_submit(ctx, &flow_info, sizeof(flow_info));
    }

    return XDP_DROP;
}