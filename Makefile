# Nome del compilatore
CC = gcc

# Opzioni di compilazione
CFLAGS = -Wall

# File di output
LIBRARY_OBJS = path_semaphore.o util.o server_operations.o
CLIENT = myFTclient
SERVER = myFTserver
ALL = all

# Target di compilazione
all: $(CLIENT) $(SERVER)

path_semaphore.o: path_semaphore.c path_semaphore.h
	$(CC) -c path_semaphore.c -o path_semaphore.o

util.o: util.c util.h
	$(CC) -c util.c -o util.o

server_operations.o: server_operations.c server_operations.h
	$(CC) -c server_operations.c -o server_operations.o

$(CLIENT): myFTclient.c util.o
	$(CC) $(CFLAGS) -o $(CLIENT) myFTclient.c util.o

$(SERVER): myFTserver.c $(LIBRARY_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER) myFTserver.c $(LIBRARY_OBJS) -lpthread

# Pulizia dei file compilati
clean:
	rm -f $(LIBRARY_OBJS) $(CLIENT) $(SERVER)

.PHONY: all clean
