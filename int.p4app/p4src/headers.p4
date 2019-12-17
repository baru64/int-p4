#ifndef _HEADERS_P4_
#define _HEADERS_P4_

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<6>  dscp;
    bit<2>  ecn;
    bit<16> totalLen;
    bit<16> id;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> len;
    bit<16> csum;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNum;
    bit<32> ackNum;
    bit<4>  dataOffset;
    bit<3>  reserved;
    bit<9>  flags;
    bit<16> winSize;
    bit<16> csum;
    bit<16> urgPoint;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

/* INT HEADER */
header int_header_t {
    bit<4>  ver;
    bit<2>  rep;
    bit<1>  c;
    bit<1>  e;
    bit<1>  m;
    bit<7>  rsvd1;
    bit<3>  rsvd2;
    bit<5>  hop_metadata_len;
    bit<8>  remaining_hop_cnt;
    bit<16>  instruction_mask;
    bit<16> rsvd3;
}

header int_data_t {
    varbit<2048> data;
}

/* INT hop metadata headers */
header int_switch_id_t {
    bit<32> switch_id;
}
header int_level1_port_ids_t {
    bit<16> ingress_port_id;
    bit<16> egress_port_id;
}

header int_hop_latency_t {
    bit<32> hop_latency;
}

header int_q_occupancy_t {
    bit<8> q_id;
    bit<24> q_occupancy;
}

header int_ingress_tstamp_t {
    bit<32> ingress_tstamp;
}

header int_egress_tstamp_t {
    bit<32> egress_tstamp;
}

header int_level2_port_ids_t {
    bit<32> ingress_port_id;
    bit<32> egress_port_id;
}

header int_egress_port_tx_util_t {
    bit<32> egress_port_tx_util;
}

/* INT SHIM */
header intl4_shim_t {
    bit<8> int_type;
    bit<8> rsvd1;
    bit<8> len;
    bit<6> dscp;
    bit<2> rsvd2;
}

/* TELEMETRY REPORT */
header report_fixed_header_t {
    bit<4> ver;
    bit<4> len;
    bit<3> nprot;
    bit<6> rep_md_bits;
    bit<6> reserved;
    bit<1> d;
    bit<1> q;
    bit<1> f;
    bit<6> hw_id;
    bit<32> switch_id;
    bit<32> seq_num;
    bit<32> ingress_tstamp;
}

const bit<8> REPORT_FIXED_HEADER_LEN = 16;

/* INT METADATA */
struct int_meta_t {
    bit<1>  source;
    bit<1>  transit;
    bit<1>  sink;
    bit<32> switch_id;
    bit<16> l3_mtu;
    bit<16> byte_cnt;
    bit<8>  word_cnt;
}

/*************/
struct metadata {
    int_meta_t           int_meta;
}

struct headers {
    // INT report headers
    ethernet_t                  report_ethernet;
    ipv4_t                      report_ipv4;
    udp_t                       report_udp;
    report_fixed_header_t       report_fixed_header;
    // normal headers
    ethernet_t                  ethernet;
    ipv4_t                      ipv4;
    tcp_t                       tcp;
    udp_t                       udp;
    // INT headers
    intl4_shim_t                int_shim;
    int_header_t                int_header;
    int_switch_id_t             int_switch_id;
    int_level1_port_ids_t       int_level1_port_ids;
    int_hop_latency_t           int_hop_latency;
    int_q_occupancy_t           int_q_occupancy;
    int_ingress_tstamp_t        int_ingress_tstamp;
    int_egress_tstamp_t         int_egress_tstamp;
    int_level2_port_ids_t       int_level2_port_ids;
    int_egress_port_tx_util_t   int_egress_port_tx_util;
    int_data_t                  int_data;
}

#endif
