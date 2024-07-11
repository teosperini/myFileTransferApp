import socket
import sys
import threading
import time

# Configurazione del server
server_address = ('127.0.0.1', 8080)  # Modifica indirizzo e porta se necessario

# Dati da inviare
file_to_write = "testfile.txt"
data_to_write = "Hello, World!\n"

# Funzione per inviare il comando PUT al server
def send_put_command(client_name):
    try:
        # Connessione al server
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect(server_address)

        # Costruzione del comando PUT
        command = f"PUT {file_to_write}"

        # Invio del comando PUT al server
        print(f"{client_name}: Invio comando PUT")
        client_socket.sendall(command.encode())

        # Simulazione di un piccolo ritardo prima di inviare i dati
        time.sleep(0.5)

        # Invio dei dati al server
        print(f"{client_name}: Invio dati")
        client_socket.sendall(data_to_write.encode())

        # Chiusura della connessione
        client_socket.close()
        print(f"{client_name}: Connessione chiusa")

    except Exception as e:
        print(f"{client_name}: Errore durante l'esecuzione del client: {e}")
        sys.exit(1)

# Creazione dei client e invio dei comandi PUT
try:
    while(True):
        # Creazione dei thread per i clienti
        thread1 = threading.Thread(target=send_put_command, args=("Client1",))
        thread2 = threading.Thread(target=send_put_command, args=("Client2",))

        # Avvio dei thread
        thread1.start()
        thread2.start()

        # Attesa della terminazione dei thread
        thread1.join()
        thread2.join()

except Exception as e:
    print(f"Errore durante l'esecuzione dei client: {e}")
    sys.exit(1)
