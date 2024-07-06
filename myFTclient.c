#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

// Funzione per stampare l'uso del programma
void print_usage(const char *prog_name) {
    fprintf(stderr, "usage:\n %s -w -a <IP address> -p <port number> -f <local_path/filename_local> -o <remote_path/filename_remote>\n", prog_name);
    fprintf(stderr, " %s -r -a <IP address> -p <port number> -f <remote_path/filename_remote> -o <local_path/filename_local>\n", prog_name);
    fprintf(stderr, " %s -l -a <IP address> -p <port number> -f <remote_path>\n", prog_name);
}

int main(int argc, char* argv[]) {
    bool type_w = false;
    bool type_r = false;
    bool type_l = false;
    char *ip_address = NULL;
    char *port_str = NULL;
    char *f_dir = NULL;
    char *o_dir = NULL;
    int opt;

    // Analizza le opzioni della linea di comando
    while ((opt = getopt(argc, argv, "wrla:p:f:o:h")) != -1) {
        switch (opt) {
        case 'w':
            if (!type_r && !type_l) {
                type_w = true;
            } else {
                print_usage(argv[0]);
                return 1;
            }
            break;
        case 'r':
            if (!type_w && !type_l) {
                type_r = true;
            } else {
                print_usage(argv[0]);
                return 1;
            }
            break;
        case 'l':
            if (!type_r && !type_w) {
                type_l = true;
            } else {
                print_usage(argv[0]);
                return 1;
            }
            break;
        case 'a':
            ip_address = optarg;
            break;
        case 'p':
            port_str = optarg;
            break;
        case 'f':
            f_dir = optarg;
            break;
        case 'o':
            o_dir = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }
    if(!type_w && !type_r && !type_l){
        fprintf(stderr, "The request need one identifier (r/w/l).\n");
        print_usage(argv[0]);
        return 1;
    }
    // Verifica che indirizzo IP e porta siano stati forniti
    if (ip_address == NULL || port_str == NULL) {
        fprintf(stderr, "IP address and port number are required.\n");
        print_usage(argv[0]);
        return 1;
    }

    // Converti la porta in un intero
    int port = atoi(port_str);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number.\n");
        return 1;
    }

    // Verifica che il messaggio sia stato fornito
    if (optind >= argc) {
        fprintf(stderr, "Expected message argument after options\n");
        print_usage(argv[0]);
        return 1;
    }

    // Assegna il messaggio passato come argomento alla variabile message
    char *message = argv[optind];

    // Dichiarazione delle variabili
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Creare il socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Configurazione dell'indirizzo del server per trasmissione
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convertire l'indirizzo IP da testo a binario e assegnarlo alla struttura serv_addr
    if (inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Tentativo di connessione al server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Inviare il messaggio al server
    send(sock, message, strlen(message), 0);
    printf("Messaggio inviato: %s\n", message);

    // Leggere la risposta dal server
    int bytes_read = read(sock, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the received string
        printf("Risposta dal server: \n%s\n", buffer);
    } else {
        printf("Nessuna risposta dal server\n");
    }

    // Chiudere il socket per liberare le risorse
    close(sock);

    return 0;
}
