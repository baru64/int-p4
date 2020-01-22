#include "exporter.h"
#include "graphite.h"

void* report_exporter(void* args) {
    Context* ctx = (Context*) args;

    hash_map flow_map; //TODO move to context
    if (hash_map_init(&flow_map, 1000000) != 0) {
        printf("error allocating hash map\n");
        pthread_exit(0);
    }
    hash_map switch_map;
    if (hash_map_init(&switch_map, 10000) != 0) {
        printf("error allocating hash map\n");
        pthread_exit(0);
    }
    hash_map link_map;
    if (hash_map_init(&link_map, 10000) != 0) {
        printf("error allocating hash map\n");
        pthread_exit(0);
    }
    hash_map queue_map;
    if (hash_map_init(&queue_map, 100000) != 0) {
        printf("error allocating hash map\n");
        pthread_exit(0);
    }

    printf("exporter starts!\n");
    while(!*ctx->terminate) {
        int info_size;
        flow_info_t* flow_info = dequeue_pop(ctx->exporter_dq, &info_size);
        if (flow_info != NULL) {
            printf("exporter got report src ip,port %x:%u\n",
                    flow_info->src_ip, flow_info->src_port);
            // check flow latency
            flow_id fid = {
                .src_ip = flow_info->src_ip,
                .dst_ip = flow_info->dst_ip,
                .src_port = flow_info->src_port,
                .dst_port = flow_info->dst_port,
                .protocol = flow_info->protocol
            };
            int val_len;
            uint32_t* flow_latency = hash_map_get(&flow_map, &fid,
                                                  sizeof(flow_id), &val_len);
            if (flow_latency != NULL) {
                if ( val_len == sizeof(uint32_t) && 
                     (ABS(*flow_latency, flow_info->flow_latency)
                     > LATENCY_THRESHOLD) ) {

                    printf("new flow_latency: %u\n\n", flow_info->flow_latency);
                    flow_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *flow_latency = flow_info->flow_latency;
                    hash_map_insert_or_replace(&flow_map, &fid, sizeof(flow_id),
                            flow_latency, sizeof(uint32_t));
                    graphite_send_flow(&fid, *flow_latency);
                }
            } else {
                printf("new flow: %u\n", flow_info->flow_latency);
                flow_latency = (uint32_t*) malloc(sizeof(uint32_t));
                *flow_latency = flow_info->flow_latency;
                hash_map_insert_or_replace(&flow_map, &fid, sizeof(flow_id),
                        flow_latency, sizeof(uint32_t));
                graphite_send_flow(&fid, *flow_latency);
            }
            // check switch latency
            int i;
            for (i=0; i < flow_info->hop_cnt; ++i) {
                switch_id sw_id = {
                    .switch_id = flow_info->switch_ids[i]
                };
                uint32_t* sw_latency = hash_map_get(&switch_map, &sw_id,
                                                      sizeof(switch_id), &val_len);
                if (sw_latency != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*sw_latency, flow_info->hop_latencies[i])
                         > SW_LATENCY_THRESHOLD) ) {

                        printf("new sw_latency: %u\n\n", flow_info->hop_latencies[i]);
                        sw_latency = (uint32_t*) malloc(sizeof(uint32_t));
                        *sw_latency = flow_info->hop_latencies[i];
                        hash_map_insert_or_replace(&switch_map, &sw_id, sizeof(switch_id),
                                sw_latency, sizeof(uint32_t));
                        graphite_send_switch(&sw_id, *sw_latency);
                    }
                } else {
                    printf("new switch: %u\n", flow_info->hop_latencies[i]);
                    sw_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *sw_latency = flow_info->hop_latencies[i];
                    hash_map_insert_or_replace(&switch_map, &sw_id, sizeof(switch_id),
                            sw_latency, sizeof(uint32_t));
                    graphite_send_switch(&sw_id, *sw_latency);
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
                uint32_t* link_latency = hash_map_get(&link_map, &link_id,
                                                      sizeof(link_id), &val_len);
                if (link_latency != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*link_latency, flow_info->link_latencies[i])
                         > SW_LATENCY_THRESHOLD) ) {

                        printf("new link_latency: %u\n\n", flow_info->link_latencies[i]);
                        link_latency = (uint32_t*) malloc(sizeof(uint32_t));
                        *link_latency = flow_info->link_latencies[i];
                        hash_map_insert_or_replace(&link_map, &link_id, sizeof(link_id),
                                link_latency, sizeof(uint32_t));
                        graphite_send_link(&link_id, *link_latency);
                    }
                } else {
                    printf("new link: %u\n", flow_info->link_latencies[i]);
                    link_latency = (uint32_t*) malloc(sizeof(uint32_t));
                    *link_latency = flow_info->link_latencies[i];
                    hash_map_insert_or_replace(&link_map, &link_id, sizeof(link_id),
                            link_latency, sizeof(uint32_t));
                    graphite_send_link(&link_id, *link_latency);
                }
            }

            // check queue occupancy
            for (i=0; i < flow_info->hop_cnt; ++i) {
                queue_id q_id = {
                    .switch_id = flow_info->switch_ids[i],
                    .queue_id = flow_info->queue_ids[i]
                };
                uint32_t* q_occup = hash_map_get(&queue_map, &q_id,
                                                      sizeof(queue_id), &val_len);
                if (q_occup != NULL) {
                    if ( val_len == sizeof(uint32_t) && 
                         (ABS(*q_occup, flow_info->queue_occups[i])
                         > Q_OCCUP_THRESHOLD) ) {

                        printf("new q_occup: %u\n\n", flow_info->queue_occups[i]);
                        q_occup = (uint32_t*) malloc(sizeof(uint32_t));
                        *q_occup = flow_info->queue_occups[i];
                        hash_map_insert_or_replace(&queue_map, &q_id, sizeof(queue_id),
                                q_occup, sizeof(uint32_t));
                        graphite_send_queue(&q_id, *q_occup);
                    }
                } else {
                    printf("new queue: %u\n", flow_info->queue_occups[i]);
                    q_occup = (uint32_t*) malloc(sizeof(uint32_t));
                    *q_occup = flow_info->queue_occups[i];
                    hash_map_insert_or_replace(&queue_map, &q_id, sizeof(queue_id),
                            q_occup, sizeof(uint32_t));
                    graphite_send_queue(&q_id, *q_occup);
                }
            }

            free(flow_info);
        }
    }
    printf("exporter exiting...\n");
    pthread_exit(0);
}

// TODO PERIODIC EXPORTER
void* periodic_exporter(void* args) {

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
