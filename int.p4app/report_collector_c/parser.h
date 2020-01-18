#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <arpa/inet.h>
#include "util.h"

// TODO fix endianness
typedef struct telemetry_report_t {
    uint8_t     ver:4,
                length:4;
    uint16_t    nprot:3,
                rep_md_bits:6,
                rsvd:6,
                d:1;
    uint8_t     q:1,
                f:1,
                hw_id:6;
    uint32_t    switch_id;
    uint32_t    seq_num;
    uint32_t    ingress_tstamp;
} __attribute__((packed)) telemetry_report_t;

typedef struct INT_shim_t {
    uint8_t type;
    uint8_t rsvd1;
    uint8_t length;
    uint8_t dscp:6, 
            rsvd2:2;
} __attribute__((packed)) INT_shim_t;

// TODO fix endianness
typedef struct INT_metadata_hdr_t {
    uint8_t     ver:4,
                rep:2,
                c:1,
                e:1;
    uint8_t     m:1,
                rsvd1:7;
    uint8_t     hop_ml:5,
                rsvd2:3;
    uint8_t     rem_hop_cnt;
    uint16_t    ins_map;
    uint16_t    rsvd3;
} __attribute__((packed)) INT_metadata_hdr_t;

typedef struct INT_metadata_t {
    uint32_t data;
} __attribute__((packed)) INT_metadata_t;

#define MAX_INT_HOP 6

typedef struct flow_info_t {
    uint32_t    src_ip;
    uint32_t    dst_ip;
    uint16_t    src_port;
    uint16_t    dst_port;
    uint8_t     protocol;

    uint32_t    report_tstamp;

    uint8_t     hop_cnt;
	uint32_t    flow_latency;
    uint32_t    switch_ids[MAX_INT_HOP];
    uint16_t    ingress_ports[MAX_INT_HOP];
    uint16_t    egress_ports[MAX_INT_HOP];
    uint32_t    hop_latencies[MAX_INT_HOP];
    uint8_t     queue_ids[MAX_INT_HOP];
    uint32_t    queue_occups[MAX_INT_HOP];
    uint32_t    ingress_tstamps[MAX_INT_HOP];
    uint32_t    egress_tstamps[MAX_INT_HOP];
    uint32_t    egress_tx_utils[MAX_INT_HOP];
} flow_info_t;

typedef struct ports_t {
    uint16_t    src_port;
    uint16_t    dst_port;
} __attribute__((packed)) ports_t;

// minimal report: REP_HDR(16)+ETH(14)+IP(20)+UDP(8)+SHIM(4)+INT_HDR(8)
#define MINIMAL_REPORT_SIZE 70

void* report_parser(void* args);

#endif
