#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <asm-generic/errno-base.h>

#define BUFFER_SIZE 1024

char *get_parent_directory(const char *path);
void remove_crlf(char *str);
int create_directories(char *path);
int is_valid_ip(const char *ip);
int is_valid_port(const char *port_str);
bool check_absolute_path(char* filename);
int is_directory(char *path);
const char *get_filename(const char *path);

#endif // UTIL_H
