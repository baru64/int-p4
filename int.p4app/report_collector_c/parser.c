#include "parser.h"

void* report_parser(void* args) {
    Context* ctx = (Context*) args;
    printf("parser starts!\n");
    while (!*ctx->terminate) {
        int packet_size;
        uint8_t* packet = dequeue_pop(ctx->parser_dq, &packet_size);
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
            if (flow_info->hop_cnt > MAX_INT_HOP) {
                printf("parser: too many hops");
                free(packet);
                continue;
            }
            flow_info->src_ip = ntohl(ip->saddr);
            flow_info->dst_ip = ntohl(ip->daddr);
            flow_info->src_port = ntohs(ports->src_port);
            flow_info->dst_port = ntohs(ports->dst_port);
            flow_info->protocol = ip->protocol;
            flow_info->report_tstamp = ntohl(report_header->ingress_tstamp);

            // extract int metadata
            int i;
            for (i = 0; i < flow_info->hop_cnt; ++i) {
                if (int_hdr->ins_map & 0x8000) {
                    flow_info->switch_ids[i] = ntohl(*(uint32_t*)cursor);
                    cursor += sizeof(uint32_t);
                }
                if (int_hdr->ins_map & 0x4000) {
                    flow_info->ingress_ports[i] = ntohs(*(uint16_t*)cursor);
                    cursor += sizeof(uint16_t);
                    flow_info->egress_ports[i] = ntohs(*(uint16_t*)cursor);
                    cursor += sizeof(uint16_t);
                }
                if (int_hdr->ins_map & 0x2000) {
                    flow_info->hop_latencies[i] = ntohl(*(uint32_t*)cursor);
                    cursor += sizeof(uint32_t);
                }
                if (int_hdr->ins_map & 0x1000) {
                    uint32_t queue_metadata = *(uint32_t*)cursor;
                    cursor += sizeof(uint32_t);
                    flow_info->queue_ids[i] = (uint8_t)(queue_metadata >> 24);
                    flow_info->queue_occups[i] = ntohl(queue_metadata & 0x00ffffff);
                }
                if (int_hdr->ins_map & 0x0800) {
                    flow_info->ingress_tstamps[i] = ntohl(*(uint32_t*)cursor);
                    cursor += sizeof(uint32_t);
                }
                if (int_hdr->ins_map & 0x0400) {
                    flow_info->egress_tstamps[i] = ntohl(*(uint32_t*)cursor);
                    cursor += sizeof(uint32_t);
                }
                if (int_hdr->ins_map & 0x0200) {
                    // skip l2_port_ids
                    cursor += sizeof(uint32_t) * 2;
                }
                if (int_hdr->ins_map & 0x0100) {
                    flow_info->egress_tx_utils[i] = ntohl(*(uint32_t*)cursor);
                    cursor += sizeof(uint32_t);
                }
            }

            // set flow latency
            flow_info->flow_latency = 
                flow_info->egress_tstamps[flow_info->hop_cnt-1] - flow_info->ingress_tstamps[0];
            
            // enqueue flow_info for exporter
            dequeue_push(ctx->exporter_dq, flow_info, sizeof(flow_info_t));
            free(packet);
        }
    }
    printf("parser exiting...\n");
    pthread_exit(0);
}
