#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "hash_map.h"

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