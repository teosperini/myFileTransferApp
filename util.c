#include "util.h"

/**
 * Questa funzione ritorna il path della/e parent directorie/s
 * del file passato in input
 * @param path il path da controllare
 * @return il percorso della parent directory, o NULL se non esiste
 */
char *get_parent_directory(const char *path) {
    char *parent_path = strdup(path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        return parent_path;
    }
    free(parent_path);
    return NULL;
}

/**
 * Questa funzione rimuove correttamente i caratteri di
 * Carriage Return e New Line
 * @param str la stringa dalla quale rimuovere i caratteri
 */
void remove_crlf(char *str) {
    int i, j = 0;
    int len = strlen(str);

    for (i = 0; i < len; i++) {
        if (str[i] != '\r' && str[i] != '\n') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

/**
 * Questa funzione crea tutte le directory necessarie per
 * la creazione del file sia sul server che sul client
 * @param path il path per creare le cartelle
 * @return 0 se non ci sono stati errori, -1 altrimenti
 */
int create_directories(char *path) {
    if(path == NULL){
        return 0;
    }
    char tmp[256];
    char *p = NULL;

    snprintf(tmp, sizeof(tmp), "%s", path);

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
                perror("Errore nella creazione della directory");
                return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
        perror("Errore nella creazione della directory finale");
        return -1;
    }
    return 0;
}

/**
 * Questa funzione verifica se la stringa è un indirizzo IP IPv4 valido
 * @param ip la stringa da verificare
 * @return 1 se la stringa è un indirizzo IP valido, 0 altrimenti
 */
int is_valid_ip(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

/**
 * Questa funzione controlla se la stringa rappresenta una porta valida
 * @param port_str la stringa da verificare
 * @return 1 se la stringa è una porta valida, 0 altrimenti
 */
int is_valid_port(const char *port_str) {
    int port = atoi(port_str);
    return port > 0 && port <= 65535;
}

/**
 * Questa funzione controlla se il path passato in input è un path
 * assoluto
 * @param filename il path da controllare
 * @return false se è un path assoluto, true altrimenti
 */
bool check_absolute_path(char* filename) {
    if (filename[0] == '/') {
        fprintf(stderr, "Access denied: '%s' is outside the server directory\n", filename);
        return false;
    }
    return true;
}

/**
 * Questa funzione verifica se il percorso specificato è una directory
 * @param path Il percorso del file o directory
 * @return 1 se è una directory, 0 altrimenti
 */
int is_directory(char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        perror("stat");
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

/**
 * Questa funzione ritorna il puntatore all'inizio del
 * nome del file associato al path passato in input
 * @param path il path del file
 * @return il puntatore al nome del file in caso venga trovato, il path altrimenti
 */
const char *get_filename(const char *path) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        return last_slash + 1;
    }
    return path;
}
