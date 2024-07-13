#include "util.h"

/**
 * @param prog_name il nome del programma
 */
void print_usage(const char *prog_name) {
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s -w -a <IP address> -p <port number> -f <local_path/filename_local> [-o <remote_path/filename_remote>]\n", prog_name);
    fprintf(stderr, "%s -r -a <IP address> -p <port number> -f <remote_path/filename_remote> [-o <local_path/filename_local>]\n", prog_name);
    fprintf(stderr, "%s -l -a <IP address> -p <port number> -f <remote_path/>\n", prog_name);
}

/**
 * Questa funzione gestice l'invio del comando GET al server e
 * la relativa ricezione di risposte da parte del server
 * @param f_path Il path del file remoto
 * @param o_path Il path del file locale
 * @param client_socket Il socket del client
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
int r_command(char *f_path, char *o_path, int client_socket) {
    char request[BUFFER_SIZE];
    // Invio la richiesta get al server
    snprintf(request, sizeof(request), "GET %s\n", f_path);
    if (send(client_socket, request, strlen(request), 0) == -1) {
        perror("Errore nell'invio della richiesta");
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
        } else if (strncmp(buffer, "IT_IS_A_DIRECTORY", 17) == 0) {
            fprintf(stderr, "Errore, '%s' è una directory\n", f_path);
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
        return -1;
    }

    // Ricezione dati e scrittura nel file locale
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, fp);
    }

    if (bytes_received < 0) {
        perror("Errore nella ricezione dei dati nella parte 2 della GET:\nDati mancanti o incorretti\n");
        return -1;
    }

    printf("File '%s' scaricato come '%s'\n", f_path, o_path);

    // Chiusura del file
    fclose(fp);
    return 0;
}

/**
 * Questa funzione gestice l'invio del comando PUT al server e
 * la relativa ricezione di risposte da parte del server
 * @param f_path Il path del file locale
 * @param o_path Il path del file remoto
 * @param client_socket Il socket del client
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
int w_command(char *f_path, char *o_path, int client_socket) {
    char request[BUFFER_SIZE];
    // Apertura file locale in lettura
    FILE *fp = fopen(f_path, "rb");
    if (fp == NULL) {
        perror("Errore nell'apertura del file locale per la lettura nella PUT\n");
        return -1;
    }

    // Invio richiesta put al server
    snprintf(request, sizeof(request), "PUT %s\n", o_path);
    if (send(client_socket, request, strlen(request), 0) == -1) {
        perror("Errore nell'invio della richiesta\n");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    // Verifica della risposta del server
    if (bytes_received < 0) {
        perror("Errore nella ricezione dei dati\n");
        return -1;
    }
    if (strncmp(buffer, "ACK", 3) != 0) {
        if (strncmp(buffer, "ABSOLUTE_PATH_NOT_ALLOWED", 25) == 0) {
            fprintf(stderr, "L'utilizzo di un path assoluto non è permesso");
        } else if (strncmp(buffer, "CANNOT_CREATE_DIRECTORY", 23) == 0) {
            fprintf(stderr, "Errore del server durante la GET\n");
        } else {
            fprintf(stderr, "Errore sconosciuto\n");
        }
        return -1;
    }
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            perror("Errore nell'invio dei dati della PUT\n");
            fclose(fp);
            return -1;
        }
    }

    if (ferror(fp)) {
        perror("Errore nella lettura del file\n");
        return -1;
    }

    printf("File '%s' correttamente inviato come '%s'\n", f_path, o_path);
    // Chiusura del file
    fclose(fp);
    return false;
}

/**
 * Questa funzione gestice l'invio del comando LST al server e
 * la relativa ricezione di risposte da parte del server
 * @param f_path Il path della directory remota
 * @param client_socket Il socket del client
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
int l_command(char **f_path, int client_socket) {
    char request[BUFFER_SIZE];
    bool f_was_null = false;
    if (*f_path == NULL) {
        f_was_null = true;
        *f_path = malloc(sizeof(char *) * PATH_MAX);
        strcat(*f_path, "");
    }

    // Invio della richiesta al server
    snprintf(request, sizeof(request), "LST %s\n", *f_path);
    if (send(client_socket, request, strlen(request), 0) < 0) {
        perror("Errore nell'invio della richiesta LS\n ");
        return -1;
    }

    if (f_was_null) {
        free(*f_path);
    }

    // Verifica della risposta del server
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytes_received < 0) {
        fprintf(stderr, "Errore nella ricezione dei dati nella parte 1 della GET:\nDati mancanti o incorretti\n");
        return -1;
    }
    if (strncmp(buffer, "ACK", 3) != 0) {
        if (strncmp(buffer, "ABSOLUTE_PATH_NOT_ALLOWED", 25) == 0) {
            fprintf(stderr, "L'utilizzo di un path assoluto non è permesso");
        } else if (strncmp(buffer, "DIRECTORY_NOT_FOUND", 19) == 0) {
            fprintf(stderr, "La directory %s non esiste\n", *f_path);
        } else {
            fprintf(stderr, "Errore sconosciuto\n");
        }
        return -1;
    }

    send(client_socket, "ACK", 3, 0);

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    if (bytes_received < 0) {
        perror("Errore nella ricezione dei dati della LS");
        return -1;
    }
    return 0;
}

/**
 * La funzione principale del client, che viene eseguita ogni volta
 * che viene inviato un messaggio dal client
 * @param argc
 * @param argv
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
int main(int argc, char *argv[]) {
    int type_w = 0;
    int type_r = 0;
    int type_l = 0;
    char *server_ip = NULL;
    char *port_str = NULL;
    char *f_path = NULL;
    char *o_path = NULL;
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
                exit(EXIT_SUCCESS);
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Verifico che siano
    int check_sum = type_w + type_r + type_l;
    if (check_sum != 1) {
        if (check_sum < 1) {
            print_usage(argv[0]);
        } else {
            print_usage(argv[0]);
        }
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Verifica che tutte le opzioni necessarie siano state fornite
    if (server_ip == NULL || port_str == NULL || (f_path == NULL && type_l == 0)) {
        fprintf(stderr, "Tutte le opzioni -a, -p, -f sono obbligatorie\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Controllo di validità dell'indirizzo IP
    if (!is_valid_ip(server_ip)) {
        fprintf(stderr, "L'indirizzo IP '%s' non è valido.\n", server_ip);
        exit(EXIT_FAILURE);
    }

    // Controllo di validità del numero di porta
    if (!is_valid_port(port_str)) {
        fprintf(stderr, "Il numero di porta '%s' non è valido.\n", port_str);
        exit(EXIT_FAILURE);
    }

    int port = atoi(port_str);

    // Se o_path è NULL e non sono nell'opzione l, allocare memoria e copiare il nome del f_path
    if (o_path == NULL && type_l == 0) {
        o_path = malloc(strlen(f_path) + 1);
        if (o_path == NULL) {
            perror("Errore di allocazione memoria");
            exit(EXIT_FAILURE);
        }
        strcpy(o_path, f_path);
    }

    // Creazione del socket per il client
    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Connessione al server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nella connessione al server");
        exit(EXIT_FAILURE);
    }

    if (type_r) {
        // Operazione di lettura (get)
        if (r_command(f_path, o_path, client_socket) < 0) {
            exit(EXIT_FAILURE);
        }
    } else if (type_w) {
        // Operazione di scrittura (put)
        if (w_command(f_path, o_path, client_socket) < 0) {
            exit(EXIT_FAILURE);
        }
    } else if (type_l) {
        // Operazione di lista (list)
        if (l_command(&f_path, client_socket) < 0) {
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
