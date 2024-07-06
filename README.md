Per compilare il server e il client usare:
Server: 'gcc -Wall -o ~/bin/myFTserver /home/matteo/Documents/SO2/projectMyFileTransferApp/myFTserver.c -lpthread'
Client: 'gcc -Wall -o ~/bin/myFTclient /home/matteo/Documents/SO2/projectMyFileTransferApp/myFTclient.c'

Per eseguire il Client usare:
myFTclient -w -a server_address -p port  -f local_path/filename_local -o remote_path/filename_remote
-w: andiamo a scrivere un file sul server (PUT)
-a: segue l'indirizzo ip
-p: segue la porta
-f: segue il path del file sorgente locale (assoluto oppure relativo rispetto alla cwd)
-o: segue il path del file sul server (relativo alla cartella di lavoro del server -- il server controllerà che non si possa scrivere al di fuori della sua cartella)

myFTclient -w -a server_address -p port  -f local_path/filename_local
se non c'è -o PATH_REMOTO si assume PATH_REMOTO = PATH_LOCALE


myFTclient -r -a server_address -p port  -f remote_path/filename_remote -o local_path/filename_local
-r: andiamo a leggere un file dal server
questa volta -f indica il PATH_REMOTO e -o indica il PATH_LOCALE

myFTclient -r -a server_address -p port  -f remote_path/filename_remote
non c'è -o PATH_REMOTO si assume PATH_REMOTO = PATH_LOCALE

myFTclient -l -a server_address -p port  -f remote_path/
-l: andiamo a visualizzare il contenuto di una cartella remota
-f: PATH_REMOTO deve essere una cartella

Per eseguire il Server usare:
myFTserver -a server_address -p server_port -d ft_root_directory
-a: imposta l'indirizzo ip di ascolto del server
-p: imposta la porta di ascolto del server
-d: imposta la root directory del server
