#ifndef CIRC_BUFFER_H
#define CIRC_BUFFER_h

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

typedef struct circular_buffer {
    int         read_p;
    int         write_p;
    uint8_t*    buffer;
    int         element_size;
    int         element_num;

    pthread_mutex_t rw_mutex;
} cbuff_buffer;

int cbuff_init(cbuff_buffer* buffer, int element_size, int element_num);
int cbuff_write(cbuff_buffer* buffer, uint8_t* data,int size);
int cbuff_read(cbuff_buffer*, uint8_t* data, int size);
void cbuff_free(cbuff_buffer* buffer);

#endif
