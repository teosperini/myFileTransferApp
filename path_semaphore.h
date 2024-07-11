#ifndef PATH_SEMAPHORE_H
#define PATH_SEMAPHORE_H

#include <pthread.h>

#define MAX_PATH_LENGTH 256


/**
 * @brief Aggiunge un semaforo per il path specificato e blocca l'accesso.
 *
 * @param path Il path da bloccare con il semaforo.
 */
void lock(const char *path);


/**
 * @brief Sblocca l'accesso al path specificato.
 *
 * @param path Il path da sbloccare.
 */
void unlock(const char *path);


#endif /* PATH_SEMAPHORE_H */
