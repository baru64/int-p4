#include "parser.h"

void* report_parser(void* args) {
    Context* ctx = (Context*) args;
    printf("parser starts!\n");
    while (!*ctx->terminate) {
        //printf("we in while\n");
        int packet_size;
        uint8_t* packet = dequeue_pop(ctx->dq, &packet_size);
        if (packet != NULL) {
            printf("parser got packet len,char: %i %c\n", packet_size, packet[0]);
            free(packet);
        }
    }
    printf("parser exiting\n");
    pthread_exit(0);
}
