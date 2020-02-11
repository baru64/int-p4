#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <pthread.h>
#include <stdint.h>

typedef struct hash_map_entry {
    int     key_len;
    int     value_len;
    void*   key;
    void*   value;
} hash_map_entry;


typedef struct hash_map {
    pthread_mutex_t mutex;

    hash_map_entry* entries;
    int             len;
} hash_map;

uint32_t djb2_hash(uint8_t* str, int strlen);

int hash_map_init(hash_map* map, int len);
int hash_map_insert_or_replace(hash_map* map, void* key, int key_len, void* value, int value_len);
void* hash_map_remove(hash_map* map, void* key, int key_len, int* value_len);
void* hash_map_get(hash_map* map, void* key, int key_len, int* value_len);
void hash_map_free(hash_map* map);

#endif