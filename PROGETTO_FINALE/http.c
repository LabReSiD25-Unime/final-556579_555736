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
#include <strings.h>
#include "http.h"
#include "mutex.h"
pthread_mutex_t mutex_globale = PTHREAD_MUTEX_INITIALIZER;

/*Header consigliati

Request:

Host (obbligatorio)

Content-Length (se body)

Content-Type (se body)

User-Agent (opzionale)

Authorization (opzionale)
++++++++++++++++++++++++++++++++++++++++++++
Response:

Content-Type

Content-Length

Date

server
*/
//restituisce l'header richiesto come parametro
const char *get_header_value(http_request_t *req, const char *header_name) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, header_name) == 0) //ignora differenze tra maiuscolo e minuscolo (RFC lo dice chiaramente)
            return req->headers[i].value;
    }
    return NULL; // non trovato
}

void build_path(char *dest, size_t size, const char *req_path) {
    if (strcmp(req_path, "/") == 0)
        snprintf(dest, size, "./www/index.html");
    else
        snprintf(dest, size, "./www%s", req_path);
}

//controlla se la risorsa è protetta
int risorsa_protetta(const char *path) {
    // Tutti i file nella cartella /private/ richiedono autorizzazione
    if (strstr(path, "/private/") != NULL)
        return 1;
    return 0; // tutto il resto è libero
}

//controlla se l'header Authorization contiene il token corretto
int autorizzazione_valida(http_request_t *req) {
    const char *auth = get_header_value(req, "Authorization");
    if (!auth) return 0; // manca header

    const char *prefix = "Bearer ";
    size_t plen = strlen(prefix);

    //deve iniziare con "Bearer "
    if (strncasecmp(auth, prefix, plen) != 0)
        return 0;

    //estraggo il token vero e proprio
    const char *token = auth + plen;

    //confronto col token che abbiamo deciso di accettare
    if (strcmp(token, "secret123") == 0)
        return 1; // valido
    else
        return 0; // sbagliato
}


char *get_html_file(const char* path) {

	//legge i contenuti di un documento html 

	int fd;
	char *body;
	int bytes_read, total_read = 0;
    
	fd = open(path, O_RDONLY);

	//se non trova il documento, ritorna del testo html minimo per segnalare il problema 
	if (fd < 0) {
		return strdup("<html><body><h1>404 Not Found</h1></body></html>");
	}

	body = malloc(MAX_FILE_SIZE + 1);

	//leggi il documento a pezzi
	while (total_read < MAX_FILE_SIZE) {
		bytes_read = read(fd, body + total_read, MAX_FILE_SIZE - total_read);
		if (bytes_read <= 0) break; 
		total_read += bytes_read;
	}
	close(fd);

	body[total_read] = '\0';
	return body;

}


int parse_http_request(const char *request, http_request_t *req){
    memset(req, 0, sizeof(http_request_t));

    //Estrai la prima linea (es. "GET /index.html HTTP/1.1")
    // CONTROLLARE SE BAD REQUEST
    if (sscanf(request, "%s %s %s", req->method, req->path, req->version) != 3) return -1;


    //controllo metodo valido in caso bad request
    if (strcmp(req->method, "GET") != 0 &&
        strcmp(req->method, "POST") != 0 &&
        strcmp(req->method, "PUT") != 0 &&
        strcmp(req->method, "DELETE") != 0) {
        return -1; // Metodo non supportato
    }

    //controllo path valido
    if (req->path[0] != '/')
        return -1; // Path non valido


    //strstr restituisce l'indirizzo della prima posizione di inizio sotto-stringa
    const char *header_start = strstr(request, "\r\n");
    if (!header_start) return -1;
    //avanziamo di 2 per saltare la new line e l'andata a capo
    header_start += 2;
    const char *ptr = header_start;

    //cicla sugli header fino a \r\n\r\n
    while (req->header_count < MAX_HEADERS && strncmp(ptr, "\r\n", 2) != 0) {
        char name[MAX_HEADER_NAME];
        char value[MAX_HEADER_VALUE];
        if (sscanf(ptr, "%[^:]: %[^\r\n]", name, value) == 2) {
            strcpy(req->headers[req->header_count].name, name);
            strcpy(req->headers[req->header_count].value, value);
            req->header_count++;
        }
        ptr = strstr(ptr, "\r\n");
        if (!ptr) break;
        ptr += 2;
    }

    
    //cerca Content-Length (se esiste)
    int content_length = 0;
    const char *cl = get_header_value(req, "Content-Length");
    if (cl)
        content_length = atoi(cl); // sicuro, converte da stringa a int


    const char *body_start = strstr(request, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        req->body = malloc(content_length + 1);
        memcpy(req->body, body_start, content_length);
        req->body[content_length] = '\0';  // lo aggiungiamo noi
        req->body_length = content_length;
    }

    return 0;
}



void send_response(int client_fd, const char *status, const char *content_type, const char *body) {
    char response[1024];
    size_t body_len = body ? strlen(body) : 0;

    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             status,
             content_type,
             body_len,
             body ? body : "");

    send(client_fd, response, strlen(response), 0);
    //close(client_fd);
}

void send_bad_request(int client_fd) {
    send_response(client_fd, "400 Bad Request", "text/plain", "400 Bad Request\n");
}

void send_unauthorized(int client_fd) {
    send_response(client_fd, "401 Unauthorized", "text/plain", "401 Unauthorized\n");
}

void send_forbidden(int client_fd) {
    send_response(client_fd, "403 Forbidden", "text/plain", "403 Forbidden\n");
}

void send_created(int client_fd) {
    send_response(client_fd, "201 Created", "text/plain", "201 Created\n");
}

