#include "dequeue.h"

int dequeue_init(dequeue* dq) {
    if (pthread_mutex_init(&dq->mutex, NULL) != 0) return -1;
    if (sem_init(&dq->sem, 0, 0) != 0) return -1;
    dq->first = NULL;
    dq->last = NULL;
    return 0;
}

void dequeue_push(dequeue* dq, void* data, int len) {
    pthread_mutex_lock(&dq->mutex);
    
    dequeue_el* new_el = (dequeue_el*) malloc(sizeof(dequeue_el));
    new_el->next = NULL,
    new_el->data = data;
    new_el->len = len;

    if (dq->last != NULL) {
        dq->last->next = new_el; 
    }
    if (dq->first == NULL) {
        dq->first = new_el;
    }
    dq->last = new_el;

    pthread_mutex_unlock(&dq->mutex);
    sem_post(&dq->sem);
}

void* dequeue_pop(dequeue* dq, int* size) {
    if(sem_wait(&dq->sem) != 0) {
        printf("nonzero wait\n");
        return NULL;
    }
    pthread_mutex_lock(&dq->mutex);
   
    void* ret = NULL;
    if (dq->first != NULL) {
        *size = dq->first->len;
        ret = dq->first->data;
        dq->first = dq->first->next;
    } else {
        *size = 0;
    }

    pthread_mutex_unlock(&dq->mutex);
    return ret;
}

void dequeue_free(dequeue* dq) {
    void* el_data;
    while(dq->first != NULL) {
        dequeue_pop(dq, el_data);
        free(el_data);
    }
    pthread_mutex_destroy(&dq->mutex);
    sem_destroy(&dq->sem);
}
