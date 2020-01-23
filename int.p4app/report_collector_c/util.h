#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <signal.h>
#include "dequeue.h"
#include "exporter.h"

typedef struct Context {
    dequeue* parser_dq;
    dequeue* exporter_dq;
    hash_map* flow_map;
    hash_map* switch_map;
    hash_map* link_map;
    hash_map* queue_map;
    sig_atomic_t* terminate;
} Context;

#define ABS(a,b) ((a>b)? (a-b):(b-a))

#endif