void send_ok(int client_fd, const char *body) {
    send_response(client_fd, "200 OK", "text/html", body);
}

void send_no_content(int client_fd) {
    send_response(client_fd, "204 No Content", "text/plain", "");
}

void send_conflict(int client_fd) {
    send_response(client_fd, "409 Conflict", "text/plain", "409 Conflict: resource already exists\n");
}

void gestione_metodo(int client_fd, http_request_t* req ){
    char filepath[MAX_PATH_LEN];

    // Gestione metodi
    if (strcmp(req->method, "GET") == 0) {
        // Esempio di risposta
        build_path(filepath, sizeof(filepath), req->path);

        if (risorsa_protetta(req->path)) {
            if (!autorizzazione_valida(req)) {
                send_unauthorized(client_fd);
            return;
            }
        }
        
        char* content_body = get_html_file(filepath);
        send_ok(client_fd, content_body);
        free(content_body);



    } else if (strcmp(req->method, "POST") == 0) {
        // Controlla header obbligatori
        const char *ctype = get_header_value(req, "Content-Type");
        const char *cl = get_header_value(req, "Content-Length");
        if (!ctype || !cl) {
            send_bad_request(client_fd);
            return;
        }


        if (risorsa_protetta(req->path)) {
            if (!autorizzazione_valida(req)) {
                send_unauthorized(client_fd);
            return;
            }
        }
        
        build_path(filepath, sizeof(filepath), req->path);
        // Verifica estensione
        if (!strstr(filepath, ".txt")) {
            send_bad_request(client_fd);
            return;
        }



         // Se il file esiste già -> 409 Conflict
        if (access(filepath, F_OK) == 0) {
            send_conflict(client_fd);
            return;
        }



        // Crea e scrivi il file
        int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Errore apertura file POST");
            send_response(client_fd, "500 Internal Server Error", "text/plain", "Impossibile creare file\n");
            return;
        }
        
       
        write(fd, req->body, req->body_length);
        close(fd);

    // Conferma creazione
    send_created(client_fd);

    } else if (strcmp(req->method, "PUT") == 0) {
        // PUT simile a POST
        pthread_mutex_lock(&mutex_globale);        //entra in sezione critica
        const char *ctype = get_header_value(req, "Content-Type");
        if (!ctype) {
            send_bad_request(client_fd);
            pthread_mutex_unlock(&mutex_globale); //esce dalla sezione critica
            return;
        }


        if (risorsa_protetta(req->path)) {
            if (!autorizzazione_valida(req)) {
                send_unauthorized(client_fd);
                pthread_mutex_unlock(&mutex_globale); //esce dalla sezione critica
            return;
            }
        }

        char filepath[256];
        build_path(filepath, sizeof(filepath), req->path);

        // Accetta solo file .txt
        if (!strstr(filepath, ".txt")) {
            send_bad_request(client_fd);
            pthread_mutex_unlock(&mutex_globale); //esce dalla sezione critica
            return;
        }
        int scelta=1;
        
        //controllo per verede se crea o sovrascrive il file
        if (access(filepath, F_OK) == 0)  scelta=0;

    
        int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        
        if (fd < 0) {
            perror("Errore apertura file PUT");
            send_response(client_fd, "500 Internal Server Error", "text/plain", "Errore creazione o aggiornamento file\n");
            pthread_mutex_unlock(&mutex_globale); //esce dalla sezione critica
            return;
        }
        // Risposta: 200 OK se aggiornato, 201 Created se nuovo
        ssize_t n= write(fd, req->body, req->body_length);
        

        if ( n == -1) {
            perror("write failed");
            send_response(client_fd, "500 Internal Server Error", "text/plain", "Errore di scrittura nel file\n");

        } else {
            if(scelta==1){
                send_created(client_fd);
            } else if(scelta==0){
                send_response(client_fd, "200 OK", "text/plain", "File aggiornato con successo\n");
            }
        }
        pthread_mutex_unlock(&mutex_globale); //esce dalla sezione critica
        close(fd);

    } else if (strcmp(req->method, "DELETE") == 0) {
        pthread_mutex_lock(&mutex_globale); //entra in sezione critica 
        // Controllo autorizzazione (es. per file riservati)
        if (risorsa_protetta(req->path)) {
            if (!autorizzazione_valida(req)) {
                send_unauthorized(client_fd);
                pthread_mutex_unlock(&mutex_globale); //esce dalla sezione critica
            return;
            }
        }
         // Costruisci percorso file
        char filepath[256];
        build_path(filepath, sizeof(filepath), req->path);

        // Cancella il file
        if (unlink(filepath) == 0) {
            send_no_content(client_fd); // 204 No Content
        } else {
            perror("Errore DELETE");
            send_response(client_fd, "404 Not Found", "text/plain", "File non trovato o impossibile da eliminare\n");
        }
        pthread_mutex_unlock(&mutex_globale); //esce dalla sezione critica
    } else {
        send_bad_request(client_fd);
    }
   // close(client_fd);
}


void gestione_request(int client_fd, const char *request) {
    http_request_t req;
    if (parse_http_request(request, &req) < 0) {
        send_bad_request(client_fd);
        return;
    }

    // Controllo header Host (obbligatorio HTTP/1.1)
    const char *host = get_header_value(&req, "Host");
    if (!host) {
        send_bad_request(client_fd);
        return;
    }
    gestione_metodo(client_fd, &req);
    
}

/*
POST    Create    Crea una nuova risorsa sul server (es. nuovo utente, nuovo file, nuovo record).
PUT    Update    Aggiorna o sostituisce una risorsa esistente
*/
