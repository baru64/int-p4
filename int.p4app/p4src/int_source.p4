//#include <core.p4>
//#include <v1model.p4>

#include "headers.p4"
#include "parser.p4"

control INT_source(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    action int_source(  bit<8> max_hop,
                        bit<5> ins_cnt,
                        bit<16> ins_map) {
        hdr.int_shim.setValid();
        hdr.int_shim.int_type = 8w1;
        hdr.int_shim.len = 8w3;
        hdr.int_shim.dscp = hdr.ipv4.dscp;

        hdr.int_header.setValid();
        hdr.int_header.ver = 4w1;
        hdr.int_header.rep = 2w0;
        hdr.int_header.c = 1w0;
        hdr.int_header.e = 1w0;
        hdr.int_header.m = 1w0;
        hdr.int_header.rsvd1 = 7w0;
        hdr.int_header.rsvd2 = 3w0;
        hdr.int_header.hop_metadata_len = ins_cnt;
        hdr.int_header.remaining_hop_cnt = max_hop;
        hdr.int_header.instruction_mask = ins_map;
        hdr.int_header.rsvd3 = 16w0;

        hdr.ipv4.totalLen = hdr.ipv4.totalLen + 16w12;
        hdr.udp.len = hdr.udp.len + 16w12;

        hdr.ipv4.dscp = 6w0x17;
    }
    action int_set_source() {
        meta.int_meta.source = 1w1;
    }

    /************ TABLES ************/

    table tb_set_source {
        actions = {
            int_set_source;
        }
        key = {
            standard_metadata.ingress_port: exact;
        }
        size = 255;
    }

    table tb_int_source {
        actions = {
            int_source;
        }
        key = {
            hdr.ipv4.srcAddr    : ternary;
            hdr.ipv4.dstAddr    : ternary;
            hdr.tcp.srcPort     : ternary;     
            hdr.tcp.dstPort     : ternary;     
            hdr.udp.srcPort     : ternary;     
            hdr.udp.dstPort     : ternary;     
        }
        size = 127;
    }

    /*********** APPLY ***********/
    apply {
        tb_set_source.apply();
        if (meta.int_meta.source == 1w1) {
            tb_int_source.apply();
        }
    }
}
