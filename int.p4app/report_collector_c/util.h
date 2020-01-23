#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <signal.h>
#include "dequeue.h"
#include "exporter.h"
#include "list.h"

typedef struct Context {
    dequeue*    parser_dq;
    dequeue*    exporter_dq;
    hash_map*   flow_map;
    hash_map*   switch_map;
    hash_map*   link_map;
    hash_map*   queue_map;
    list*       flow_id_list;
    list*       switch_id_list;
    list*       link_id_list;
    list*       queue_id_list;
    sig_atomic_t* terminate;
} Context;

#define ABS(a,b) ((a>b)? (a-b):(b-a))

#endif
