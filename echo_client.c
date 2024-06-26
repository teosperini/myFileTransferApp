#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    // Dichiarazione delle variabili
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Verifica che sia stato passato un argomento (messaggio) al programma
    if (argc != 2) {
        printf("Usage: %s <message>\n", argv[0]);
        return -1;
    }

    // Assegna il messaggio passato come argomento alla variabile message
    char *message = argv[1];

    // Creare il socket
    // AF_INET indica che utilizzeremo IPv4
    // SOCK_STREAM indica che utilizzeremo il protocollo TCP
    // 0 indica che il protocollo sarà scelto automaticamente basato sui parametri precedenti (TCP in questo caso)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Errore nella creazione del socket \n");
        return -1;
    }

    // Configurazione dell'indirizzo del server per trasmissione
    serv_addr.sin_family = AF_INET;  // AF_INET indica che utilizzeremo IPv4
    serv_addr.sin_port = htons(PORT);  // Convertire il numero di porta in network byte order

    // Convertire l'indirizzo IP da testo (stringa) a binario e assegnarlo alla struttura serv_addr
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Indirizzo non valido/Indirizzo non supportato \n");
        return -1;
    }

    // Tentativo di connessione al server
    // La funzione connect stabilisce una connessione con il server specificato in serv_addr
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Connessione fallita \n");
        return -1;
    }

    // Inviare il messaggio al server
    send(sock, message, strlen(message), 0);
    printf("Messaggio inviato: %s\n", message);

    // Leggere la risposta dal server
    // La funzione read legge i dati dal socket nel buffer
    read(sock, buffer, BUFFER_SIZE);
    printf("Risposta dal server: \n%s\n", buffer);

    // Chiudere il socket per liberare le risorse
    close(sock);

    return 0;
}