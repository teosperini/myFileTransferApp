#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>


#define BUFFER_SIZE 1024

// Funzione per stampare l'uso del programma
void print_usage(const char *prog_name) {
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s -w -a <IP address> -p <port number> -f <local_path/filename_local> [-o <remote_path/filename_remote>]\n", prog_name);
    fprintf(stderr, "%s -r -a <IP address> -p <port number> -f <remote_path/filename_remote> [-o <local_path/filename_local>]\n", prog_name);
    fprintf(stderr, "%s -l -a <IP address> -p <port number> -f <remote_path/>\n", prog_name);
}

int is_valid_ip(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

int is_valid_port(const char *port_str) {
    int port = atoi(port_str);
    return port > 0 && port <= 65535;
}

void create_directories(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    // Copia il percorso in una variabile temporanea
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

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
}




int main(int argc, char *argv[]) {
    int type_w = 0;
    int type_r = 0;
    int type_l = 0;
    char *server_ip = NULL;
    char *port_str = NULL;
    char *f_file = NULL;
    char *o_file = NULL;
    int local_file_allocated = 0;
    int opt;

    // Analizza le opzioni della linea di comando
    while ((opt = getopt(argc, argv, "wrla:p:f:o:h")) != -1) {
        switch (opt) {
            case 'w':
                type_w = 1;
            break;
            case 'r':
                type_r = 1;
            break;
            case 'l':
                type_l = 1;
            break;
            case 'a':
                server_ip = optarg;
                break;
            case 'p':
                port_str = optarg;
                break;
            case 'f':
                f_file = optarg;
                break;
            case 'o':
                o_file = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    int check_sum = type_w + type_r + type_l;
    if (check_sum != 1) {
        if (check_sum < 1) {
            fprintf(stderr, "You need one type for the instruction\n");
            print_usage(argv[0]);
            return 1;
        }
        fprintf(stderr, "Too many types in the instruction\n");
        print_usage(argv[0]);
        return 2;
    }

    // Verifica che tutte le opzioni necessarie siano state fornite
    if (server_ip == NULL || port_str == NULL || (f_file == NULL && type_l == 0)) {
        fprintf(stderr, "Tutte le opzioni -a, -p, -f sono obbligatorie\n");
        print_usage(argv[0]);
        return 1;
    }

    // Controllo di validità dell'indirizzo IP
    if (!is_valid_ip(server_ip)) {
        fprintf(stderr, "L'indirizzo IP '%s' non è valido.\n", server_ip);
        return 1;
    }

    // Controllo di validità del numero di porta
    if (!is_valid_port(port_str)) {
        fprintf(stderr, "Il numero di porta '%s' non è valido.\n", port_str);
        return 1;
    }

    int port = atoi(port_str);

    // Se local_file è NULL, allocare memoria e copiare il nome del remote_file
    if (o_file == NULL && type_l == 0) {
        o_file = malloc(strlen(f_file) + 1);
        if (o_file == NULL) {
            perror("Errore di allocazione memoria");
            return 1;
        }
        strcpy(o_file, f_file);
        local_file_allocated = 1;
    }

    // Creazione del socket per il client
    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Connessione al server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nella connessione al server");
        close(client_socket);
        return 1;
    }

    char request[BUFFER_SIZE];

    if (type_r) {  // Operazione di lettura (get)
        // Invio della richiesta al server
            snprintf(request, sizeof(request), "GET %s\n", f_file);
        if (send(client_socket, request, strlen(request), 0) == -1) {
            perror("Errore nell'invio della richiesta");
            close(client_socket);
            return 1;
        }

        // Creazione delle directory se non esistono
        create_directories(o_file);
        // Ricezione del file dal server
        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        printf("il client ha ricevuto  \n");

        // Verifica della risposta del server
        if (bytes_received <= 0) {
            perror("Errore nella ricezione dei dati");
        } else {
            // Controllo se la risposta indica che il file non è stato trovato
            if (strncmp(buffer, "ACCESS_DENIED", 13) == 0) {
                printf("Accesso negato al file %s sul server\n", f_file);
            } else if (strncmp(buffer, "FILE_NOT_FOUND", 14) == 0) {
                printf("Il file '%s' non è stato trovato sul server\n", f_file);
            } else {
                // Apertura del file locale per la scrittura
                FILE *fp = fopen(o_file, "wb");
                if (fp == NULL) {
                    perror("Errore nell'apertura del file locale scrittura");
                    close(client_socket);
                    return 1;
                }

                // Scrivi i dati ricevuti nel file locale
                fwrite(buffer, 1, bytes_received, fp);

                // Continua a ricevere i dati rimanenti, se ce ne sono
                while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
                    fwrite(buffer, 1, bytes_received, fp);
                }

                if (bytes_received == -1) {
                    perror("Errore nella ricezione dei dati");
                } else {
                    printf("File '%s' scaricato come '%s'\n", o_file, f_file);
                }

                // Chiusura del file
                fclose(fp);
            }
        }
    } else if (type_w) {  // Operazione di scrittura (put)
        // Invio della richiesta al server
        snprintf(request, sizeof(request), "PUT %s\n", o_file);
        if (send(client_socket, request, strlen(request), 0) == -1) {
            perror("Errore nell'invio della richiesta");
            close(client_socket);
            return 1;
        }

        char buffer_ack[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_socket, buffer_ack, sizeof(buffer_ack), 0);

        // Verifica della risposta del server
        if (bytes_received <= 0) {
            perror("Errore nella ricezione dei dati");
        } else {
            // Controllo se la risposta indica che il file non è stato trovato
            if (strncmp(buffer_ack, "ACK", 3) == 0) {
                printf("Ack ricevuto");
            }
        }

        // Apertura del file locale per la lettura
        FILE *fp = fopen(f_file, "rb");
        if (fp == NULL) {
            perror("Errore nell'apertura del file locale per la lettura");
            close(client_socket);
            return 1;
        }

        // Invio del file al server
        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            if (send(client_socket, buffer, bytes_read, 0) == -1) {
                perror("Errore nell'invio dei dati");
                fclose(fp);
                close(client_socket);
                return 1;
            }
        }

        if (ferror(fp)) {
            perror("Errore nella lettura del file");
        } else {
            printf("File '%s' inviato come '%s'\n", f_file, o_file);
        }

        // Chiusura del file
        fclose(fp);
    } else if (type_l) {  // Operazione di lista (list)
        // Invio della richiesta al server
        bool f_was_null = false;
        if (f_file == NULL) {
            f_was_null = true;
            f_file = malloc(sizeof(char*)*250);
            strcat(f_file, "");
        }
        snprintf(request, sizeof(request), "LST %s\n", f_file);
        if (send(client_socket, request, strlen(request), 0) == -1) {
            perror("Errore nell'invio della richiesta");
            close(client_socket);
            return 1;
        }
        if (f_was_null) {
            free(f_file);
        }

        // Ricezione della lista dal server
        char buffer[BUFFER_SIZE];
        ssize_t bytes_received;
        while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
        }

        if (bytes_received == -1) {
            perror("Errore nella ricezione dei dati");
        }
    }

    // Chiusura del socket
    close(client_socket);

    // Libera la memoria allocata per local_file se era stata allocata
    if (local_file_allocated) {
        free(o_file);
    }

    return 0;
}
