all: client server

server: server.c
	gcc server.c -o server -lpthread -Wall

client: client.c
	gcc client.c -o client -lpthread -Wall
