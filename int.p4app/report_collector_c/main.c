#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#include "util.h"
#include "dequeue.h"
#include "parser.h"
#include "exporter.h"

#define BUFFER_SIZE 512
#define PORT 9555
#define SERVER_ADDR "0.0.0.0"

dequeue packet_queue;
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
    pthread_t parser_thread;
    void* parser_result;
    //packet_queue = (dequeue*) malloc(sizeof(dequeue));
    Context parser_context = {&packet_queue, &terminate};

    struct sigaction action;
    action.sa_handler = sigterm_handler;
    //sigaction(SIGTERM, &action, NULL);
    //sigaction(SIGINT, &action, NULL);
  
    if (dequeue_init(&packet_queue) != 0) {
        perror("cannot create packet queue");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&parser_thread, NULL, report_parser,
                       (void*) &parser_context) != 0) {
        perror("cannot create parser thread");
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
            dequeue_push(&packet_queue, buffer, rx_size);
        }
    }

    pthread_join(parser_thread, &parser_result);
    dequeue_free(&packet_queue);
    return 0;
}
