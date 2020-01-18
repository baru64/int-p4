#include "exporter.h"

void* report_exporter(void* args) {
    Context* ctx = (Context*) args;
    printf("exporter starts!\n");
    while(!*ctx->terminate) {
        int info_size;
        flow_info_t* flow_info = dequeue_pop(ctx->exporter_dq, &info_size);
        if (flow_info != NULL) {
            printf("parser got report src ip,port %x:%u\n",
                    flow_info->src_ip, flow_info->src_port);
            free(flow_info);
        }
    }
    printf("exporter exiting...\n");
    pthread_exit(0);
}
