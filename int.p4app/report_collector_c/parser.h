#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "util.h"

struct telemetry_report_t {
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
} __attribute__((packed));

struct INT_shim_t {
    uint8_t type;
    uint8_t rsvd1;
    uint8_t length;
    uint8_t dscp:6, 
            rsvd2:2;
} __attribute__((packed));

struct INT_metadata_hdr_t {
    uint8_t     ver:4,
                rep:2,
                c:1,
                e:1;
    uint16_t    m:1,
                rsvd1:10,
                hop_ml:5;
    uint8_t     rem_hop_cnt;
    uint16_t    ins_map;
    uint16_t    rsvd2;
} __attribute__((packed));

struct INT_metadata_t {
    uint32_t data;
} __attribute__((packed));

void* report_parser(void* args);

#endif
