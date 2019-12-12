//#include <core.p4>
//#include <v1model.p4>

#include "headers.p4"
#include "parser.p4"

control INT_transit(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    /********* UPDATE ACTIONS *********/
    action int_update_ipv4_ac() {
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + (bit<16>)meta.int_meta.byte_cnt;
    }
    action int_update_shim_ac() {
        hdr.int_shim.len = hdr.int_shim.len + (bit<8>)hdr.int_header.hop_metadata_len;
    }
    action int_update_udp_ac() {
        hdr.udp.len = hdr.udp.len + (bit<16>)meta.int_meta.byte_cnt;
    }
    action int_transit_config(bit<32> switch_id, bit<16> l3_mtu) {
        meta.int_meta.l3_mtu = l3_mtu;
        meta.int_meta.switch_id = switch_id;
        meta.int_meta.byte_cnt = (bit<16>) hdr.int_header.hop_metadata_len << 2;
        meta.int_meta.word_cnt = (bit<8>) hdr.int_header.hop_metadata_len;
    }
    action int_hop_cnt_decrement() {
        hdr.int_header.remaining_hop_cnt = hdr.int_header.remaining_hop_cnt - 1;
    }
    action int_hop_cnt_exceeded() {
        hdr.int_header.e = 1w1;
    }
    action int_mtu_exceeded() {
        hdr.int_header.m = 1w1;
    }

    /****** INT HOP METADATA ACTIONS *******/
    action int_set_header_0() {
        hdr.int_switch_id.setValid();
        hdr.int_switch_id.switch_id = meta.int_meta.switch_id;
    }
    action int_set_header_1() {
        hdr.int_level1_port_ids.setValid();
        hdr.int_level1_port_ids.ingress_port_id = (bit<16>)standard_metadata.ingress_port;
        hdr.int_level1_port_ids.egress_port_id = (bit<16>)standard_metadata.egress_port;
    }
    action int_set_header_2() {
        hdr.int_hop_latency.setValid();
        hdr.int_hop_latency.hop_latency = (bit<32>)standard_metadata.deq_timedelta;
    }
    action int_set_header_3() {
        hdr.int_q_occupancy.setValid();
        hdr.int_q_occupancy.q_id = 0x00; // qid not defined in v1model
        hdr.int_q_occupancy.q_occupancy = (bit<24>)standard_metadata.enq_qdepth;
    }
    action int_set_header_4() {
        hdr.int_ingress_tstamp.setValid();
        hdr.int_ingress_tstamp.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
    }
    action int_set_header_5() {
        hdr.int_egress_tstamp.setValid();
        hdr.int_egress_tstamp.egress_tstamp = (bit<32>)standard_metadata.egress_global_timestamp;
    }
    action int_set_header_6() {
        // no such metadata in v1model
        hdr.int_level2_port_ids.ingress_port_id = 32w0;
        hdr.int_level2_port_ids.egress_port_id = 32w0;
    }
    action int_set_header_7() {
        // no such metadata in v1model
        hdr.int_egress_port_tx_util.egress_port_tx_util = 32w0;
    }

    /******** INT BITS 0-3 ********/
    action int_set_header_0003_i0() {
    }
    action int_set_header_0003_i1() {
        int_set_header_3();
    }
    action int_set_header_0003_i2() {
        int_set_header_2();
    }
    action int_set_header_0003_i3() {
        int_set_header_3();
        int_set_header_2();
    }
    action int_set_header_0003_i4() {
        int_set_header_1();
    }
    action int_set_header_0003_i5() {
        int_set_header_3();
        int_set_header_1();
    }
    action int_set_header_0003_i6() {
        int_set_header_2();
        int_set_header_1();
    }
    action int_set_header_0003_i7() {
        int_set_header_3();
        int_set_header_2();
        int_set_header_1();
    }
    action int_set_header_0003_i8() {
        int_set_header_0();
    }
    action int_set_header_0003_i9() {
        int_set_header_3();
        int_set_header_0();
    }
    action int_set_header_0003_i10() {
        int_set_header_2();
        int_set_header_0();
    }
    action int_set_header_0003_i11() {
        int_set_header_3();
        int_set_header_2();
        int_set_header_0();
    }
    action int_set_header_0003_i12() {
        int_set_header_1();
        int_set_header_0();
    }
    action int_set_header_0003_i13() {
        int_set_header_3();
        int_set_header_1();
        int_set_header_0();
    }
    action int_set_header_0003_i14() {
        int_set_header_2();
        int_set_header_1();
        int_set_header_0();
    }
    action int_set_header_0003_i15() {
        int_set_header_3();
        int_set_header_2();
        int_set_header_1();
        int_set_header_0();
    }
    /******** INT BITS 4-7 ********/
    action int_set_header_0407_i0() {
    }
    action int_set_header_0407_i1() {
        int_set_header_7();
    }
    action int_set_header_0407_i2() {
        int_set_header_6();
    }
    action int_set_header_0407_i3() {
        int_set_header_7();
        int_set_header_6();
    }
    action int_set_header_0407_i4() {
        int_set_header_5();
    }
    action int_set_header_0407_i5() {
        int_set_header_5();
        int_set_header_7();
    }
    action int_set_header_0407_i6() {
        int_set_header_5();
        int_set_header_6();
    }
    action int_set_header_0407_i7() {
        int_set_header_7();
        int_set_header_6();
        int_set_header_5();
    }
    action int_set_header_0407_i8() {
        int_set_header_4();
    }
    action int_set_header_0407_i9() {
        int_set_header_4();
        int_set_header_7();
    }
    action int_set_header_0407_i10() {
        int_set_header_4();
        int_set_header_6();
    }
    action int_set_header_0407_i11() {
        int_set_header_4();
        int_set_header_7();
        int_set_header_6();
    }
    action int_set_header_0407_i12() {
        int_set_header_5();
        int_set_header_4();
    }
    action int_set_header_0407_i13() {
        int_set_header_4();
        int_set_header_5();
        int_set_header_7();
    }
    action int_set_header_0407_i14() {
        int_set_header_4();
        int_set_header_5();
        int_set_header_6();
    }
    action int_set_header_0407_i15() {
        int_set_header_4();
        int_set_header_5();
        int_set_header_6();
        int_set_header_7();
    }

    /************ TABLES ************/
    table tb_int_inst_0003 {
        key = {
            hdr.int_header.instruction_mask : ternary;
        }
        actions = {
            int_set_header_0003_i0;
            int_set_header_0003_i1;
            int_set_header_0003_i2;
            int_set_header_0003_i3;
            int_set_header_0003_i4;
            int_set_header_0003_i5;
            int_set_header_0003_i6;
            int_set_header_0003_i7;
            int_set_header_0003_i8;
            int_set_header_0003_i9;
            int_set_header_0003_i10;
            int_set_header_0003_i11;
            int_set_header_0003_i12;
            int_set_header_0003_i13;
            int_set_header_0003_i14;
            int_set_header_0003_i15;
        }
        default_action = int_set_header_0003_i0();
        size = 16;
        const entries = {
            0x00 &&& 0xF0 : int_set_header_0003_i0();
            0x10 &&& 0xF0 : int_set_header_0003_i1();
            0x20 &&& 0xF0 : int_set_header_0003_i2();
            0x30 &&& 0xF0 : int_set_header_0003_i3();
            0x40 &&& 0xF0 : int_set_header_0003_i4();
            0x50 &&& 0xF0 : int_set_header_0003_i5();
            0x60 &&& 0xF0 : int_set_header_0003_i6();
            0x70 &&& 0xF0 : int_set_header_0003_i7();
            0x80 &&& 0xF0 : int_set_header_0003_i8();
            0x90 &&& 0xF0 : int_set_header_0003_i9();
            0xa0 &&& 0xF0 : int_set_header_0003_i10();
            0xb0 &&& 0xF0 : int_set_header_0003_i11();
            0xc0 &&& 0xF0 : int_set_header_0003_i12();
            0xd0 &&& 0xF0 : int_set_header_0003_i13();
            0xe0 &&& 0xF0 : int_set_header_0003_i14();
            0xf0 &&& 0xF0 : int_set_header_0003_i15();
        }
    }

    table tb_int_inst_0407 {
        actions = {
            int_set_header_0407_i0;
            int_set_header_0407_i1;
            int_set_header_0407_i2;
            int_set_header_0407_i3;
            int_set_header_0407_i4;
            int_set_header_0407_i5;
            int_set_header_0407_i6;
            int_set_header_0407_i7;
            int_set_header_0407_i8;
            int_set_header_0407_i9;
            int_set_header_0407_i10;
            int_set_header_0407_i11;
            int_set_header_0407_i12;
            int_set_header_0407_i13;
            int_set_header_0407_i14;
            int_set_header_0407_i15;
        }
        key = {
            hdr.int_header.instruction_mask: ternary;
        }
        size = 16;
        const entries = {
            0x00 &&& 0x0F : int_set_header_0407_i0();
            0x01 &&& 0x0F : int_set_header_0407_i1();
            0x02 &&& 0x0F : int_set_header_0407_i2();
            0x03 &&& 0x0F : int_set_header_0407_i3();
            0x04 &&& 0x0F : int_set_header_0407_i4();
            0x05 &&& 0x0F : int_set_header_0407_i5();
            0x06 &&& 0x0F : int_set_header_0407_i6();
            0x07 &&& 0x0F : int_set_header_0407_i7();
            0x08 &&& 0x0F : int_set_header_0407_i8();
            0x09 &&& 0x0F : int_set_header_0407_i9();
            0x0a &&& 0x0F : int_set_header_0407_i10();
            0x0b &&& 0x0F : int_set_header_0407_i11();
            0x0c &&& 0x0F : int_set_header_0407_i12();
            0x0d &&& 0x0F : int_set_header_0407_i13();
            0x0e &&& 0x0F : int_set_header_0407_i14();
            0x0f &&& 0x0F : int_set_header_0407_i15();
        }
    }

    table tb_int_transit {
        actions = {
            int_transit_config;
        }
    }

    apply {
        if (hdr.udp.isValid() || hdr.tcp.isValid()) {
            if (hdr.int_header.isValid()) {
                tb_int_transit.apply();
                if (hdr.int_header.remaining_hop_cnt == 0 || hdr.int_header.e == 1) {
                    int_hop_cnt_exceeded();
                    return;
                }
                if ((hdr.ipv4.totalLen + meta.int_meta.byte_cnt) > meta.int_meta.l3_mtu) {
                    int_mtu_exceeded();     
                    return;
                }
                tb_int_inst_0003.apply();
                tb_int_inst_0407.apply();
                
                int_hop_cnt_decrement();
                int_update_ipv4_ac();
                if (hdr.udp.isValid()) int_update_udp_ac();
                if (hdr.int_shim.isValid()) int_update_shim_ac();
            }
          }
    }
}
