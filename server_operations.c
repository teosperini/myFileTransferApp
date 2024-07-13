#include "server_operations.h"
#include "util.h"
#include "path_semaphore.h"


/**
 * @brief Questa funzione gestisce la risposta del server in caso
 * dell'arrivo di una richiesta LIST da parte di un client
 * @param client_socket il socket dal quale proviene la richiesta
 * @param dirname il path e il nome della cartella della quale inviare il contenuto
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
int handle_ls(int client_socket, char* dirname) {
    if(!check_absolute_path(dirname)) {
        send(client_socket, "ABSOLUTE_PATH_NOT_ALLOWED", 25, 0);
        close(client_socket);
        return -1;
    }

    char path[BUFFER_SIZE] = "ls ";
    strcat(path, dirname);

    FILE *fp = popen(path, "r");
    if (fp == NULL) {
        perror("popen");
        send(client_socket, "DIRECTORY_NOT_FOUND", 19, 0);
        close(client_socket);
        return -1;
    }

    send(client_socket, "ACK", 3, 0);

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

/**
 * @brief Questa funzione gestisce la risposta del server in caso
 * dell'arrivo di una richiesta GET da parte di un client
 * @param client_socket il socket dal quale proviene la richiesta
 * @param filename il path e il nome del file da inviare al client
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
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

    lock(filename);
    printf("File %s bloccato.\n", filename);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        send(client_socket, "FILE_NOT_FOUND", 14, 0);
        close(client_socket);
        unlock(filename);
        printf("File %s sbloccato.\n", filename);
        return -1;
    }

    send(client_socket, "ACK", 3, 0);

    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);

    if (strncmp(buffer, "ACK", 3) == 0) {
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            if (send(client_socket, buffer, bytes_read, 0) < 0) {
                perror("Errore nell'invio dei dati");
                fclose(fp);
                close(client_socket);
                unlock(filename);
                printf("File %s sbloccato.\n", filename);
                return -1;
            }
        }
    }
    fclose(fp);
    unlock(filename);
    printf("File %s sbloccato.\n", filename);
    return 0;
}

/**
 * @brief Questa funzione gestisce la risposta del server in caso
 * dell'arrivo di una richiesta PUT da parte di un client
 * @param client_socket il socket dal quale proviene la richiesta
 * @param filename il path e il nome del file da salvare sul server
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
int handle_put(int client_socket, char* filename) {
    if (!check_absolute_path(filename)) {
        send(client_socket, "ABSOLUTE_PATH_NOT_ALLOWED", 25, 0);
        close(client_socket);
        printf("File %s sbloccato.\n", filename);
        return -1;
    }

    lock(filename);
    printf("File %s bloccato.\n", filename);

    char *parent_dir = get_parent_directory(filename);
    if (parent_dir) {
        if (create_directories(parent_dir) < 0) {
            send(client_socket, "CANNOT_CREATE_DIRECTORY", 23, 0);
            close(client_socket);
            free(parent_dir);
            return -1;
        }
        free(parent_dir);
    }

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Errore nell'apertura del file sul server in scrittura");
        close(client_socket);
        unlock(filename);
        printf("File %s sbloccato.\n", filename);
        return -1;
    }

    send(client_socket, "ACK", 3, 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, fp);
    }
    fclose(fp);

    unlock(filename);
    printf("File %s sbloccato.\n", filename);
    return 0;
}