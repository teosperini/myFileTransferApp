#include <errno.h>
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
#include <signal.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

int server_socket; // dichiarare globalmente il server socket

// Funzioni di utilità

// Stampa l'uso corretto del programma
void print_usage(const char *prog_name) {
    fprintf(stderr, "Uso: %s -d <directory> -a <indirizzo IP> -p <numero di porta> [-h]\n", prog_name);
}

// Restituisce il percorso della directory padre
char *get_parent_directory(const char *path) {
    char *parent_path = strdup(path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        return parent_path;
    }
    free(parent_path);
    return NULL; // Se non ci sono slash nel percorso
}

// Rimuove i caratteri \r e \n dalla stringa
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

// Crea le directory necessarie per il percorso specificato
int create_directories(const char *path) {
    if(path == NULL){
        return 0;
    }
    printf("Path nella funzione: %s\n", path);
    char tmp[256];
    char *p = NULL;

    // Copia il percorso in una variabile temporanea
    snprintf(tmp, sizeof(tmp), "%s", path);

    // Crea le directory
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            printf("Creazione cartella: %s\n", tmp);
            if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
                perror("Errore nella creazione della directory");
                return -1;
            }
            *p = '/';
        }
    }

    // Crea la directory finale se necessario
    if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
        perror("Errore nella creazione della directory finale");
        return -1;
    }
    printf("Cartella finale creata: %s\n", tmp);
    return 0;
}

// Controlla la validità dell'indirizzo IP
int is_valid_ip(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

// Controlla la validità del numero di porta
int is_valid_port(const char *port_str) {
    int port = atoi(port_str);
    return port > 0 && port <= 65535;
}

// Verifica che il percorso non sia assoluto
bool check_absolute_path(char* filename) {
    if (filename[0] == '/') {
        fprintf(stderr, "Access denied: '%s' is outside the server directory\n", filename);
        return false;
    }
    return true;
}

// Controlla se il path corrisponde ad un file o una directory
int is_directory(char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        // Error in stat
        perror("stat");
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

// Gestisce il comando "LS" inviato dal client
int handle_ls(int client_socket, char* filename) {
    if(!check_absolute_path(filename)) {
        send(client_socket, "ABSOLUTE_PATH_NOT_ALLOWED", 25, 0);
        close(client_socket);
        return -1;
    }

    char path[BUFFER_SIZE] = "ls ";
    strcat(path, filename);

    FILE *fp = popen(path, "r");
    if (fp == NULL) {
        perror("popen");
        send(client_socket, "DIRECTORY_NOT_FOUND", 19, 0);
        close(client_socket);
        return -1;
    }

    // Invio l'ACK al client
    send(client_socket, "ACK", 3, 0);

    // Aspetto l'ACK del client
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);

    if (strncmp(buffer, "ACK", 3) == 0) {
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }
    pclose(fp);
    return 0;
}

// Gestisce il comando "GET" inviato dal client
int handle_get(int client_socket, char *filename) {
    if(!check_absolute_path(filename)) {
        send(client_socket, "ABSOLUTE_PATH_NOT_ALLOWED", 25, 0);
        close(client_socket);
        return -1;
    }

    if(is_directory(filename)) {
        fprintf(stderr, "Errore, è una directory");
        send(client_socket, "IT_IS_A_DIRECTORY", 17, 0);
        close(client_socket);
        return -1;
    }
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        send(client_socket, "FILE_NOT_FOUND", 14, 0);
        close(client_socket);
        return -1;
    }

    // Invio l'ACK al client
    send(client_socket, "ACK", 3, 0);

    // Aspetto l'ACK del client
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);

    if (strncmp(buffer, "ACK", 3) == 0) {
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            if (send(client_socket, buffer, bytes_read, 0) < 0) {
                perror("Errore nell'invio dei dati");
                fclose(fp);
                close(client_socket);
                return -1;
            }
        }
    }
    fclose(fp);
    return 0;
}

// Gestisce il comando "PUT" inviato dal client
int handle_put(int client_socket, char* filename) {
    if(!check_absolute_path(filename)) {
        send(client_socket, "ABSOLUTE_PATH_NOT_ALLOWED", 25, 0);
        close(client_socket);
        return -1;
    }
    if(create_directories(get_parent_directory(filename)) < 0) {
        send(client_socket, "CANNOT_CREATE_DIRECTORY", 23, 0);
        close(client_socket);
        return -1;
    }

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Errore nell'apertura del file sul server in scrittura");
        close(client_socket);
        return -1;
    }

    send(client_socket, "ACK", 3, 0);

    char buffer_in[BUFFER_SIZE];
    ssize_t bytes_received;
    // Continua a ricevere i dati rimanenti, se ce ne sono
    while ((bytes_received = recv(client_socket, buffer_in, sizeof(buffer_in), 0)) > 0) {
        printf("'%s'\n",buffer_in);
        fwrite(buffer_in, 1, bytes_received, fp);
    }

    fclose(fp);
    return 0;
}

