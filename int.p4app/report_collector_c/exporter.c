#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "util.h"
#include "parser.h"
#include "graphite.h"
#include "exporter.h"

#define INFLUXDB_ADDR "http://10.0.128.1:8086/write?db=int"

void* report_exporter(void* args) {
    Context* ctx = (Context*) args;

    printf("exporter starts!\n");
    while(!*ctx->terminate) {
        int info_size;
        flow_info_t* flow_info = dequeue_pop(ctx->exporter_dq, &info_size);
        if (flow_info != NULL) {
            printf("exporter got report src ip,port %x:%u\n",
                    flow_info->src_ip, flow_info->src_port);
            
#ifdef INFLUXDB_EXPORTER
            CURL *curl;
            CURLcode res;
            curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL, INFLUXDB_ADDR);

            struct timespec metric_timestamp;
            clock_gettime(CLOCK_REALTIME, &metric_timestamp);
#endif


            // check flow latency
            flow_id fid = {
                .src_ip = flow_info->src_ip,
                .dst_ip = flow_info->dst_ip,
                .src_port = flow_info->src_port,
                .dst_port = flow_info->dst_port,
                .protocol = flow_info->protocol
            };
            int val_len;
            uint32_t* flow_latency = hash_map_get(ctx->flow_map, &fid,
                                                  sizeof(flow_id), &val_len);
            if (flow_latency != NULL) {
                if ( val_len == sizeof(uint32_t) && 
                     (ABS(*flow_latency, flow_info->flow_latency)
                     > LATENCY_THRESHOLD) ) {

                    printf("new flow_latency: %u\n\n", flow_info->flow_latency);
                    flow_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *flow_latency = flow_info->flow_latency;
                    hash_map_insert_or_replace(ctx->flow_map, &fid, sizeof(flow_id),
                            flow_latency, sizeof(uint32_t));
                    
                    // send data to data base
#ifndef INFLUXDB_EXPORTER
                    graphite_send_flow(&fid, *flow_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_flow(curl, &fid, *flow_latency, &metric_timestamp);
#endif
                }
            } else {
                printf("new flow: %u\n", flow_info->flow_latency);
                flow_latency = (uint32_t*) malloc(sizeof(uint32_t));
                *flow_latency = flow_info->flow_latency;
                hash_map_insert_or_replace(ctx->flow_map, &fid, sizeof(flow_id),
                        flow_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                graphite_send_flow(&fid, *flow_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                influxdb_send_flow(curl, &fid, *flow_latency, &metric_timestamp);
#endif
            }
            // check switch latency
            int i;
            for (i=0; i < flow_info->hop_cnt; ++i) {
                switch_id sw_id = {
                    .switch_id = flow_info->switch_ids[i]
                };
                uint32_t* sw_latency = hash_map_get(ctx->switch_map, &sw_id,
                                                      sizeof(switch_id), &val_len);
                if (sw_latency != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*sw_latency, flow_info->hop_latencies[i])
                         > SW_LATENCY_THRESHOLD) ) {

                        printf("new sw_latency: %u\n\n", flow_info->hop_latencies[i]);
                        sw_latency = (uint32_t*) malloc(sizeof(uint32_t));
                        *sw_latency = flow_info->hop_latencies[i];
                        hash_map_insert_or_replace(ctx->switch_map, &sw_id, sizeof(switch_id),
                                sw_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                        graphite_send_switch(&sw_id, *sw_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                        influxdb_send_switch(curl, &sw_id, *sw_latency, &metric_timestamp);
#endif
                    }
                } else {
                    printf("new switch: %u\n", flow_info->hop_latencies[i]);
                    sw_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *sw_latency = flow_info->hop_latencies[i];
                    hash_map_insert_or_replace(ctx->switch_map, &sw_id, sizeof(switch_id),
                            sw_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                    graphite_send_switch(&sw_id, *sw_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_switch(curl, &sw_id, *sw_latency, &metric_timestamp);
#endif
                }
            }

            // check link latency
            for (i=0; i < (flow_info->hop_cnt-1); ++i) {
                link_id link_id = {
                    .ingress_port_id = flow_info->ingress_ports[i+1],
                    .ingress_switch_id = flow_info->switch_ids[i+1],
                    .egress_port_id = flow_info->egress_ports[i],
                    .egress_switch_id = flow_info->switch_ids[i]
                };
                uint32_t* link_latency = hash_map_get(ctx->link_map, &link_id,
                                                      sizeof(link_id), &val_len);
                if (link_latency != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*link_latency, flow_info->link_latencies[i])
                         > SW_LATENCY_THRESHOLD) ) {

                        printf("new link_latency: %u\n\n", flow_info->link_latencies[i]);
                        link_latency = (uint32_t*) malloc(sizeof(uint32_t));
                        *link_latency = flow_info->link_latencies[i];
                        hash_map_insert_or_replace(ctx->link_map, &link_id, sizeof(link_id),
                                link_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                        graphite_send_link(&link_id, *link_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                        influxdb_send_link(curl, &link_id, *link_latency, &metric_timestamp);
#endif
                    }
                } else {
                    printf("new link: %u\n", flow_info->link_latencies[i]);
                    link_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *link_latency = flow_info->link_latencies[i];
                    hash_map_insert_or_replace(ctx->link_map, &link_id, sizeof(link_id),
                            link_latency, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                    graphite_send_link(&link_id, *link_latency);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_link(curl, &link_id, *link_latency, &metric_timestamp);
#endif
                }
            }

            // check queue occupancy
            for (i=0; i < flow_info->hop_cnt; ++i) {
                queue_id q_id = {
                    .switch_id = flow_info->switch_ids[i],
                    .queue_id = flow_info->queue_ids[i]
                };
                uint32_t* q_occup = hash_map_get(ctx->queue_map, &q_id,
                                                      sizeof(queue_id), &val_len);
                if (q_occup != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*q_occup, flow_info->queue_occups[i])
                         > Q_OCCUP_THRESHOLD) ) {

                        printf("new q_occup: %u\n\n", flow_info->queue_occups[i]);
                        q_occup = (uint32_t*) malloc(sizeof(uint32_t));
                        *q_occup = flow_info->queue_occups[i];
                        hash_map_insert_or_replace(ctx->queue_map, &q_id, sizeof(queue_id),
                                q_occup, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                        graphite_send_queue(&q_id, *q_occup);
#endif
#ifdef INFLUXDB_EXPORTER
                        influxdb_send_queue(curl, &q_id, *q_occup, &metric_timestamp);
#endif
                    }
                } else {
                    printf("new queue: %u\n", flow_info->queue_occups[i]);
                    q_occup = (uint32_t*) malloc(sizeof(uint32_t));
                    *q_occup = flow_info->queue_occups[i];
                    hash_map_insert_or_replace(ctx->queue_map, &q_id, sizeof(queue_id),
                            q_occup, sizeof(uint32_t));
#ifndef INFLUXDB_EXPORTER
                    graphite_send_queue(&q_id, *q_occup);
#endif
#ifdef INFLUXDB_EXPORTER
                    influxdb_send_queue(curl, &q_id, *q_occup, &metric_timestamp);
#endif
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

// TODO PERIODIC EXPORTER
void* periodic_exporter(void* args) {
    pthread_exit(0);
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

/******** influxdb functions *********/
CURLcode influxdb_send(CURL* curl, char* poststr) {
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, poststr);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    return res;
}

CURLcode influxdb_send_flow(CURL* curl, flow_id* fid, uint32_t value,
                       struct timespec* tstamp) {
    char poststr[256] = {0};
    snprintf(poststr, 256, "flow_latency,src_ip=%u,dst_ip=%u,src_port=%hu," \
                            "dst_port=%hu,protocol=%hhu value=%u %li%li",
                fid->src_ip, fid->dst_ip, fid->src_port, fid->dst_port,
                fid->protocol, value, tstamp->tv_sec, tstamp->tv_nsec);
    return influxdb_send(curl, poststr);
}
CURLcode influxdb_send_switch(CURL* curl, switch_id* sid, uint32_t value,
                         struct timespec* tstamp) {
    char poststr[256] = {0};
    snprintf(poststr, 256, "switch_latency,switch_id=%u value=%u %li%li",
                    sid->switch_id, value, tstamp->tv_sec, tstamp->tv_nsec);
    return influxdb_send(curl, poststr);
}
CURLcode influxdb_send_link(CURL* curl, link_id* lid, uint32_t value,
                         struct timespec* tstamp) {
    char poststr[256] = {0};
    snprintf(poststr, 256, "link_latency,ingress_port_id=%u,ingress_switch_id=%u," \
                         "egress_port_id=%u,egress_switch_id=%u value=%u %li%li",
                lid->ingress_port_id, lid->ingress_switch_id,
                lid->egress_port_id, lid->egress_switch_id,
                value, tstamp->tv_sec, tstamp->tv_nsec);
    return influxdb_send(curl, poststr);
}
CURLcode influxdb_send_queue(CURL* curl, queue_id* qid, uint32_t value,
                         struct timespec* tstamp) {
    char poststr[256] = {0};
    snprintf(poststr, 256, "queue_occupancy,switch_id=%u,queue_id=%hhu " \
                            "value=%u %li%li",
                qid->switch_id, qid->queue_id, value,
                tstamp->tv_sec, tstamp->tv_nsec);
    return influxdb_send(curl, poststr);
}

/******** hash map implementation ********/

uint32_t djb2_hash(uint8_t* str, int strlen) {
	uint32_t hash = 5381;
    int c;
    int i;
    for(i = 0; i < strlen; i++) {
        c = str[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

int hash_map_init(hash_map* map, int len) {
    if (pthread_mutex_init(&map->mutex, NULL) != 0) return -1;
    map->entries = (hash_map_entry*) malloc(sizeof(hash_map_entry)*len);
    if (map->entries == NULL) return -1;
    map->len = len;
    return 0;
}

int hash_map_insert_or_replace(hash_map* map, void* key, int key_len,
                               void* value, int value_len) {
    pthread_mutex_lock(&map->mutex);

    uint32_t hash = djb2_hash((uint8_t*)key, key_len);
    uint32_t index = hash % map->len;
    if (map->entries[index].key != NULL) {
        // no collision handling, replace entry
        //free(map->entries[index].key);
        //free(map->entries[index].value);

        map->entries[index].key = key;
        map->entries[index].key_len = key_len;
        map->entries[index].value = value;
        map->entries[index].value_len = value_len;
    } else {
        map->entries[index].key = key;
        map->entries[index].key_len = key_len;
        map->entries[index].value = value;
        map->entries[index].value_len = value_len;
    }

    pthread_mutex_unlock(&map->mutex);
    return 0;
}

void* hash_map_remove(hash_map* map, void* key, int key_len, int* value_len) {
    pthread_mutex_lock(&map->mutex);

    uint32_t hash = djb2_hash((uint8_t*)key, key_len);
    uint32_t index = hash % map->len;
    void* ret = NULL;
    if (map->entries[index].key != NULL) {
        ret = map->entries[index].value;
        *value_len = map->entries[index].value_len;

        map->entries[index].key = NULL;
        map->entries[index].value = NULL;
    }

    pthread_mutex_unlock(&map->mutex);
    return ret;
}

void* hash_map_get(hash_map* map, void* key, int key_len, int* value_len) {
    pthread_mutex_lock(&map->mutex);
    
    uint32_t hash = djb2_hash((uint8_t*)key, key_len);
    uint32_t index = hash % map->len;
    void* ret = NULL;
    if (map->entries[index].key != NULL) {
        ret = map->entries[index].value;
        *value_len = map->entries[index].value_len;
    }
    
    pthread_mutex_unlock(&map->mutex);
    return ret;
}

void hash_map_free(hash_map* map) {
    int i;
    for(i=0; i < map->len; ++i) {
        if (map->entries[i].key != NULL) {
            free(map->entries[i].key);
            free(map->entries[i].value);
        }
    }
    free(map->entries);
    free(map);
    pthread_mutex_destroy(&map->mutex);
}
