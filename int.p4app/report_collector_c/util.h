#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <signal.h>
#include "dequeue.h"

typedef struct Context {
    dequeue* dq;
    sig_atomic_t* terminate;
} Context;

#endif
