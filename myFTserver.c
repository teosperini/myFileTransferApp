#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <semaphore.h>

#define BUFFER_SIZE 1024

sem_t dir_semaphore;


int server_socket; // dichiarare globalmente il server socket

void print_usage(const char *prog_name) {
    fprintf(stderr, "Uso: %s -d <directory> -a <indirizzo IP> -p <numero di porta> [-h]\n", prog_name);
}

// UTILITY
void print_string_ascii(const char *str) {
    int i = 0;
    while (str[i] != '\0') {
        printf("Carattere: %c, Codice ASCII: %d\n", str[i], str[i]);
        i++;
    }
}

void remove_crlf(char *str) {
    int i, j = 0;
    int len = strlen(str);

    for (i = 0; i < len; i++) {
        if (str[i] != '\r' && str[i] != '\n') {
            str[j++] = str[i];
        }
    }

    str[j] = '\0'; // Termina la stringa con il carattere null
}

// UTILITY END

int create_directories(const char *path, bool server_folder) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    // Copia il percorso in una variabile temporanea
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    if(server_folder) {
        if (tmp[len - 1] == '/') {
            tmp[len - 1] = 0;
        }

        // Crea le directory necessarie lungo il percorso
        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;
                // Crea la directory intermedia se non esiste
                if (access(tmp, F_OK) != 0) {
                    if (mkdir(tmp, 0777) != 0) {
                        perror("Errore nella creazione della cartella");
                        return 1;
                    }
                }
                *p = '/';
            }
        }

        // Crea la directory finale
        if (access(tmp, F_OK) != 0) {
            if (mkdir(tmp, 0777) != 0) {
                perror("Errore nella creazione della cartella");
                return 1;
            }
        }

        printf("La cartella '%s' è stata creata.\n", path);
        return 0;
    }
    // Trova l'ultimo '/' per escludere il nome del file
    for (p = tmp + len - 1; p >= tmp; p--) {
        if (*p == '/') {
            *p = '\0';
            break;
        }
    }

    // Crea le directory
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(tmp, S_IRWXU);
    return 0;
}



int is_valid_ip(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

int is_valid_port(const char *port_str) {
    int port = atoi(port_str);
    return port > 0 && port <= 65535;
}


// gestisci la richiesta di list della cartella server
int handle_ls(int new_socket, char* filename) {
    // Acquisisce il semaforo
    sem_wait(&dir_semaphore);

    char path[BUFFER_SIZE] = "ls ";
    strncat(path, filename, sizeof(path) - strlen(path) - 1);
    FILE *fp = popen(path, "r");
    if (fp == NULL) {
        perror("popen");
        sem_post(&dir_semaphore); // Rilascia il semaforo prima di tornare
        return 1;
    }

    char bufferOut[BUFFER_SIZE];

    // Leggi il risultato del comando
    while (fgets(bufferOut, sizeof(bufferOut), fp) != NULL) {
        // Manda il risultato al client
        send(new_socket, bufferOut, strlen(bufferOut), 0);
    }
    printf(" ls inviato correttamente\n");

    pclose(fp);

    // Rilascia il semaforo
    sem_post(&dir_semaphore);

    return 0;
}



bool is_subdirectory(const char *parent, const char *sub) {
    size_t len = strlen(parent);
    return strncmp(parent, sub, len) == 0 && (sub[len] == '/' || sub[len] == '\0');
}

int handle_get(int client_socket, char *filename) {
    // Acquisisce il semaforo
    sem_wait(&dir_semaphore);

    char cwd[PATH_MAX];
    char resolved_path[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
        send(client_socket, "SERVER_ERROR\n", 13, 0);
        sem_post(&dir_semaphore); // Rilascia il semaforo prima di tornare
        return 1;
    }

    printf("apro il file: '%s'\n", filename);
    printf("filename length: %zu\n", strlen(filename));
    remove_crlf(filename);

    // Risolvi il path assoluto del file richiesto
    if (realpath(filename, resolved_path) == NULL) {
        perror("realpath");
        send(client_socket, "FILE_NOT_FOUND\n", 15, 0);
        sem_post(&dir_semaphore); // Rilascia il semaforo prima di tornare
        return 1;
    }

    printf("%s", resolved_path);

    // Controlla se il path risolto è sotto la CWD
    if (!is_subdirectory(cwd, resolved_path)) {
        fprintf(stderr, "Access denied: '%s' is outside the server directory\n", resolved_path);
        send(client_socket, "ACCESS_DENIED\n", 14, 0);
        sem_post(&dir_semaphore); // Rilascia il semaforo prima di tornare
        return 1;
    }

    FILE *fp = fopen(resolved_path, "r");
    if (fp == NULL) {
        perror("fopen");
        send(client_socket, "FILE_NOT_FOUND\n", 15, 0);
        sem_post(&dir_semaphore); // Rilascia il semaforo prima di tornare
        return 1;
    }

    char buffer_out[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer_out, 1, sizeof(buffer_out), fp)) > 0) {
        if (send(client_socket, buffer_out, bytes_read, 0) == -1) {
            perror("Errore nell'invio dei dati");
            fclose(fp);
            close(client_socket);
            sem_post(&dir_semaphore); // Rilascia il semaforo prima di tornare
            return 1;
        }
    }

    fclose(fp);

    // Rilascia il semaforo
    sem_post(&dir_semaphore);

    return 0;
}


