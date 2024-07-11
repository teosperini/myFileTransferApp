#include "path_semaphore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct path_semaphore *path_semaphores = NULL;
static int num_semaphores = 0;

void add_path_semaphore(const char *path) {
    struct path_semaphore new_semaphore;

    if (num_semaphores == 0) {
        path_semaphores = malloc(sizeof(struct path_semaphore));
    } else {
        path_semaphores = realloc(path_semaphores, (num_semaphores + 1) * sizeof(struct path_semaphore));
    }

    if (path_semaphores == NULL) {
        fprintf(stderr, "Errore nell'allocazione della memoria per i semafori.\n");
        return;
    }

    strcpy(new_semaphore.path, path);
    pthread_mutex_init(&new_semaphore.mutex, NULL);

    path_semaphores[num_semaphores++] = new_semaphore;
}

void remove_path_semaphore(const char *path) {
    int i;
    for (i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            pthread_mutex_destroy(&path_semaphores[i].mutex);
            if (num_semaphores > 1) {
                memmove(&path_semaphores[i], &path_semaphores[i + 1], (num_semaphores - i - 1) * sizeof(struct path_semaphore));
            }
            num_semaphores--;
            path_semaphores = realloc(path_semaphores, num_semaphores * sizeof(struct path_semaphore));
            return;
        }
    }
    fprintf(stderr, "Semaforo per il path %s non trovato.\n", path);
}

void lock_path(const char *path) {
    int i;
    for (i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            pthread_mutex_lock(&path_semaphores[i].mutex);
            return;
        }
    }
    fprintf(stderr, "Semaforo per il path %s non trovato.\n", path);
}

void unlock_path(const char *path) {
    int i;
    for (i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            pthread_mutex_unlock(&path_semaphores[i].mutex);
            return;
        }
    }
    fprintf(stderr, "Semaforo per il path %s non trovato.\n", path);
}

