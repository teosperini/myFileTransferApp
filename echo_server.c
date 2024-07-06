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

void print_usage(const char *prog_name) {
    fprintf(stderr, "Uso: %s -d <directory> -a <indirizzo IP> -p <numero di porta> [-h]\n", prog_name);
}

int create_directory(const char *directory) {
    // Controlla se la cartella esiste
    if (access(directory, F_OK) == 0) {
        printf("La cartella '%s' esiste già.\n", directory);
        return 0;
    } else {
        // La cartella non esiste, creala con permessi 0777
        if (mkdir(directory, 0777) == 0) {
            printf("La cartella '%s' è stata creata.\n", directory);
            return 0;
        } else {
            perror("Errore nella creazione della cartella");
            return 1;
        }
    }
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
int handle_ls_request(int new_socket) {
    char bufferOut[1024];

    FILE *fp = popen("ls", "r");
    if (fp == NULL) {
        perror("popen");
        return 1;
    }

    // Leggi il risultato del comando
    while (fgets(bufferOut, sizeof(bufferOut), fp) != NULL) {
        // Manda il risultato al client
        send(new_socket, bufferOut, strlen(bufferOut), 0);
    }

    pclose(fp);
    return 0;
}

// gestisce la comunicazione con il client
void handle_client(int client_socket) {
    char buffer[1024];
    ssize_t bytes_read;

    // leggiamo il messaggio inviato dal client
    bytes_read = read(client_socket, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Messaggio ricevuto dal client: %s\n", buffer);
    }

    if(strncmp(buffer, "ls", 2) == 0) {
        handle_ls_request(client_socket);
    }

    // Chiudi il socket del client
    close(client_socket);
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
    if (create_directory(directory) != 0) {
        fprintf(stderr, "Errore nella creazione della cartella.\n");
        return 1;
    }

    // posizionati nella cartella specificata; se non riesci ritorna errore
    if (chdir(directory) != 0) {
        perror("chdir failed");
        printf("non riesco ad entrare nella cartella '%s'\n", directory);
        return 1;
    }

    // Creazione del socket per il server
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

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

    printf("Server in ascolto su %s:%s\n", ip_address, port_str);

    // Accetta le connessioni in entrata
    while (1) {
        client_len = sizeof(client_addr);
        if ((client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len)) == -1) {
            perror("Errore nell'accettazione della connessione");
            close(server_socket);
            return 1;
        }

        // Gestisce la connessione del client
        handle_client(client_socket);
    }

    // Chiude il socket del server (questo codice non sarà mai raggiunto)
    close(server_socket);

    return 0;
}