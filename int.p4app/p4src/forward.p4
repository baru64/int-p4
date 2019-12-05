control Forward(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    action send_to_port(bit<9> port) {
        standard_metadata.egress_port = port;
        standard_metadata.egress_spec = port;
    }

    table tb_forward {
        actions = {
            send_to_port;
        }
        key = {
            hdr.ethernet.dstAddr: ternary;
        }
        size = 48;
    }

    apply {
        tb_forward.apply();
    }
}