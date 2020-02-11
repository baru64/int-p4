#ifndef INFLUXDB_H
#define INFLUXDB_H

#include <stdint.h>
#include <curl/curl.h>

CURLcode influxdb_send(CURL* curl, char* poststr);
CURLcode influxdb_send_flow(CURL* curl, flow_id* fid, uint32_t value,
                                    struct timespec* tstamp);
CURLcode influxdb_send_switch(CURL* curl, switch_id* sid, uint32_t value,
                                    struct timespec* tstamp);
CURLcode influxdb_send_link(CURL* curl, link_id* lid, uint32_t value,
                                    struct timespec* tstamp);
CURLcode influxdb_send_queue(CURL* curl, queue_id* qid, uint32_t value,
                                    struct timespec* tstamp);

#endif