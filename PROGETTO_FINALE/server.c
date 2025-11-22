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
#include <pthread.h>
#include "inizializzazione_server.h"
#include "gestione_client.h"


int epollfd;
int main() {
    pthread_t thread_id;
    int sockfd, newsockfd, n, i;
    struct sockaddr_in cli_addr;
    socklen_t clilen_addrlen = sizeof(cli_addr);
    struct epoll_event ev, events[MAX_EVENTS];

    sockfd = init_server();

    // Creazione dell'epoll instance
    if((epollfd=epoll_create1(0))==-1){
        perror("creazione istanza epoll fallita");
        exit(EXIT_FAILURE);
    }
    
    // Aggiungiamo il socket principale all'epoll
    ev.events = EPOLLIN; // Evento di lettura
    ev.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Aspettiamo che un evento accada su uno dei file descriptor
        n = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (n == -1) {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }

        // Gestiamo gli eventi
        for (i = 0; i < n; i++) {
            if (events[i].data.fd == sockfd) {
                // Nuova connessione in arrivo
                if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen_addrlen)) < 0) {
                    perror("accept failed");
                    continue;
                }

                // Impostiamo il nuovo socket come non bloccante
                set_nonblocking(newsockfd);

                // Aggiungiamo il nuovo socket all'epoll
                ev.events = EPOLLIN | EPOLLET; // Evento di lettura, modalità edge-triggered
                ev.data.fd = newsockfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newsockfd, &ev) == -1) {
                    perror("epoll_ctl failed");
                    exit(EXIT_FAILURE);
                }

                printf("Nuova connessione, socket fd = %d\n", newsockfd);
            } else {
                int sd = events[i].data.fd;
                int* arg = malloc(sizeof(int));
                *arg = sd; //significa: “vai nella zona di memoria puntata da arg e scrivi lì dentro”, “Dentro la memoria puntata da arg, memorizza il numero sd
                pthread_create(&thread_id, NULL, thread_function, arg);
            }
        }
    }


    return 0;
}