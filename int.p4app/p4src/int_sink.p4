control INT_sink(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    action int_sink() {
        meta.int_meta.sink = 1w1;
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

