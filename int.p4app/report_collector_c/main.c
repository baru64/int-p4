#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <curl/curl.h>

#include "dequeue.h"
#include "parser.h"
#include "exporter.h"
#include "util.h"

#define BUFFER_SIZE 512
#define PORT 9555
#define SERVER_ADDR "0.0.0.0"

dequeue parser_queue;
dequeue exporter_queue;
hash_map flow_map;
hash_map switch_map;
hash_map link_map;
hash_map queue_map;
list flow_id_list;
list switch_id_list;
list link_id_list;
list queue_id_list;
sig_atomic_t terminate = false;

void sigterm_handler(int signum) {
    terminate = true;
}

int main() {
    int sock_fd, rx_size;
    char* buffer;
    struct sockaddr_in server_addr;
    struct sockaddr src_addr;
    int addrlen = sizeof src_addr;
    
    // threads
    pthread_t parser_thread;
    void* parser_result;
    pthread_t exporter_thread;
    void* exporter_result;
    pthread_t periodic_exporter_thread;
    void* periodic_exporter_result;
    Context context = {
        &parser_queue,
        &exporter_queue,
        &flow_map,
        &switch_map,
        &link_map,
        &queue_map,
        &flow_id_list,
        &switch_id_list,
        &link_id_list,
        &queue_id_list,
        &terminate
    };

    struct sigaction action;
    action.sa_handler = sigterm_handler;
    action.sa_flags = 0;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

#ifdef INFLUXDB_EXPORTER
    curl_global_init(CURL_GLOBAL_ALL);
#endif

    // initiate queues
    if (dequeue_init(&parser_queue) != 0) {
        perror("cannot create parser queue");
        exit(EXIT_FAILURE);
    }
    
    if (dequeue_init(&exporter_queue) != 0) {
        perror("cannot create exporter queue");
        exit(EXIT_FAILURE);
    }
    
    // initiate hash maps
    if (hash_map_init(&flow_map, 1000000) != 0) {
        printf("error allocating hash map\n");
        exit(EXIT_FAILURE);
    }
    if (hash_map_init(&switch_map, 10000) != 0) {
        printf("error allocating hash map\n");
        exit(EXIT_FAILURE);
    }
    if (hash_map_init(&link_map, 10000) != 0) {
        printf("error allocating hash map\n");
        exit(EXIT_FAILURE);
    }
    if (hash_map_init(&queue_map, 100000) != 0) {
        printf("error allocating hash map\n");
        exit(EXIT_FAILURE);
    }


    // create threads
    if (pthread_create(&parser_thread, NULL, report_parser,
                       (void*) &context) != 0) {
        perror("cannot create parser thread");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&exporter_thread, NULL, report_exporter,
                       (void*) &context) != 0) {
        perror("cannot create exporter thread");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&periodic_exporter_thread, NULL, periodic_exporter,
                       (void*) &context) != 0) {
        perror("cannot create periodic exporter thread");
        exit(EXIT_FAILURE);
    }

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))
        == -1) {
        perror("cannot bind socket");
        exit(EXIT_FAILURE);
    }


    // MAIN LOOP
    while(!terminate) {
        buffer = (uint8_t*) malloc(sizeof(uint8_t)*BUFFER_SIZE);
        rx_size = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, &src_addr, &addrlen);
        if (rx_size > 0) {
            printf("received: %d bytes, first char: %c\n", rx_size, buffer[0]);
            dequeue_push(&parser_queue, buffer, rx_size);
        }
    }
    sem_post(&parser_queue.sem);
    sem_post(&exporter_queue.sem);
    printf("exiting...\n");
    close(sock_fd);
    pthread_join(parser_thread, &parser_result);
    pthread_join(exporter_thread, &exporter_result);
    pthread_join(periodic_exporter_thread, &periodic_exporter_result);
    dequeue_free(&parser_queue);
    dequeue_free(&exporter_queue);

#ifdef INFLUXDB_EXPORTER
    curl_global_cleanup();
#endif

    return 0;
}
