//=========== TRABALHO INF 452 ================
//
//	Arthur Gonçalves do Carmo     - 85250
//      Giovanni Vitorassi Moreira    - 85284
//
//=================CLIENT======================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 50
#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_SOCKET_PORT 12000

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

void * listener_f();

int cl_socket, kill_listener = 0, set_alias = 0;
sockaddr_in sv_address;

int main(){
	pthread_t listener;
	char *buff = (char *) malloc(BUFF_SIZE + 1);
	char *server_response = (char *) malloc(BUFF_SIZE + 1);
	char *sv_iaddr = (char *) malloc( 16 );
	char opt;
	
	strcpy(sv_iaddr, SERVER_IP_ADDRESS);
	
	cl_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	sv_address.sin_family = AF_INET;
	sv_address.sin_addr.s_addr = inet_addr(sv_iaddr);
	sv_address.sin_port = htons(SERVER_SOCKET_PORT);
	memset(&sv_address.sin_zero, 0, sizeof(sv_address.sin_zero));
	
	while(connect(cl_socket, (sockaddr *) &sv_address, sizeof(sockaddr_in)) == -1){
		printf("Servidor em %s indisponivel\nTentar outro ip[s/N]: ", sv_iaddr);
		scanf(" %c", &opt);
		if(opt == 's' || opt == 'S'){
			printf("Novo ip: ");
			scanf("%s", sv_iaddr);
			sv_address.sin_addr.s_addr = inet_addr(sv_iaddr);
		}else{
			shutdown(cl_socket, 1);
			close(cl_socket);	
			return 0;
		}
	}
	
	if(recv(cl_socket, (void *) server_response, BUFF_SIZE, 0) == -1){
		printf("Servidor indisponivel\n");
		exit(EXIT_FAILURE);
	}
	
	printf("%s\n", server_response);
	
	pthread_create(&listener, NULL, listener_f, NULL);

	do{
		strncpy(buff, "", BUFF_SIZE);
		fgets(buff, BUFF_SIZE, stdin);
		if(strcmp(buff, "alias\n") == 0){ set_alias = 1; }
		send(cl_socket, (void *)buff, strlen(buff), 0);
		if(set_alias){
			set_alias = 0;
			printf("Novo alias: ");
			strncpy(buff, "", BUFF_SIZE);
			fgets(buff, BUFF_SIZE, stdin);
			buff[strlen(buff) - 1] = '\0';
			send(cl_socket, (void *)buff, strlen(buff), 0);
		}	
	}while(strcmp(buff, "bye\n"));
	
	kill_listener = 1;
	
	pthread_cancel(listener);
	
	shutdown(cl_socket, 1);
	close(cl_socket);
	
	return 0;
}

void * listener_f(){	
	char *server_response = (char *) malloc(BUFF_SIZE + 1);
	while(!kill_listener){
		if(recv(cl_socket, (void *) server_response, BUFF_SIZE, 0) != -1){
			if(strcmp(server_response, "__sv_exit_") == 0){
				printf("Servidor parou\n");
				shutdown(cl_socket, 1);
				close(cl_socket);
				exit(EXIT_SUCCESS);
			}else{
				printf("%s", server_response);
				strncpy(server_response, "", BUFF_SIZE);
			}
		}
	}
	return 0;
}
