#ifndef INIT_H
#define INIT_H

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_EVENTS 512

void set_nonblocking(int sockfd);
int init_server();

#endif
