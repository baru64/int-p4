#ifndef GRAPHITE_H
#define GRAPHITE_H

#define GRAPHITE_ADDR "10.0.128.1"
#define GRAPHITE_PORT 2003

int graphite_send(char* path, uint32_t value);
int graphite_send_flow(flow_id* fid, uint32_t value);
int graphite_send_switch(switch_id* sid, uint32_t value);
int graphite_send_link(link_id* lid, uint32_t value);
int graphite_send_queue(queue_id* qid, uint32_t value);

#endif
