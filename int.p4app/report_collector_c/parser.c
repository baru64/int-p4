#include "parser.h"

void* report_parser(void* args) {
    Context* ctx = (Context*) args;
    printf("parser starts!\n");
    while (!*ctx->terminate) {
        int packet_size;
        uint8_t* packet = dequeue_pop(ctx->dq, &packet_size);
        if (packet != NULL) {
            printf("parser got packet len,char: %i %c\n", packet_size, packet[0]);
            if (packet_size < MINIMAL_REPORT_SIZE) {
                printf("parser: packet too small for a report\n");
                free(packet);
                continue;
            }

            uint8_t* cursor = packet;
            
            //extract report header
            telemetry_report_t* report_header = (telemetry_report_t*) cursor;
            cursor += sizeof(telemetry_report_t);

            //skip inner ethernet header
            cursor += 14;
            
            //extract inner ip header
            struct iphdr* ip = (struct iphdr*) cursor;
            cursor += sizeof(struct iphdr);
            
            //extract tcp/udp ports
            ports_t* ports;
            int remaining_size;
            if (ip->protocol == IPPROTO_UDP) {
                ports = (ports_t*) cursor;
                cursor += 8; // udp header size
                remaining_size = packet_size - 58;
            } else if (ip->protocol == IPPROTO_TCP) {
                ports = (ports_t*) cursor;
                cursor += 20; // tcp header size
                remaining_size = packet_size - 70;
            } else {
                printf("parser: wrong protocol\n");
                free(packet);
                continue;
            }

            if (remaining_size < 12) {
                printf("parser: no int headers in packet\n");
                free(packet);
                continue;
            }

            // extract int shim
            INT_shim_t* int_shim = (INT_shim_t*) cursor;  
            cursor += sizeof(INT_shim_t);
            remaining_size -= sizeof(INT_shim_t);

            // extract int header
            INT_metadata_hdr_t* int_hdr = (INT_metadata_hdr_t*) cursor;
            cursor += sizeof(INT_metadata_hdr_t);
            remaining_size -= sizeof(INT_metadata_hdr_t);

            // create new flow_info
            flow_info_t* flow_info = (flow_info_t*) malloc(sizeof(flow_info_t));
            flow_info->hop_cnt = remaining_size / (int_hdr->hop_ml*4);
            // TODO malloc for metadata

            // extract int metadata
        }
    }
    printf("parser exiting...\n");
    pthread_exit(0);
}
