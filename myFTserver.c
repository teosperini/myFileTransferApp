#include "util.h"
#include "path_mutex.h"
#include "server_operations.h"

// Il descrittore del socket
int server_socket;

/**
 * @param prog_name il nome del programma
 */
void print_usage(const char *prog_name) {
    fprintf(stderr, "Uso: %s -d <directory> -a <indirizzo IP> -p <numero di porta> [-h]\n", prog_name);
}

/**
 * @brief Questa funzione esegue lo shutdown della connessione sia
 * in ricezione che in invio, poi chiude definitivamente il socket
 * @param sig il segnale inviato per la chiusura del socket
 */
void handle_sigint(int sig) {
    printf("Interruzione ricevuta, chiudo il socket ...\n");
    if (shutdown(server_socket, SHUT_RDWR) == -1) {
        perror("Errore nella chiusura del socket con shutdown");
    }
    exit(EXIT_SUCCESS);
}

/**
 * @brief Questa funzione gestisce la richiesta iniziale di ogni client
 * smistandola tra le varie funzioni di gestione dei comandi in base ai
 * primi 4 caratteri del buffer in arrivo
 * @param client_socket il socket dal quale proviene la richiesta
 */
void handle_client(int client_socket) {
    char buffer_in[BUFFER_SIZE];
    // legge il messaggio inviato dal client
    ssize_t bytes_read = read(client_socket, buffer_in, sizeof(buffer_in) - 1);
    if (bytes_read > 0) {
        buffer_in[bytes_read] = '\0';
        printf("Messaggio ricevuto dal client: %s\n", buffer_in);
    } else {
        return;
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
}

/**
 * @brief Questa funzione è la prima funzione eseguita da ogni nuovo thread.
 * Gestisce la deferenziazione del socket e la chiamata della funzione che
 * gestisce il client
 * @param arg l'indirizzo dell'area di memoria dove è allocato il valore
 * del socket
 * @return ritorna NULL
 */
void *client_thread(void *arg) {
    // copio il socket in una variabile locale alla funzione
    int client_socket = *((int *)arg);

    // libero la memoria che era stata riservata al puntatore
    free(arg);

    handle_client(client_socket);

    // chiudi il socket
    close(client_socket);
    return NULL;
}

/**
 * La funzione principale, nella quale viene avviato il server
 * e viene messo in ascolto
 * @param argc 
 * @param argv 
 * @return EXIT_SUCCESS se non ci sono stati errori, EXIT_FAILURE altrimenti
 */
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

    // Viene eseguito un controllo per verificare che il server sia stato
    // correttamente avviato
    if (directory == NULL || ip_address == NULL || port_str == NULL || !is_valid_ip(ip_address) || !is_valid_port(port_str)) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Viene tradotto il numero di porta da stringa a intero
    int port = atoi(port_str);

    // Viene impostata la funzione handle_sigint come handler
    // per il segnale SIGINT
    signal(SIGINT, handle_sigint);

    // Viene cambiata la directory del server a quella specificata
    // durante l'avvio
    if (chdir(directory) < 0) {
        perror("Errore nel cambio della directory");
        exit(EXIT_FAILURE);
    }

    // La struttura struct sockaddr_in è utilizzata per rappresentare
    // un indirizzo IPv4 e una porta a cui il socket deve essere
    // associato tramite bind
    struct sockaddr_in server_addr, client_addr;

    // Creazione del socket TCP IPv4
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }
    
    // Viene impostata l'opzione SO_REUSEADDR per poter riutilizzare
    // l'indirizzo subito dopo la chiusura del server socket
    int option = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        perror("Errore nella impostazione di SO_REUSEADDR");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Inizializzazione della struttura server_addr a zero
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;  // Famiglia di indirizzi IPv4
    server_addr.sin_addr.s_addr = inet_addr(ip_address);  // Indirizzo IP del server
    server_addr.sin_port = htons(port);  // Porta del server in formato di rete

    // Binding del socket all'indirizzo e alla porta specificati nella struttura server_addr
    // (viene assegnato un valore alla socket)
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel binding del socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Il socket è ora pronto ad accettare connessioni in entrata (massimo 5)
    // e si mette in ascolto sulla porta specificata
    if (listen(server_socket, 5) < 0) {
        perror("Errore nella listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto su %s:%d\n", ip_address, port);

    while (1) {
        socklen_t client_len = sizeof(client_addr);

        // Il client socket verrà gestito da un thread; la memoria per andare a scrivere questo socket
        // sarà "di proprietà" del thread e dobbiamo allocarla dinamicamente
        int *client_socket = malloc(sizeof(int));

       // Accettazione della connessione in entrata
        if ((*client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) == -1) {
            perror("Errore nell'accettazione della connessione");
            close(server_socket);
            free(client_socket);  // Liberazione della memoria allocata per il descrittore del client socket
            exit(EXIT_FAILURE);
        }

        // Creazione di un thread per gestire la connessione del client
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, client_socket) != 0) {
            perror("Errore nella creazione del thread");
            close(*client_socket);  // Chiusura del socket del client nel caso di errore
            free(client_socket);    // Liberazione della memoria allocata per il descrittore del socket client
            continue;  // Continua con il prossimo ciclo del while
        }
        
        // Detach del thread per permettere la terminazione automatica
        pthread_detach(tid);
    }
}
