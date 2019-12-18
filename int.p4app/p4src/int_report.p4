// register to store seq_num
register<bit<32>> (1) report_seq_num_register;

control INT_report(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    bit<32> seq_num_value = 0;

    action make_report(
        bit<48> src_mac, bit<48> dst_mac,
        bit<32> src_ip, bit<32> dst_ip,
        bit<16> dst_port
    ) {
        // INT Raport structure
        // [Eth][IP][UDP][INT RAPORT HDR][ETH][IP][UDP/TCP][INT SHIM][INT DATA]
        // Ethernet **********************************************************
        hdr.report_ethernet.setValid();
        hdr.report_ethernet.dstAddr = dst_mac;
        hdr.report_ethernet.srcAddr = src_mac;
        hdr.report_ethernet.etherType = 0x0800;

        // IPv4 **************************************************************
        hdr.report_ipv4.setValid();
        hdr.report_ipv4.version = 4;
        hdr.report_ipv4.ihl = 5;
        hdr.report_ipv4.dscp = 0;
        hdr.report_ipv4.ecn = 0;
        // 2x ipv4 header + udp header + eth header + report header + int data len
        hdr.report_ipv4.totalLen = (bit<16>)(20 + 20 + 8 + 14)
                                    + (bit<16>)REPORT_FIXED_HEADER_LEN
                                    + (((bit<16>)hdr.int_shim.len) << 2 );
        // add size of original tcp/udp header
        if (hdr.tcp.isValid()) {
            hdr.report_ipv4.totalLen = hdr.report_ipv4.totalLen
                                       + (((bit<16>)hdr.tcp.dataOffset) << 2);
        } else {
            hdr.report_ipv4.totalLen = hdr.report_ipv4.totalLen + 8;
        }
        hdr.report_ipv4.id = 0;
        hdr.report_ipv4.flags = 0;
        hdr.report_ipv4.fragOffset = 0;
        hdr.report_ipv4.ttl = 64;
        hdr.report_ipv4.protocol = 17; // UDP
        hdr.report_ipv4.srcAddr = src_ip;
        hdr.report_ipv4.dstAddr = dst_ip;

        // UDP ***************************************************************
        hdr.report_udp.setValid();
        hdr.report_udp.srcPort = 0;
        hdr.report_udp.dstPort = dst_port;
        // ipv4 header + 2x udp header + eth header + report header + int data len
        // hdr.report_udp.len = (bit<16>)(20 + 8 + 8 + 14)
        //                       + (bit<16>)REPORT_FIXED_HEADER_LEN
        //                       + (((bit<16>)hdr.int_shim.len) << 2 );
        hdr.report_udp.len = hdr.report_ipv4.totalLen - 20;
        
        // Report fixed header ***********************************************
        hdr.report_fixed_header.setValid();
        hdr.report_fixed_header.ver = 1;
        hdr.report_fixed_header.len = 4;
        hdr.report_fixed_header.nprot = 0; // 0 for Ethernet
        hdr.report_fixed_header.rep_md_bits = 0;
        hdr.report_fixed_header.reserved = 0;
        hdr.report_fixed_header.d = 0;
        hdr.report_fixed_header.q = 0;
        // f - indicates that report is for tracked flow, INT data is present
        hdr.report_fixed_header.f = 1;
        // hw_id - specific to the switch, e.g. id of linecard
        hdr.report_fixed_header.hw_id = 0;
        hdr.report_fixed_header.switch_id = meta.int_meta.switch_id;
        report_seq_num_register.read(seq_num_value, 0);
        hdr.report_fixed_header.seq_num = seq_num_value;
        report_seq_num_register.write(0, seq_num_value + 1);
        hdr.report_fixed_header.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        
        // Original packet headers, INT shim and INT data come after report header.
        // drop all data besides int report and report eth header
        truncate((bit<32>)hdr.report_ipv4.totalLen + 14);
    }

    table tb_make_report {
        actions = {
            make_report;
        }
    }

    apply {
        tb_make_report.apply();
    }
}