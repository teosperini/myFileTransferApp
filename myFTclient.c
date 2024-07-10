#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm-generic/errno-base.h>


#define BUFFER_SIZE 1024

// Funzione per stampare l'uso del programma
void print_usage(const char *prog_name) {
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s -w -a <IP address> -p <port number> -f <local_path/filename_local> [-o <remote_path/filename_remote>]\n", prog_name);
    fprintf(stderr, "%s -r -a <IP address> -p <port number> -f <remote_path/filename_remote> [-o <local_path/filename_local>]\n", prog_name);
    fprintf(stderr, "%s -l -a <IP address> -p <port number> -f <remote_path/>\n", prog_name);
}

//todo duplicate
int is_valid_ip(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

// Funzione per ottenere il nome del file da un percorso completo
const char *get_filename(const char *path) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        return last_slash + 1;
    }
    return path; // Se non ci sono slash nel percorso
}

// Funzione per ottenere la directory padre da un percorso completo
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

//todo duplicate
int is_valid_port(const char *port_str) {
    int port = atoi(port_str);
    return port > 0 && port <= 65535;
}

void create_directories(char *path) {
    if(path == NULL){
        return;
    }
    printf("Path nella funzione: %s\n", path);
    char tmp[256];
    char *p = NULL;

    // Copia il percorso in una variabile temporanea
    snprintf(tmp, sizeof(tmp), "%s", path);
    free(path);
    // Crea le directory
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            printf("Creazione cartella: %s\n", tmp);
            if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
                perror("Errore nella creazione della directory");
            }
            *p = '/';
        }
    }

    // Crea la directory finale se necessario
    if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
        perror("Errore nella creazione della directory finale");
    }
    printf("Cartella finale creata: %s\n", tmp);
}




