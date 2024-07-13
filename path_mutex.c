#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_PATH_LENGTH 256

// Struttura che rappresenta un mutex associato a un percorso
struct path_mutex {
    char path[MAX_PATH_LENGTH];
    pthread_mutex_t mutex;
    int ref_count; // Contatore di riferimento
};

static pthread_mutex_t lock_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct path_mutex *path_mutexes = NULL;
static int num_mutexes = 0;

/**
 * @brief fa il lock del mutex associato al path in input
 *
 * Questa funzione cerca prima un mutex già associato al path
 * Se il mutex non esiste, ne crea uno nuovo e lo aggiunge alla lista
 * Altrimenti incrementa il contatore di riferimento per il mutex associato al path
 * Infine, fa il lock del mutex
 *
 * @param path percorso del file per cui fare il lock del mutex
 */
void lock(const char *path) {
    pthread_mutex_lock(&lock_mutex);

    // Verifica se il mutex esiste già
    int j = -1;
    for (int i = 0; i < num_mutexes; i++) {
        if (strcmp(path_mutexes[i].path, path) == 0) {
            j = i;
            break;
        }
    }

    if (j < 0) {
        // Se il mutex non esiste, creane uno nuovo
        struct path_mutex new_mutex;
        strcpy(new_mutex.path, path);
        pthread_mutex_init(&new_mutex.mutex, NULL);
        new_mutex.ref_count = 1;

        if (num_mutexes == 0) {
            path_mutexes = malloc(sizeof(struct path_mutex));
        } else {
            path_mutexes = realloc(path_mutexes, (num_mutexes + 1) * sizeof(struct path_mutex));
        }

        if (path_mutexes == NULL) {
            fprintf(stderr, "Errore nell'allocazione della memoria per i mutex\n");
            return;
        }

        path_mutexes[num_mutexes] = new_mutex;
        j = num_mutexes;
        num_mutexes ++;
    }

    // incrementa il contatore
    path_mutexes[j].ref_count++;

    pthread_mutex_unlock(&lock_mutex);

    pthread_mutex_lock(&path_mutexes[j].mutex);
}

/**
 * @brief fa l'unlock il mutex associato al path in input
 *
 * Questa funzione cerca prima il mutex associato al path
 * Fa l'unlock del mutex e decrementa il contatore di riferimento
 * Se il contatore di riferimento arriva a zero, distrugge il mutex
 * e lo rimuove dalla lista
 *
 * @param path percorso del file per cui fare l'unlock del mutex
 */
void unlock(const char *path) {
    pthread_mutex_lock(&lock_mutex);

    for (int i = 0; i < num_mutexes; i++) {
        if (strcmp(path_mutexes[i].path, path) == 0) {
            // sblocca il mutex e diminuisce il contatore di riferimento
            pthread_mutex_unlock(&path_mutexes[i].mutex);
            path_mutexes[i].ref_count--;

            // se il contatore è zero, si rimuove il mutex
            if (path_mutexes[i].ref_count == 0) {
                pthread_mutex_destroy(&path_mutexes[i].mutex);
                for (int j = i; j < num_mutexes - 1; j++) {
                    path_mutexes[j] = path_mutexes[j + 1];
                }
                num_mutexes--;
                if (num_mutexes == 0) {
                    free(path_mutexes);
                    path_mutexes = NULL;
                } else {
                    path_mutexes = realloc(path_mutexes, num_mutexes * sizeof(struct path_mutex));
                }
            }
            break;
        }
    }

    pthread_mutex_unlock(&lock_mutex);
}
