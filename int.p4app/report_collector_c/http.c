#include "http.h"
int http_request(char* method, char* url, char* data, int datalen) {
    // TODO
	int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[512] = {0};
    snprintf(buffer, 512, "%s %u %li\n", path, value, time(0));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("exporter: socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(GRAPHITE_PORT);
    inet_pton(AF_INET, GRAPHITE_ADDR, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("exporter: connection failed\n");
        return -1;
    }
    send(sockfd, buffer, strlen(buffer), 0);
    // TODO recv
    close(sockfd);
    printf("exporter: data sent\n");
    return 0; // TODO RETURN CODE
}
