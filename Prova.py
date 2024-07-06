import socket
import threading
import time

# Configurazione del server
server_ip = '127.0.0.1'
server_port = 8080  # Usa la porta configurata nel tuo server

# Numero di connessioni da testare
num_connections = 20

def connect_to_server(message):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((server_ip, server_port))
        print("Connessione riuscita")

        # Invia il messaggio al server
        s.sendall(message.encode())

        # Attendi una risposta dal server
        response = s.recv(1024)
        print(f"Risposta dal server: {response.decode()}")

        time.sleep(10)  # Mantieni la connessione aperta per 10 secondi
        s.close()
    except Exception as e:
        print(f"Connessione fallita: {e}")

# Messaggio da inviare al server
message = "ls"  # Puoi cambiare questo messaggio per testare altre risposte del server

# Crea e avvia i thread per effettuare connessioni simultanee
threads = []
for _ in range(num_connections):
    t = threading.Thread(target=connect_to_server, args=(message,))
    t.start()
    threads.append(t)

# Attendi che tutti i thread terminino
for t in threads:
    t.join()
