#ifndef PATH_SEMAPHORE_H
#define PATH_SEMAPHORE_H

#include <pthread.h>

#define MAX_PATH_LENGTH 256

struct path_semaphore {
    char path[MAX_PATH_LENGTH];
    pthread_mutex_t mutex;
};

/**
 * @brief Aggiunge un semaforo per il path specificato.
 *
 * @param path Il path da bloccare con il semaforo.
 */
void add_path_semaphore(const char *path);

/**
 * @brief Blocca l'accesso al path specificato.
 *
 * @param path Il path da bloccare.
 */
void lock_path(const char *path);

/**
 * @brief Sblocca l'accesso al path specificato.
 *
 * @param path Il path da sbloccare.
 */
void unlock_path(const char *path);


#endif /* PATH_SEMAPHORE_H */

