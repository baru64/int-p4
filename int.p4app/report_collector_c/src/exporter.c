#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <curl/curl.h>

#include "util.h"
#include "parser.h"
#include "graphite.h"
#include "influxdb.h"
#include "exporter.h"
#include "list.h"
#include "hash_map.h"

#define INFLUXDB_ADDR "http://10.0.128.1:8086/write?db=int"

void* report_exporter(void* args) {
    Context* ctx = (Context*) args;
    char ipstr[16] = {0};

    printf("exporter starts!\n");
    while(!*ctx->terminate) {
        int info_size;
        flow_info_t* flow_info = dequeue_pop(ctx->exporter_dq, &info_size);
        if (flow_info != NULL) {
            
#ifdef INFLUXDB_EXPORTER
            CURL *curl;
            CURLcode res;
            curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL, INFLUXDB_ADDR);

            struct timespec metric_timestamp;
            clock_gettime(CLOCK_REALTIME, &metric_timestamp);
#endif


            // flow id
            flow_id* fid = malloc(sizeof(flow_id));
            fid->src_ip = flow_info->src_ip;
            fid->dst_ip = flow_info->dst_ip;
            fid->src_port = flow_info->src_port;
            fid->dst_port = flow_info->dst_port;
            fid->protocol = flow_info->protocol;

            int val_len;
            uint32_t* flow_latency = hash_map_get(ctx->flow_map, fid,
                                                  sizeof(flow_id), &val_len);
            if (flow_latency != NULL) {
                if ( ABS(*flow_latency, flow_info->flow_latency)
                     > LATENCY_THRESHOLD ) {
                    printf("new flow_latency: %u\n\n", flow_info->flow_latency);
                    // send data to data base
#ifndef INFLUXDB_EXPORTER
                    graphite_send_flow(fid, *flow_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_flow(curl, fid, *flow_latency, &metric_timestamp);
#endif
                }

                flow_latency = (uint32_t*) malloc(sizeof(uint32_t));
                *flow_latency = flow_info->flow_latency;
                hash_map_insert_or_replace(ctx->flow_map, fid, sizeof(flow_id),
                        flow_latency, sizeof(uint32_t));
                free(fid);

            } else {
                printf("new flow: %u\n", flow_info->flow_latency);
                flow_latency = (uint32_t*) malloc(sizeof(uint32_t));
                *flow_latency = flow_info->flow_latency;
                hash_map_insert_or_replace(ctx->flow_map, fid, sizeof(flow_id),
                        flow_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                graphite_send_flow(fid, *flow_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                influxdb_send_flow(curl, fid, *flow_latency, &metric_timestamp);
#endif
                // insert to list for periodic db update
                list_insert(ctx->flow_id_list, fid, sizeof(flow_id));
            }


            // check switch latency
            int i;
            for (i=0; i < flow_info->hop_cnt; ++i) {
                switch_id* sw_id = malloc(sizeof(switch_id));
                sw_id->switch_id = flow_info->switch_ids[i];
                uint32_t* sw_latency = hash_map_get(ctx->switch_map, sw_id,
                                                sizeof(switch_id), &val_len);
                if (sw_latency != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*sw_latency, flow_info->hop_latencies[i])
                         > SW_LATENCY_THRESHOLD) ) {
                        printf("new sw_latency: %u\n\n", flow_info->hop_latencies[i]);
#ifndef INFLUXDB_EXPORTER
                        graphite_send_switch(sw_id, *sw_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                        influxdb_send_switch(curl, sw_id, *sw_latency, &metric_timestamp);
#endif
                    }

                    sw_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *sw_latency = flow_info->hop_latencies[i];
                    hash_map_insert_or_replace(ctx->switch_map, sw_id,
                        sizeof(switch_id), sw_latency, sizeof(uint32_t));
                    free(sw_id);

                } else {
                    printf("new switch: %u\n", flow_info->hop_latencies[i]);
                    sw_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *sw_latency = flow_info->hop_latencies[i];
                    hash_map_insert_or_replace(ctx->switch_map, sw_id, sizeof(switch_id),
                            sw_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                    graphite_send_switch(sw_id, *sw_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_switch(curl, sw_id, *sw_latency, &metric_timestamp);
#endif
                    // insert to list for periodic db update
                    list_insert(ctx->switch_id_list, sw_id, sizeof(switch_id));
                }
            }


            // check link latency
            for (i=0; i < (flow_info->hop_cnt-1); ++i) {
                link_id* link_id = malloc(sizeof(link_id));
                link_id->ingress_port_id = flow_info->ingress_ports[i+1];
                link_id->ingress_switch_id = flow_info->switch_ids[i+1];
                link_id->egress_port_id = flow_info->egress_ports[i];
                link_id->egress_switch_id = flow_info->switch_ids[i];

                uint32_t* link_latency = hash_map_get(ctx->link_map, link_id,
                                                      sizeof(link_id), &val_len);
                if (link_latency != NULL) {
                    if ( ABS(*link_latency, flow_info->link_latencies[i])
                         > SW_LATENCY_THRESHOLD ) {
                        printf("new link_latency: %u\n\n", flow_info->link_latencies[i]);
#ifndef INFLUXDB_EXPORTER
                        graphite_send_link(link_id, *link_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                        influxdb_send_link(curl, link_id, *link_latency, &metric_timestamp);
#endif
                    }

                    link_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *link_latency = flow_info->link_latencies[i];
                    hash_map_insert_or_replace(ctx->link_map, link_id, sizeof(link_id),
                            link_latency, sizeof(uint32_t));
                    free(link_id);
                
                } else {
                    printf("new link: %u\n", flow_info->link_latencies[i]);
                    link_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *link_latency = flow_info->link_latencies[i];
                    hash_map_insert_or_replace(ctx->link_map, link_id, sizeof(link_id),
                            link_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                    graphite_send_link(link_id, *link_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_link(curl, link_id, *link_latency, &metric_timestamp);
#endif
                    // insert to list for periodic db update
                    list_insert(ctx->link_id_list, link_id, sizeof(link_id));
                }
            }


            // check queue occupancy
            for (i=0; i < flow_info->hop_cnt; ++i) {
                queue_id* q_id = malloc(sizeof(queue_id));
                q_id->switch_id = flow_info->switch_ids[i];
                q_id->queue_id = flow_info->queue_ids[i];

                uint32_t* q_occup = hash_map_get(ctx->queue_map, q_id,
                                                      sizeof(queue_id), &val_len);
                if (q_occup != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*q_occup, flow_info->queue_occups[i])
                         > Q_OCCUP_THRESHOLD) ) {

                        printf("new q_occup: %u\n\n", flow_info->queue_occups[i]);
#ifndef INFLUXDB_EXPORTER
                        graphite_send_queue(q_id, *q_occup);
#endif
#ifdef INFLUXDB_EXPORTER
                        influxdb_send_queue(curl, q_id, *q_occup, &metric_timestamp);
#endif
                    }
                    q_occup = (uint32_t*) malloc(sizeof(uint32_t));
                    *q_occup = flow_info->queue_occups[i];
                    hash_map_insert_or_replace(ctx->queue_map, q_id, sizeof(queue_id),
                            q_occup, sizeof(uint32_t));
                    free(q_id);

                } else {
                    printf("new queue: %u\n", flow_info->queue_occups[i]);
                    q_occup = (uint32_t*) malloc(sizeof(uint32_t));
                    *q_occup = flow_info->queue_occups[i];
                    hash_map_insert_or_replace(ctx->queue_map, q_id, sizeof(queue_id),
                            q_occup, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                    graphite_send_queue(q_id, *q_occup);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_queue(curl, q_id, *q_occup, &metric_timestamp);
#endif
                    // insert to list for periodic db update
                    list_insert(ctx->queue_id_list, q_id, sizeof(queue_id));
                }
            }

#ifdef INFLUXDB_EXPORTER
            curl_easy_cleanup(curl);
#endif

            free(flow_info);
        }
    }

    printf("exporter exiting...\n");
    pthread_exit(0);
}

/****************** PERIODIC EXPORTER ******************/
void* periodic_exporter(void* args) {
    Context* ctx = (Context*) args;
    while(!*ctx->terminate) {
#ifdef INFLUXDB_EXPORTER
        sleep(1);
#else
        sleep(20);
#endif

#ifdef INFLUXDB_EXPORTER
        CURL *curl;
        CURLcode res;
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, INFLUXDB_ADDR);
        struct timespec metric_timestamp;
        clock_gettime(CLOCK_REALTIME, &metric_timestamp);
#endif

        list_el* fid = ctx->flow_id_list->first;
        list_el* sid = ctx->switch_id_list->first;
        list_el* lid = ctx->link_id_list->first;
        list_el* qid = ctx->queue_id_list->first;
        int val_len;
        while(fid != NULL) {
            uint32_t* flow_latency =
                hash_map_get(ctx->flow_map, fid->data, fid->len, &val_len);
            if (flow_latency == NULL) {
                perror("error wrong key in flow list\n");
                break;
            }
#ifndef INFLUXDB_EXPORTER
            graphite_send_flow(fid->data, *flow_latency);
#endif
#ifdef INFLUXDB_EXPORTER
            influxdb_send_flow(curl, fid->data, *flow_latency, &metric_timestamp);
#endif
            fid = fid->next;
        }

        while(sid != NULL) {
            uint32_t* switch_latency =
                hash_map_get(ctx->switch_map, sid->data, sid->len, &val_len);
            if (switch_latency == NULL) {
                perror("error wrong key in switch list\n");
                break;
            }
#ifndef INFLUXDB_EXPORTER
            graphite_send_switch(sid->data, *switch_latency);
#endif
#ifdef INFLUXDB_EXPORTER
            influxdb_send_switch(curl, sid->data, *switch_latency, &metric_timestamp);
#endif
            sid = sid->next;
        }

        while(lid != NULL) {
            uint32_t* link_latency =
                hash_map_get(ctx->link_map, lid->data, lid->len, &val_len);
            if (link_latency == NULL) {
                perror("error wrong key in link list\n");
                break;
            }
#ifndef INFLUXDB_EXPORTER
            graphite_send_link(lid->data, *link_latency);
#endif
#ifdef INFLUXDB_EXPORTER
            influxdb_send_link(curl, lid->data, *link_latency, &metric_timestamp);
#endif
            lid = lid->next;
        }

        while(qid != NULL) {
            uint32_t* q_occup =
                hash_map_get(ctx->queue_map, qid->data, qid->len, &val_len);
            if (q_occup == NULL) {
                perror("error wrong key in queue list\n");
                break;
            }
#ifndef INFLUXDB_EXPORTER
            graphite_send_queue(qid->data, *q_occup);
#endif
#ifdef INFLUXDB_EXPORTER
            influxdb_send_queue(curl, qid->data, *q_occup, &metric_timestamp);
#endif
            qid = qid->next;
        }
#ifdef INFLUXDB_EXPORTER
        curl_easy_cleanup(curl);
#endif
    }
    pthread_exit(0);
}
/* uint32_t to IPv4 string */
char* ip2str(uint32_t ipaddr, char* ipstr) {
    uint8_t* ipbytes = (uint8_t*)&ipaddr;
    snprintf(ipstr, 16, "%hhu.%hhu.%hhu.%hhu",
             ipbytes[3], ipbytes[2], ipbytes[1], ipbytes[0]);
    return ipstr;
}

/************** syslog ***************/
int syslog_send(char* ipaddr, int priority, char* host, char* process, char* msg) {
    int sockfd; 
    char buffer[512] = {0}; 
    struct sockaddr_in servaddr; 

    time_t rawtime;
    time(&rawtime);
    char timestr[64] = {0};
    strftime(timestr, 64, "%b %d %H:%M:%S", localtime(&rawtime));
    snprintf(buffer, 512, "<%i>%s %s %s:%s",
        priority, timestr, host, process, msg);
  
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(514); 
    servaddr.sin_addr.s_addr = inet_addr(ipaddr); 
      
    sendto(sockfd, (const char *)buffer, strlen(buffer), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    close(sockfd); 
    return 0; 
}