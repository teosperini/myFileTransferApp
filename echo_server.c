#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char bufferIn[BUFFER_SIZE];
    char bufferOut[BUFFER_SIZE];
    //strcat(path, "/home/matteo/Documents/SO2/projectMyFileTransferApp");
    // Creare socket file descriptor
    // Questo socket è utilizzato per ascoltare
    // AF_INET: IPv4, SOCK_STREAM: TCP, 0: selezione automatica del protocollo (TCP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurare l'indirizzo del server e il binding del socket
    address.sin_family = AF_INET;  // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // Accetta connessioni su qualsiasi indirizzo locale
    address.sin_port = htons(PORT);  // Convertire la porta in network byte order

    // Bind al socket
    // Assegnare l'indirizzo e la porta al socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen per connessioni in arrivo
    // 3: numero massimo di connessioni in coda
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d\n", PORT);

    while (1) {
        // Accettare una connessione in arrivo
        // La connessione in arrivo viene accettata su un nuovo socket, c'è quindi un socket per ogni connessione
        // Qaundo viene accettata una connessione, apre un nuovo socket per comunicare con essa
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        printf("Connessione accettata\n");

        // Leggere il messaggio dal client
        int valread = read(new_socket, bufferIn, BUFFER_SIZE);
        if(strncmp(bufferIn, "ls", 2) == 0) {
            // se il comando è ls, allora eseguilo sulla cartella riservata
            // all'utente sul server e mandagli in ritorno il risultato


            //printf("comando_ls");
            // Esegui il comando ls

            if (chdir("/home/matteo/Documents/SO2/projectMyFileTransferApp/ServerRootFolder") != 0) {
                perror("chdir failed");
                // Gestione dell'errore se il cambio di directory fallisce
            }

            FILE *fp = popen("ls", "r");
            if (fp == NULL) {
                perror("popen");
                close(new_socket);
                continue;
            }

            // Leggi il risultato del comando
            while (fgets(bufferOut, sizeof(bufferOut), fp) != NULL) {
                // Manda il risultato al client
                send(new_socket, bufferOut, strlen(bufferOut), 0);
            }


            pclose(fp);

        } else {
            //altrimenti fai l'echo del comando
            printf("Messaggio ricevuto: %s\n", bufferIn);
            send(new_socket, bufferIn, valread, 0);
            printf("Messaggio mandato indietro al client\n");
        }
        // Inviare il messaggio ricevuto al client (echo)

        // Chiudere la connessione con il client
        close(new_socket);
    }

    return 0;
}
