#include "util.h"

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

int is_valid_ip(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

int is_valid_port(const char *port_str) {
    int port = atoi(port_str);
    return port > 0 && port <= 65535;
}

bool check_absolute_path(char* filename) {
    if (filename[0] == '/') {
        fprintf(stderr, "Access denied: '%s' is outside the server directory\n", filename);
        return false;
    }
    return true;
}

int is_directory(char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        perror("stat");
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

const char *get_filename(const char *path) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        return last_slash + 1;
    }
    return path;
}
