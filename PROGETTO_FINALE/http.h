#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_HEADERS 10
#define MAX_HEADER_NAME 64
#define MAX_HEADER_VALUE 256
#define MAX_METHOD_LEN 8
#define MAX_PATH_LEN 256
#define MAX_VERSION_LEN 8
#define MAX_FILE_SIZE 1024

// header HTTP
typedef struct {
    char name[MAX_HEADER_NAME];
    char value[MAX_HEADER_VALUE];
} http_header_t;

// Rappresenta una richiesta HTTP completa
typedef struct {
    char method[MAX_METHOD_LEN];       // GET, POST, PUT, DELETE...
    char path[MAX_PATH_LEN];           // /index.html
    char version[MAX_VERSION_LEN];     // HTTP/1.1
    http_header_t headers[MAX_HEADERS];// array di header
    int header_count;                  // quanti header effettivi
    char *body;                        // puntatore al corpo (per POST/PUT)
    size_t body_length;                // lunghezza del body
} http_request_t;

/*
// Rappresenta una risposta HTTP completa
typedef struct {
    int status_code;                   // 200, 201, 204, 400, 401...
    char status_text[64];              // "OK", "Created", ecc.
    http_header_t headers[MAX_HEADERS];// header di risposta
    int header_count;                  // quanti header
    char *body;                        // corpo della risposta
    size_t body_length;                // lunghezza del corpo
} http_response_t;
*/
// Funzione da implementare in http.c richiamata da gestione_client.c
void gestione_request(int client_fd, const char *request);

#endif