int handle_put(int client_socket, char* filename) {
    // Acquisisce il semaforo
    sem_wait(&dir_semaphore);

    remove_crlf(filename);
    create_directories(filename, false);

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Errore nell'apertura del file sul server in scrittura");
        close(client_socket);
        sem_post(&dir_semaphore); // Rilascia il semaforo prima di tornare
        return 1;
    }

    send(client_socket, "ACK", 3, 0);

    char buffer_in[BUFFER_SIZE];
    ssize_t bytes_received;
    // Continua a ricevere i dati rimanenti, se ce ne sono
    while ((bytes_received = recv(client_socket, buffer_in, sizeof(buffer_in), 0)) > 0) {
        fwrite(buffer_in, 1, bytes_received, fp);
    }

    fclose(fp);

    // Rilascia il semaforo
    sem_post(&dir_semaphore);

    return 0;
}


// gestisce la comunicazione con il client
void handle_client(int client_socket) {
    char buffer_in[BUFFER_SIZE];
    // legge il messaggio inviato dal client
    ssize_t bytes_read = read(client_socket, buffer_in, sizeof(buffer_in) - 1);
    if (bytes_read > 0) {
        buffer_in[bytes_read] = '\0';
        printf("Messaggio ricevuto dal client: %s\n", buffer_in);
    }

    if (strncmp(buffer_in, "LST ", 4) == 0) {
        char* filename = buffer_in + 4;
        handle_ls(client_socket, filename);
    } else if (strncmp(buffer_in, "GET ", 4) == 0) {
        char* filename = buffer_in + 4;
        handle_get(client_socket, filename);
    } else if (strncmp(buffer_in, "PUT ", 4) == 0) {
        char* filename = buffer_in + 4; // Il nome del file inizia dopo il comando "PUT "
        handle_put(client_socket, filename);
    }

    // chiudi il socket
    close(client_socket);
}


void *client_thread(void *arg) {
    // copio il socket in una variabile locale alla funzione
    int client_socket = *((int *)arg);

    // libero la memoria che era stata riservata al puntatore
    free(arg);

    handle_client(client_socket);
    return NULL;
}

void handle_sigint(int sig) {
    printf("Interruzione ricevuta. Chiudendo il socket...\n");
    close(server_socket);
    sem_destroy(&dir_semaphore); // Distrugge il semaforo
    exit(0);
}