// Gestisce la comunicazione con il client
int handle_client(int client_socket) {
    char buffer_in[BUFFER_SIZE];
    // legge il messaggio inviato dal client
    ssize_t bytes_read = read(client_socket, buffer_in, sizeof(buffer_in) - 1);
    if (bytes_read > 0) {
        buffer_in[bytes_read] = '\0';
        printf("Messaggio ricevuto dal client: %s\n", buffer_in);
    } else {
        close(client_socket);
        return -1;
    }
    char* command = buffer_in;
    char* filename = buffer_in+4;
    remove_crlf(filename);

    if (strncmp(command, "LST ", 4) == 0) {
        handle_ls(client_socket, filename);
    } else if (strncmp(command, "GET ", 4) == 0) {
        handle_get(client_socket, filename);
    } else if (strncmp(command, "PUT ", 4) == 0) {
        handle_put(client_socket, filename);
    }

    // chiudi il socket
    close(client_socket);
    return 0;
}

// Thread che gestisce il client
void *client_thread(void *arg) {
    // copio il socket in una variabile locale alla funzione
    int client_socket = *((int *)arg);

    // libero la memoria che era stata riservata al puntatore
    free(arg);

    handle_client(client_socket);
    return NULL;
}

// Gestisce il segnale SIGINT per terminare il server
void handle_sigint(int sig) {
    printf("Interruzione ricevuta. Chiudendo il socket...\n");
    if (shutdown(server_socket, SHUT_RDWR) == -1) {
        perror("Errore nella chiusura del socket con shutdown");
    }
    close(server_socket);
    exit(0);
}

// Funzione principale del server
int main(int argc, char *argv[])
{
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
            return -1;
        }
    }

    // Verifica che l'opzione -d sia stata fornita
    if (directory == NULL) {
        fprintf(stderr, "L'opzione -d <directory> è obbligatoria\n");
        print_usage(argv[0]);
        return -1;
    }

    // Verifica che l'opzione -a sia stata fornita
    if (ip_address == NULL) {
        fprintf(stderr, "L'opzione -a <indirizzo IP> è obbligatoria\n");
        print_usage(argv[0]);
        return -1;
    }
    // Verifica che l'opzione -p sia stata fornita
    if (port_str == NULL) {
        fprintf(stderr, "L'opzione -p <numero di porta> è obbligatoria\n");
        print_usage(argv[0]);
        return -1;
    }

    // Controllo di validità dell'indirizzo IP
    if (!is_valid_ip(ip_address)) {
        fprintf(stderr, "L'indirizzo IP '%s' non è valido.\n", ip_address);
        return -1;
    }

    // Controllo di validità del numero di porta
    if (!is_valid_port(port_str)) {
        fprintf(stderr, "Il numero di porta '%s' non è valido.\n", port_str);
        return -1;
    }

    // Stampa l'indirizzo IP e il numero di porta specificati
    printf("Indirizzo IP specificato: %s\n", ip_address);
    printf("Numero di porta specificato: %s\n", port_str);

    // Controlla e crea la cartella
    if(create_directories(get_parent_directory(directory)) < 0) {
        // La creazione della cartella è fallita
        return -1;
    }

    // posizionati nella cartella specificata; se non riesci ritorna errore
    if (chdir(directory) != 0) {
        perror("chdir failed");
        printf("non riesco ad entrare nella cartella '%s'\n", directory);
        return -1;
    }

    struct sockaddr_in server_addr, client_addr;

    // Crea il server socket TCP
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        return -1;
    }

    // Prepara l'indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    server_addr.sin_port = htons(atoi(port_str));

    // Imposta l'opzione SO_REUSEADDR per riutilizzare l'indirizzo subito dopo la chiusura
    int option = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        perror("Errore nella impostazione di SO_REUSEADDR");
        close(server_socket);
        return -1;
    }

    // Associa l'indirizzo e la porta al server socket
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nell'associazione dell'indirizzo e porta al socket");
        close(server_socket);
        return -1;
    }

    // Mette il socket in ascolto; impostiamo il no di pending connection a 5
    if (listen(server_socket, 5) == -1) {
        perror("Errore nella messa in ascolto del socket");
        close(server_socket);
        return -1;
    }

    printf("Server in ascolto su %s:%s\n", ip_address, port_str);

    // Gestione del segnale SIGINT per terminare il server
    signal(SIGINT, handle_sigint);

    // Accetta le connessioni in entrata
    while (1) {
        socklen_t client_len = sizeof(client_addr);
        // Il client socket verrà gestito da un thread; la memoria per andare a scrivere questo socket
        // sarà "di proprietà" del thread e dobbiamo allocarla dinamicamente
        int *client_socket = malloc(sizeof(int));

        if ((*client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len)) == -1) {
            perror("Errore nell'accettazione della connessione");
            close(server_socket);
            return -1;
        }

        // Thread che gestisce la connessione del client
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, client_socket);
        pthread_detach(tid);
    }

    // Chiude il socket del server (questo codice non sarà mai raggiunto)
    close(server_socket);

    return 0;
}