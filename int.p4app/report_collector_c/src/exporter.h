#ifndef EXPORTER_H
#define EXPORTER_H

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

int syslog_send(char* ipaddr, int priority,
    char* host, char* process, char* msg);
char* ip2str(uint32_t ipaddr, char* ipstr);

#endif
