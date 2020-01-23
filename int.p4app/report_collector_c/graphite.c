#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include "exporter.h"

#include "graphite.h"

int graphite_send(char* path, uint32_t value) {
    int sockfd; 
    struct sockaddr_in serv_addr; 
    char buffer[512] = {0}; 
    snprintf(buffer, 512, "%s %u %li\n", path, value, time(0));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("exporter: socket creation error\n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(GRAPHITE_PORT); 
    inet_pton(AF_INET, GRAPHITE_ADDR, &serv_addr.sin_addr);
   
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("exporter: connection failed\n"); 
        return -1; 
    } 
    send(sockfd, buffer, strlen(buffer), 0); 
    close(sockfd);
    printf("exporter: data sent\n"); 
    return 0; 
}

int graphite_send_flow(flow_id* fid, uint32_t value) {
    char path[256] = {0};
    snprintf(path, 256, "int.flow_latency;src_ip=%u;dst_ip=%u;src_port=%hu;dst_port=%hu;protocol=%hhu",
                fid->src_ip, fid->dst_ip, fid->src_port, fid->dst_port, fid->protocol);
    return graphite_send(path, value);
}

int graphite_send_switch(switch_id* sid, uint32_t value) {
    char path[128] = {0};
    snprintf(path, 128, "int.switch_latency;switch_id=%u", sid->switch_id);
    return graphite_send(path, value);
}

int graphite_send_link(link_id* lid, uint32_t value) {
    char path[256] = {0};
    snprintf(path, 256, "int.link_latency;ingress_port_id=%u;ingress_switch_id=%u;" \
                         "egress_port_id=%u;egress_switch_id=%u",
                lid->ingress_port_id, lid->ingress_switch_id,
                lid->egress_port_id, lid->egress_switch_id);
    return graphite_send(path, value);
}

int graphite_send_queue(queue_id* qid, uint32_t value) {
    char path[256] = {0};
    snprintf(path, 256, "int.queue_occupancy;switch_id=%u;queue_id=%hhu",
                qid->switch_id, qid->queue_id);
    return graphite_send(path, value);
}
