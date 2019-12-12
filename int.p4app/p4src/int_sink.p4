control INT_sink(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    action int_sink() {
        meta.int_meta.sink = 1w1;
        // restore original headers
        hdr.ipv4.dscp = hdr.int_shim.dscp;
        bit<16> len_bytes = ((bit<16>)hdr.int_shim.len) << 2;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen - len_bytes;
        if (hdr.udp.isValid()) {
            hdr.udp.len = hdr.udp.len - len_bytes;
        }
        // remove int data
        hdr.int_shim.setInvalid();
        hdr.int_header.setInvalid();
        hdr.int_switch_id.setInvalid();
        hdr.int_level1_port_ids.setInvalid();
        hdr.int_hop_latency.setInvalid();
        hdr.int_q_occupancy.setInvalid();
        hdr.int_ingress_tstamp.setInvalid();
        hdr.int_egress_tstamp.setInvalid();
        hdr.int_level2_port_ids.setInvalid();
        hdr.int_egress_port_tx_util.setInvalid();
        hdr.int_data.setInvalid();
    }
    table tb_set_sink {
        actions = {
            int_sink;
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        size = 255;
    }

    apply {
        tb_set_sink.apply();
    }
}