int main(int argc, char *argv[]) {
    char *directory = NULL;
    char *ip_address = NULL;
    char *port_str = NULL;
    int opt;

    // Analizza le opzioni della linea di comando
    while ((opt = getopt(argc, argv, "d:a:p:h")) != -1) {
        switch (opt) {
            case 'd':
                directory = optarg;
                break;
            case 'a':
                ip_address = optarg;
                break;
            case 'p':
                port_str = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Verifica che l'opzione -d sia stata fornita
    if (directory == NULL) {
        fprintf(stderr, "L'opzione -d <directory> è obbligatoria\n");
        print_usage(argv[0]);
        return 1;
    }

    // Verifica che l'opzione -a sia stata fornita
    if (ip_address == NULL) {
        fprintf(stderr, "L'opzione -a <indirizzo IP> è obbligatoria\n");
        print_usage(argv[0]);
        return 1;
    }

    // Verifica che l'opzione -p sia stata fornita
    if (port_str == NULL) {
        fprintf(stderr, "L'opzione -p <numero di porta> è obbligatoria\n");
        print_usage(argv[0]);
        return 1;
    }

    // Controllo di validità dell'indirizzo IP
    if (!is_valid_ip(ip_address)) {
        fprintf(stderr, "L'indirizzo IP '%s' non è valido.\n", ip_address);
        return 1;
    }

    // Controllo di validità del numero di porta
    if (!is_valid_port(port_str)) {
        fprintf(stderr, "Il numero di porta '%s' non è valido.\n", port_str);
        return 1;
    }

    // Stampa l'indirizzo IP e il numero di porta specificati
    printf("Indirizzo IP specificato: %s\n", ip_address);
    printf("Numero di porta specificato: %s\n", port_str);

    // Controlla e crea la cartella
    if (create_directories(directory, true) != 0) {
        fprintf(stderr, "Errore nella creazione della cartella.\n");
        return 1;
    }

    // posizionati nella cartella specificata; se non riesci ritorna errore
    if (chdir(directory) != 0) {
        perror("chdir failed");
        printf("non riesco ad entrare nella cartella '%s'\n", directory);
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;

    // Crea il server socket TCP
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        return 1;
    }

    // Prepara l'indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    server_addr.sin_port = htons(atoi(port_str));

    // Associa l'indirizzo e la porta al server socket
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nell'associazione dell'indirizzo e porta al socket");
        close(server_socket);
        return 1;
    }

    // Mette il socket in ascolto; impostiamo il no di pending connection a 5
    if (listen(server_socket, 5) == -1) {
        perror("Errore nella messa in ascolto del socket");
        close(server_socket);
        return 1;
    }

    // Imposta l'opzione SO_REUSEADDR per riutilizzare l'indirizzo subito dopo la chiusura
    int option = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        perror("Errore nella impostazione di SO_REUSEADDR");
        close(server_socket);
        return 1;
    }

    printf("Server in ascolto su %s:%s\n", ip_address, port_str);

    // Inizializza il semaforo
    if (sem_init(&dir_semaphore, 0, 1) != 0) {
        perror("Errore nell'inizializzazione del semaforo");
        return 1;
    }

    // Accetta le connessioni in entrata
    while (1) {
        socklen_t client_len = sizeof(client_addr);
        // Il client socket verrà gestito da un thread; la memoria per andare a scrivere questo socket
        // sarà "di proprietà" del thread e dobbiamo allocarla dinamicamente
        int *client_socket = malloc(sizeof(int));

        if ((*client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len)) == -1) {
            perror("Errore nell'accettazione della connessione");
            close(server_socket);
            return 1;
        }

        // thread che gestisce la connessione del client: vuole una funzione con un unico argomento puntatore generico (void*) che accetta un unico parametro puntatore generico (void*)
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, client_socket);
        pthread_detach(tid);
    }

    // Chiude il socket del server (questo codice non sarà mai raggiunto)
    close(server_socket);

    return 0;
}