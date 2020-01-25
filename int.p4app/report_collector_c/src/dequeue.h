#ifndef DEQUEUE_H
#define DEQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct dequeue_el {
    void*   next;
    void*   data;
    int     len;
} dequeue_el;

typedef struct dequeue {
    pthread_mutex_t mutex;
    sem_t           sem;
    dequeue_el*     first;
    dequeue_el*     last;
} dequeue;

int     dequeue_init(dequeue* dq);
void    dequeue_push(dequeue* dq, void* data, int len);
void*   dequeue_pop(dequeue* dq, int* size);
void    dequeue_free(dequeue* dq);

#endif
