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
#include "http.h"

void *thread_function(void* arg) {
   int sd = *(int* )arg;   // recupera il file descriptor passato
    char request[BUFFER_SIZE];
    int valore_letto;

    if((valore_letto = read(sd, request, sizeof(request))) > 0) {
        request[valore_letto] = '\0';
        printf("Ricevuto da %d: %s\n", sd, request);
        //send(sd, "Messaggio Ricevuto\n", 20, 0);
        gestione_request(sd, request);
    }
    else if (valore_letto == 0) {
        printf("Client disconnesso, socket FD=%d\n", sd);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, sd, NULL);
        close(sd);

    } else{ //tutto letto ma client ancora connesso
        perror("ERRORE");
        //epoll_ctl(epollfd, EPOLL_CTL_DEL, sd, NULL);
        //close(sd);
    }
    free(arg);
    pthread_exit(NULL);
}
