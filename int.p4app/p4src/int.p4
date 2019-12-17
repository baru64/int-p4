#include <core.p4>
#include <v1model.p4>


#include "headers.p4"
#include "parser.p4"
#include "int_source.p4"
#include "int_transit.p4"
#include "int_sink.p4"
#include "int_report.p4"
#include "forward.p4"

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
	INT_source() int_source;
    INT_sink_ingress() int_sink_ingress;
	
	Forward() forward;
	
	apply {	
        if(!hdr.ethernet.isValid())
            exit;

		forward.apply(hdr, meta, standard_metadata);
		if (hdr.udp.isValid() || hdr.tcp.isValid()) {
		    int_source.apply(hdr, meta, standard_metadata);
            int_sink_ingress.apply(hdr, meta, standard_metadata);
        }
        if (meta.int_meta.sink == 1w1) {
            // clone packet for INT report
            clone3(CloneType.I2E, 100, standard_metadata);
        }
	}
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    INT_transit() int_transit;
	INT_sink_egress() int_sink_egress;
    INT_report() int_report;
    
    apply {
		if (hdr.udp.isValid() || hdr.tcp.isValid()) {
		    int_transit.apply(hdr, meta, standard_metadata);

            // instance type 0 -> NORMAL, instance type 1 -> ingress clone
            if (standard_metadata.instance_type == 1) {
                // send report to INT collector
                int_report.apply(hdr, meta, standard_metadata);
            }
            if ((meta.int_meta.sink == 1w1) &&
                (standard_metadata.instance_type != 1)) {
                int_sink_egress.apply(hdr, meta, standard_metadata);
            }
        }
    }
}

V1Switch(
	ParserImpl(),
    verifyChecksum(),
    ingress(),
    egress(),
    computeChecksum(),
    DeparserImpl()
) main;
