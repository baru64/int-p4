#ifndef EXPORTER_H
#define EXPORTER_H

#include <curl/curl.h>

typedef struct hash_map_entry {
    int     key_len;
    int     value_len;
    void*   key;
    void*   value;
} hash_map_entry;


typedef struct hash_map {
    pthread_mutex_t mutex;

    hash_map_entry* entries;
    int             len;
} hash_map;

uint32_t djb2_hash(uint8_t* str, int strlen);

int hash_map_init(hash_map* map, int len);
int hash_map_insert_or_replace(hash_map* map, void* key, int key_len, void* value, int value_len);
void* hash_map_remove(hash_map* map, void* key, int key_len, int* value_len);
void* hash_map_get(hash_map* map, void* key, int key_len, int* value_len);
void hash_map_free(hash_map* map);

#define LATENCY_THRESHOLD 50
#define SW_LATENCY_THRESHOLD 5
#define Q_OCCUP_THRESHOLD 1

typedef struct flow_id {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;
} flow_id;

typedef struct switch_id {
    uint32_t switch_id;
} switch_id;

typedef struct link_id {
    uint32_t ingress_port_id;
    uint32_t ingress_switch_id;
    uint32_t egress_port_id;
    uint32_t egress_switch_id;
} link_id;

typedef struct queue_id {
    uint32_t switch_id;
    uint8_t  queue_id;
} queue_id;


void* report_exporter(void* args);
void* periodic_exporter(void* args);

CURLcode influxdb_send(CURL* curl, char* poststr);
CURLcode influxdb_send_flow(CURL* curl, flow_id* fid, uint32_t value,
                                    struct timespec* tstamp);
CURLcode influxdb_send_switch(CURL* curl, switch_id* sid, uint32_t value,
                                    struct timespec* tstamp);
CURLcode influxdb_send_link(CURL* curl, link_id* lid, uint32_t value,
                                    struct timespec* tstamp);
CURLcode influxdb_send_queue(CURL* curl, queue_id* qid, uint32_t value,
                                    struct timespec* tstamp);

int syslog_send(char* ipaddr, int priority,
    char* host, char* process, char* msg);
char* ip2str(uint32_t ipaddr, char* ipstr);

#endif
