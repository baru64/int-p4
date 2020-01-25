#include "circ_buffer.h"

int cbuff_init(cbuff_buffer* buffer, int element_size, int element_num) {
    buffer->element_size = element_size;
    buffer->element_num = element_num;
    buffer->read_p = 0;
    buffer->write_p = 0;
    if (pthread_mutex_init(&(buffer->rw_mutex), NULL) != 0) {
        return -2;
    }
    buffer->buffer = (uint8_t*) malloc(element_num * element_size);
    if (buffer->buffer == NULL) return -1; 
    return 0;
}

int cbuff_write(cbuff_buffer* buffer, uint8_t* data,int size) {
    if (buffer->element_size < size) return -1;
    pthread_mutex_lock(&buffer->rw_mutex);
    memcpy(&buffer->buffer[buffer->write_p*buffer->element_size], data, size);
    buffer->write_p = (buffer->write_p + 1) % buffer->element_num;
    if (buffer->write_p == buffer->read_p) {
        buffer->read_p = (buffer->read_p + 1) % buffer->element_num;
    }
    pthread_mutex_unlock(&buffer->rw_mutex);
    return 0;
}

int cbuff_read(cbuff_buffer* buffer, uint8_t* data, int size) {
    pthread_mutex_lock(&buffer->rw_mutex);
    if (buffer->element_size > size) return -2;
    if (buffer->read_p == buffer->write_p) return -1;
    memcpy(data, &buffer->buffer[buffer->read_p*buffer->element_size],
           buffer->element_size);
    pthread_mutex_unlock(&buffer->rw_mutex);
    return 0;
}

void cbuff_free(cbuff_buffer* buffer) {
    free(buffer->buffer);
}
