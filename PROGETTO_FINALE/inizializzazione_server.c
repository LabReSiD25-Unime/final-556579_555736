#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include "inizializzazione_server.h"
// Funzione per inizializzare un server TCP e restituire il socket di ascolto

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int init_server() {
    int server_fd;
    struct sockaddr_in serv_addr;

    // Creazione del socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore creazione socket");
        exit(EXIT_FAILURE);
    }

    // Impostiamo il socket come non bloccante
    set_nonblocking(server_fd);
    
    // Configura l'indirizzo del server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Collega il socket alla porta
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Mette il socket in ascolto
    if (listen(server_fd, 128) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }


    printf("Server in ascolto sulla porta %d...\n", PORT);
    return server_fd;
}