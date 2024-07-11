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

static pthread_mutex_t lock_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct path_semaphore *path_semaphores = NULL;
static int num_semaphores = 0;

void lock(const char *path) {
    pthread_mutex_lock(&lock_mutex);

    // Verifica se il semaforo esiste già
    int j = -1;
    for (int i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            j = i;
            break;
        }
    }

    if (j < 0) {
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

        path_semaphores[num_semaphores] = new_semaphore;
        j = num_semaphores;
        num_semaphores ++;
    }

    // incrementa il contatore
    path_semaphores[j].ref_count++;

    pthread_mutex_unlock(&lock_mutex);

    pthread_mutex_lock(&path_semaphores[j].mutex);
}

void unlock(const char *path) {
    pthread_mutex_lock(&lock_mutex);

    for (int i = 0; i < num_semaphores; i++) {
        if (strcmp(path_semaphores[i].path, path) == 0) {
            // sblocca il mutex e diminuisce il contatore di riferimento
            pthread_mutex_unlock(&path_semaphores[i].mutex);
            path_semaphores[i].ref_count--;

            // se il contatore è zero, si rimuove il semaforo
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
            break;
        }
    }

    pthread_mutex_unlock(&lock_mutex);
}
