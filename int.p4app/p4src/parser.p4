#ifndef _PARSER_P4_
#define _PARSER_P4_

#include <core.p4>
#include <v1model.p4>

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }

    state parse_int_shim {
        packet.extract(hdr.int_shim);
        transition parse_int_header;
    }
    state parse_int_header {
        packet.extract(hdr.int_header);
        transition parse_int_data;
    }
    state parse_int_data {
        packet.extract(hdr.int_data, ((bit<32>)(hdr.int_shim.len - 3)) << 5);
        transition accept;
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);

        transition select(hdr.ipv4.protocol) {
            8w0x11: parse_udp;
            8w0x6: parse_tcp;
            default: accept;
        }
    }

    state parse_tcp {
        packet.extract(hdr.tcp);

        transition select(hdr.ipv4.dscp) {
            6w0x17: parse_int_shim;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract(hdr.udp);

        transition select(hdr.ipv4.dscp) {
            6w0x17 &&& 6w0x3F : parse_int_shim;
            default: accept;
        }
    }
    state start {
        transition parse_ethernet;
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        // raport headers
        packet.emit(hdr.report_ethernet);
        packet.emit(hdr.report_ipv4);
        packet.emit(hdr.report_udp);
        packet.emit(hdr.report_fixed_header);
        // original headers
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
        // int header
        packet.emit(hdr.int_shim);
        packet.emit(hdr.int_header);
        
        // hop metadata
        packet.emit(hdr.int_switch_id);
        packet.emit(hdr.int_level1_port_ids);
        packet.emit(hdr.int_hop_latency);
        packet.emit(hdr.int_q_occupancy);
        packet.emit(hdr.int_ingress_tstamp);
        packet.emit(hdr.int_egress_tstamp);
        packet.emit(hdr.int_level2_port_ids);
        packet.emit(hdr.int_egress_port_tx_util);

        // int data
        packet.emit(hdr.int_data);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        /* IPv4 CHECKSUM */
        update_checksum(
            hdr.ipv4.isValid(),
            {
                hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp,hdr.ipv4.ecn,
                hdr.ipv4.totalLen, hdr.ipv4.id, hdr.ipv4.flags,
                hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol,
                hdr.ipv4.srcAddr, hdr.ipv4.dstAddr
            },
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16
        );
        /* UDP CHECKSUM */
        update_checksum(
            hdr.udp.isValid(),
            {
                hdr.udp.srcPort, hdr.udp.dstPort, hdr.udp.len
            },
            hdr.udp.csum,
            HashAlgorithm.csum16
        );
        /* INT REPORT UDP CHECKSUM */
        update_checksum(
            hdr.report_udp.isValid(),
            {
                hdr.report_udp.srcPort, hdr.report_udp.dstPort, hdr.report_udp.len
            },
            hdr.report_udp.csum,
            HashAlgorithm.csum16
        );
        /* TCP CHECKSUM */
        // DOESN'T WORK
        // update_checksum_with_payload(
        //     hdr.tcp.isValid(),
        //     {
        //         hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.totalLen,
        //         hdr.tcp.srcPort, hdr.tcp.dstPort,
        //         hdr.tcp.seqNum, hdr.tcp.ackNum,
        //         hdr.tcp.dataOffset, hdr.tcp.reserved, hdr.tcp.flags,
        //         hdr.tcp.winSize, hdr.tcp.urgPoint
        //     },
        //     hdr.tcp.csum,
        //     HashAlgorithm.csum16
        // );
    }
}
#endif