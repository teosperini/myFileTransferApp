#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_PATH_LENGTH 256

struct path_semaphore {
    char path[MAX_PATH_LENGTH];
    pthread_mutex_t mutex;
    int ref_count; // Contatore di riferimento
};

static struct path_semaphore *path_semaphores = NULL;
static int num_semaphores = 0;

void add_path_semaphore(const char *path) {
    // Verifica se il semaforo esiste già
    for (int i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            // Incrementa il contatore di riferimento se il semaforo esiste già
            path_semaphores[i].ref_count++;
            return;
        }
    }

    // Se il semaforo non esiste, creane uno nuovo
    struct path_semaphore new_semaphore;
    strcpy(new_semaphore.path, path);
    pthread_mutex_init(&new_semaphore.mutex, NULL);
    new_semaphore.ref_count = 1;

    if (num_semaphores == 0) {
        path_semaphores = malloc(sizeof(struct path_semaphore));
    } else {
        path_semaphores = realloc(path_semaphores, (num_semaphores + 1) * sizeof(struct path_semaphore));
    }

    if (path_semaphores == NULL) {
        fprintf(stderr, "Errore nell'allocazione della memoria per i semafori.\n");
        return;
    }

    path_semaphores[num_semaphores++] = new_semaphore;
}

void lock_path(const char *path) {
    for (int i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            pthread_mutex_lock(&path_semaphores[i].mutex);
            //printf("File %s Bloccato.\n", path);
            return;
        }
    }
    printf("Semaforo per %s non trovato.\n", path);
}

void unlock_path(const char *path) {
    for (int i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            pthread_mutex_unlock(&path_semaphores[i].mutex);
            //printf("File %s Sbloccato.\n", path);
            // Decrementa il contatore di riferimento
            path_semaphores[i].ref_count--;
            // Rimuovi il semaforo se il contatore di riferimento raggiunge zero
            if (path_semaphores[i].ref_count == 0) {
                pthread_mutex_destroy(&path_semaphores[i].mutex);
                for (int j = i; j < num_semaphores - 1; j++) {
                    path_semaphores[j] = path_semaphores[j + 1];
                }
                num_semaphores--;
                if (num_semaphores == 0) {
                    free(path_semaphores);
                    path_semaphores = NULL;
                } else {
                    path_semaphores = realloc(path_semaphores, num_semaphores * sizeof(struct path_semaphore));
                }
            }
            return;
        }
    }
    printf("Semaforo per %s non trovato.\n", path);
}
