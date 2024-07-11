# Nome del compilatore
CC = gcc

# Opzioni di compilazione
CFLAGS = -Wall

# File di output
LIBRARY = path_semaphore.o
CLIENT = myFTclient
SERVER = myFTserver
ALL = all

# Target di compilazione
all: $(CLIENT) $(SERVER)

$(LIBRARY): path_semaphore.c path_semaphore.h
	$(CC) -c path_semaphore.c -o $(LIBRARY)

$(CLIENT): myFTclient.c
	$(CC) $(CFLAGS) -o $(CLIENT) myFTclient.c

$(SERVER): myFTserver.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $(SERVER) myFTserver.c $(LIBRARY) -lpthread

# Pulizia dei file compilati
clean:
	rm -f $(LIBRARY) $(CLIENT) $(SERVER)

.PHONY: all clean