int main(int argc, char *argv[]) {
    int type_w = 0;
    int type_r = 0;
    int type_l = 0;
    char *server_ip = NULL;
    char *port_str = NULL;
    char *f_path = NULL;
    char *o_path = NULL;
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
                f_path = optarg;
            break;
            case 'o':
                o_path = optarg;
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
    if (server_ip == NULL || port_str == NULL || (f_path == NULL && type_l == 0)) {
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
    if (o_path == NULL && type_l == 0) {
        o_path = malloc(strlen(f_path) + 1);
        if (o_path == NULL) {
            perror("Errore di allocazione memoria");
            return 1;
        }
        strcpy(o_path, f_path);
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

    if (type_r) {
        // Operazione di lettura (get)

        // Invio la richiesta get al server
        snprintf(request, sizeof(request), "GET %s\n", f_path);
        if (send(client_socket, request, strlen(request), 0) == -1) {
            perror("Errore nell'invio della richiesta");
            close(client_socket);
            return -1;
        }

        // Verifico la risposta del server
        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received < 0) {
            fprintf(stderr, "Errore nella ricezione dei dati nella parte 1 della GET:\nDati mancanti o incorretti\n");
            return -1;
        }

        if (strncmp(buffer, "ACK", 3) != 0) {
            if (strncmp(buffer, "ABSOLUTE_PATH_NOT_ALLOWED", 25) == 0) {
                fprintf(stderr, "L'utilizzo di un path assoluto non è permesso\n");
            } else if (strncmp(buffer, "FILE_NOT_FOUND", 14) == 0) {
                fprintf(stderr, "Il file '%s' non è stato trovato sul server\n", f_path);
            } else {
                fprintf(stderr, "Errore sconosciuto\n");
            }
            return -1;
        }
        // Se siamo a questo punto abbiamo ricevuto un ACK dal server

        // Invio ACK al server per autorizzare il trasferimento del file
        send(client_socket, "ACK", 3, 0);

        create_directories(get_parent_directory(o_path));

        // Apertura del file locale per la scrittura
        FILE *fp = fopen(o_path, "wb");
        if (fp == NULL) {
            perror("Errore nell'apertura del file locale scrittura\n");
            close(client_socket);
            return 1;
        }

        // Ricezione dati e scrittura nel file locale
        while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
            fwrite(buffer, 1, bytes_received, fp);
        }

        if (bytes_received < 0) {
            perror("Errore nella ricezione dei dati nella parte 2 della GET:\nDati mancanti o incorretti\n");
        } else {
            printf("File '%s' scaricato come '%s'\n", f_path, o_path);
        }

        // Chiusura del file
        fclose(fp);

    } else if (type_w) {
        // Operazione di scrittura (put)

        // Apertura file locale in lettura
        FILE *fp = fopen(f_path, "rb");
        if (fp == NULL) {
            perror("Errore nell'apertura del file locale per la lettura nella PUT\n");
            close(client_socket);
            return 1;
        }

        // Invio richiesta put al server
        snprintf(request, sizeof(request), "PUT %s\n", o_path);
        if (send(client_socket, request, strlen(request), 0) == -1) {
            perror("Errore nell'invio della richiesta\n");
            close(client_socket);
            return 1;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        // Verifica della risposta del server
        if (bytes_received < 0) {
            perror("Errore nella ricezione dei dati\n");
            return 1;
        }

        if (strncmp(buffer, "ABSOLUTE_PATH_NOT_ALLOWED", 25) == 0) {
            fprintf(stderr, "L'utilizzo di un path assoluto non è permesso");
        } else if (strncmp(buffer, "CANNOT_CREATE_DIRECTORY", 23) == 0) {
            fprintf(stderr, "Errore del server durante la GET\n");
        } else if (strncmp(buffer, "ACK", 3) != 0) {

            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                if (send(client_socket, buffer, bytes_read, 0) < 0) {
                    perror("Errore nell'invio dei dati della PUT\n");
                    fclose(fp);
                    close(client_socket);
                    return 1;
                }
            }

            if (ferror(fp)) {
                perror("Errore nella lettura del file\n");
            } else {
                printf("File '%s' correttamente inviato come '%s'\n", f_path, o_path);
            }
            // Chiusura del file
            fclose(fp);
        }
    } else if (type_l) {
        // Operazione di lista (list)
        // Invio della richiesta al server
        bool f_was_null = false;
        if (f_path == NULL) {
            f_was_null = true;
            f_path = malloc(sizeof(char*)*PATH_MAX);
            strcat(f_path, "");
        }

        snprintf(request, sizeof(request), "LST %s\n", f_path);
        if (send(client_socket, request, strlen(request), 0) < 0) {
            perror("Errore nell'invio della richiesta LS\n ");
            close(client_socket);
            return 1;
        }
        if (f_was_null) {
            free(f_path);
        }

        // Ricezione della lista dal server
        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            fprintf(stderr, "Errore nella ricezione dei dati nella parte 1 della GET:\nDati mancanti o incorretti\n");
        } else
            if (strncmp(buffer, "ABSOLUTE_PATH_NOT_ALLOWED", 25) == 0) {
                fprintf(stderr, "L'utilizzo di un path assoluto non è permesso\n");
            } else {
                if (strncmp(buffer, "FILE_PATH_NOT_RESOLVED", 22) == 0) {
                    fprintf(stderr, "Il path del file '%s' non è stato risolto correttamente\n",f_path);
                } else
                    if (strncmp(buffer, "FILE_NOT_FOUND", 14) == 0) {
                        fprintf(stderr, "Il file '%s' non è stato trovato sul server\n", f_path);
                    } else
                        if (strncmp(buffer, "ACCESS_DENIED", 13) == 0) {
                            fprintf(stderr, "Accesso negato al file %s sul server\n", f_path);
                        } else
                            if (strncmp(buffer, "SERVER_ERROR", 12) == 0) {
                                fprintf(stderr, "Errore del server durante la GET\n");
                            } else {
                                buffer[bytes_received] = '\0';
                                // Continua a ricevere i dati rimanenti, se ce ne sono
                                printf("%s", buffer);
                                while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
                                    buffer[bytes_received] = '\0';
                                    printf("%s", buffer);
                                }

                                if (bytes_received < 0) {
                                    perror("Errore nella ricezione dei dati della LS");
                                }
                            }
        }
    }

    // Chiusura del socket
    close(client_socket);

    // Libera la memoria allocata per local_file se era stata allocata
    if (local_file_allocated) {
        free(o_path);
    }
    return 0;
}
