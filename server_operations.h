#ifndef SERVER_OPERATIONS_H
#define SERVER_OPERATIONS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>

int handle_ls(int client_socket, char* filename);
int handle_get(int client_socket, char *filename);
int handle_put(int client_socket, char* filename);
void handle_sigint(int sig);

#endif // SERVER_OPERATIONS_H
