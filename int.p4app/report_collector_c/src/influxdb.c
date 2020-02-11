#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <curl/curl.h>

#include "exporter.h"


CURLcode influxdb_send(CURL* curl, char* poststr) {
    CURLcode res;
#ifdef _DEBUG
    printf("%s\n", poststr);
#endif
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
    char ipstr[16] = {0};
    char ipstr2[16] = {0};
    snprintf(poststr, 256, "flow_latency,src_ip=%s,dst_ip=%s,src_port=%hu," \
                            "dst_port=%hu,protocol=%hhu value=%u %li%li",
                ip2str(fid->src_ip, ipstr), ip2str(fid->dst_ip, ipstr2),
                fid->src_port, fid->dst_port, fid->protocol, value,
                tstamp->tv_sec, tstamp->tv_nsec);
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