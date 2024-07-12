#include "util.h"
#include "server_operations.h"
#include "path_semaphore.h"


#define BUFFER_SIZE 1024

int server_socket;

void print_usage(const char *prog_name) {
    fprintf(stderr, "Uso: %s -d <directory> -a <indirizzo IP> -p <numero di porta> [-h]\n", prog_name);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            perror("recv");
            close(client_socket);
            break; // Esci dal ciclo invece di chiamare pthread_exit
        }

        buffer[bytes_received] = '\0';

        char *command = strtok(buffer, " ");
        char *filename = strtok(NULL, " ");

        if (strcmp(command, "ls") == 0) {
            handle_ls(client_socket, filename);
        } else if (strcmp(command, "get") == 0) {
            handle_get(client_socket, filename);
        } else if (strcmp(command, "put") == 0) {
            handle_put(client_socket, filename);
        } else {
            send(client_socket, "INVALID_COMMAND", 15, 0);
        }
    }
    close(client_socket);
    return NULL; // Assicurati di restituire NULL alla fine della funzione
}

int main(int argc, char *argv[]) {
    int opt;
    char *directory = NULL;
    char *ip_address = NULL;
    char *port_str = NULL;

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
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (directory == NULL || ip_address == NULL || port_str == NULL || !is_valid_ip(ip_address) || !is_valid_port(port_str)) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(port_str);

    signal(SIGINT, handle_sigint);

    if (chdir(directory) < 0) {
        perror("Errore nel cambio della directory");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel binding del socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Errore nella listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto su %s:%d\n", ip_address, port);

    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Errore nell'accept");
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, &client_socket);
        pthread_detach(tid);
    }

    close(server_socket);
    return 0;
}
