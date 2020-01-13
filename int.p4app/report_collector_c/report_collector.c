#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define BUFFER_SIZE 512

struct telemetry_report_t {
    uint8_t     ver:4,
                length:4;
    uint16_t    nprot:3,
                rep_md_bits:6,
                rsvd:6,
                d:1;
    uint8_t     q:1,
                f:1,
                hw_id:6;
    uint32_t    switch_id;
    uint32_t    seq_num;
    uint32_t    ingress_tstamp;
} __attribute__((packed));

struct INT_shim_t {
    uint8_t type;
    uint8_t rsvd1;
    uint8_t length;
    uint8_t dscp:6, 
            rsvd2:2;
} __attribute__((packed));

struct INT_metadata_hdr_t {
    uint8_t     ver:4,
                rep:2,
                c:1,
                e:1;
    uint16_t    m:1,
                rsvd1:10,
                hop_ml:5;
    uint8_t     rem_hop_cnt;
    uint16_t    ins_map;
    uint16_t    rsvd2;
} __attribute__((packed));

struct INT_metadata_t {
    uint32_t data;
} __attribute__((packed));

int main(){
    int sock_fd, rx_size;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    struct sockaddr src_addr;
    int addrlen = sizeof src_addr;

    /*Create UDP socket*/
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    /*Configure server address struct*/
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9555);
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    /*Bind socket with address struct*/
    bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    while(1){
        memset(buffer, 0, BUFFER_SIZE);
        rx_size = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, &src_addr, &addrlen);
        printf("received: %d bytes, str: %s\n", rx_size, buffer);
    }

    return 0;
}