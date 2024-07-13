# Nome del compilatore
CC = gcc

# Opzioni di compilazione
CFLAGS = -Wall

# File di output
LIBRARY_OBJS = path_mutex.o util.o server_operations.o
CLIENT = myFTclient
SERVER = myFTserver
ALL = all

# Target di compilazione
all: $(CLIENT) $(SERVER)

path_mutex.o: path_mutex.c path_mutex.h
	$(CC) -c path_mutex.c -o path_mutex.o

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
