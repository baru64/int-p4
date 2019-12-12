#include <core.p4>
#include <v1model.p4>


#include "headers.p4"
#include "parser.p4"
#include "int_source.p4"
#include "int_transit.p4"
#include "int_sink.p4"
#include "forward.p4"

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
	INT_source() int_source;
	
	Forward() forward;
	
	apply {	
        if(!hdr.ethernet.isValid())
            exit;

		if (hdr.udp.isValid() || hdr.tcp.isValid()) {
		    int_source.apply(hdr, meta, standard_metadata);
        }
		forward.apply(hdr, meta, standard_metadata);
	}
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    INT_transit() int_transit;
	INT_sink() int_sink;
    
    apply {
		if (hdr.udp.isValid() || hdr.tcp.isValid()) {
		    int_transit.apply(hdr, meta, standard_metadata);
		    int_sink.apply(hdr, meta, standard_metadata);
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
