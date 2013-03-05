all:
	g++ ./Server/server.c -g -Wall -o ./server
	g++ ./Server/server_select.c -g -Wall -o ./server_select
	g++ ./Server/server_multithreading.c -g -Wall -pthread -o ./server_multithreading
	g++ ./Client/client.c -g -Wall -o ./client
