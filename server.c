//=========== TRABALHO INF 452 ================
//
//	Arthur Gonçalves do Carmo     - 85250
//      Giovanni Vitorassi Moreira    - 85284
//
//=================SERVER======================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_SOCKET_PORT 12000
#define MAX_CLIENTS 10
#define BUFF_SIZE 70
#define LISTEN_BACKLOG 50
    
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

int sv_socket, cl_socket[MAX_CLIENTS], cl_stub, next_id;
int active[MAX_CLIENTS] = { 0 };
char alias[10][200] = { "Cliente 0", "Cliente 1", "Cliente 2", "Cliente 3", "Cliente 4", "Cliente 5", "Cliente 6", "Cliente 7", "Cliente 8", "Cliente 9"};

sockaddr_in sv_address, cl_stub_addr, cl_address[MAX_CLIENTS];
socklen_t cl_addr_size[MAX_CLIENTS];

pthread_t handler[MAX_CLIENTS];
pthread_mutex_t queue_line, active_index;

void send_greet(char *, int);
void list_everyone(char *, int);
void send_to_all(char *, int);
void server_list();
void server_kick(int);
void * handle_client(void *);
void * terminate();
void reset_alias(int);

int kill = 0;

int main(){
	pthread_t k;
	char *greet = (char *) malloc(BUFF_SIZE + 1);
	int *arg = malloc(sizeof(*arg)), start_id;
	
	next_id = 0;
	
	sv_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	sv_address.sin_family = AF_INET;
	sv_address.sin_port = htons(SERVER_SOCKET_PORT);
	memset(&sv_address.sin_zero, 0, sizeof(sv_address.sin_zero));
	
	if(bind(sv_socket, (sockaddr *) &sv_address, sizeof(sockaddr_in)) == -1){
		printf("Port indisponivel\n");
		return 0;
	}
	
	listen(sv_socket, LISTEN_BACKLOG);
	
	pthread_create(&k, NULL, terminate, NULL);
	
	printf("Aguardando conexoes no port %d...\n", SERVER_SOCKET_PORT);
	
	do{
		
		
		pthread_mutex_lock(&queue_line);
		pthread_mutex_lock(&active_index);

		start_id = next_id;
		while(active[next_id]){
			pthread_mutex_unlock(&active_index);
			
			++next_id;
			next_id %= MAX_CLIENTS;
			if(next_id == start_id){
				pthread_mutex_lock(&queue_line);
				if(kill) break;
			}
			pthread_mutex_lock(&active_index);
		}
		
		pthread_mutex_unlock(&queue_line);
		pthread_mutex_unlock(&active_index);

		if(kill) break;
		cl_addr_size[next_id] = sizeof(sockaddr_in);
		cl_socket[next_id] = accept(sv_socket, (sockaddr *) &cl_address[next_id], &cl_addr_size[next_id]);
		if(kill) break;
		
		pthread_mutex_lock(&active_index);
		active[next_id] = 1;
		pthread_mutex_unlock(&active_index);

		send_greet(greet, next_id);
		
		*arg = next_id;
		pthread_create(&handler[*arg], NULL, handle_client, arg);
		
	}while(!kill);
	
	pthread_join(k, NULL);
	close(sv_socket);
	
	pthread_exit(NULL);
	return 0;
}

void * handle_client(void * id){
	int client_id = *((int *)id);
	char *msg  = (char *) malloc(BUFF_SIZE + 1);
	char *buff = (char *) malloc(BUFF_SIZE + 1);
	
	sprintf(buff, "%s conectou\n", alias[client_id]);
	printf("%s", buff);
	send_to_all(buff, client_id);
	
	do{
		strncpy(buff, "", BUFF_SIZE);
		strncpy(msg, "", BUFF_SIZE);
		
		recv(cl_socket[client_id], (void *)buff, BUFF_SIZE, 0);
		
		if(strcmp(buff, "bye\n") == 0){
			sprintf(buff, "%s desconectou\n", alias[client_id]);
			printf("%s", buff);
			send_to_all(buff, client_id);
			break;
			
		}else if(strcmp(buff, "list\n") == 0){
			list_everyone(buff, client_id);
			
		}else if(strcmp(buff, "alias\n") == 0){
			recv(cl_socket[client_id], (void *)buff, BUFF_SIZE, 0);
			if(strcmp(buff, "") != 0 && strcmp(buff, "\n") != 0){
				sprintf(msg, "%s mudou o alias para -> %s\n", alias[client_id], buff);
				printf("%s", msg);
				send_to_all(msg, client_id);
				strncpy(alias[client_id], buff, BUFF_SIZE);
			}
		}else if(strcmp(buff, "") == 0){
			sprintf(buff, "%s caiu\n", alias[client_id]);
			printf("%s", buff);
			send_to_all(buff, client_id);
			break;
			
		}else{
			sprintf(msg, "%s disse: %s", alias[client_id], buff);
			send_to_all(msg, client_id);
		}	
	}while(!kill);
	
	shutdown(cl_socket[client_id], 1);
	close(cl_socket[client_id]);
		
	pthread_mutex_lock(&active_index);
	active[client_id] = 0;
	reset_alias(client_id);
	pthread_mutex_unlock(&active_index);
	
	pthread_mutex_unlock(&queue_line);
	return 0;
}

void * terminate(){
	int cl;
	char *c = (char *) malloc(BUFF_SIZE + 1);
	while(1){

		fgets(c, BUFF_SIZE, stdin);
		
		if(strcmp(c, "kill\n") == 0 || strcmp(c, "exit\n") == 0 || strcmp(c, "quit\n") == 0 || strcmp(c, "bye\n") == 0){
		
			kill = 1;
		
			for(cl = 0; cl < MAX_CLIENTS; ++cl)
				server_kick(cl);
			
			cl_stub = socket(AF_INET, SOCK_STREAM, 0);
			pthread_mutex_unlock(&queue_line);
			connect(cl_stub, (sockaddr *) &sv_address, sizeof(sockaddr_in));
			
			close(cl_stub);
			break;
			
		}else if(strcmp(c, "list\n") == 0){
			server_list();
			
		}else if(strcmp(c, "kick\n") == 0){
			printf("Kick client: ");
			scanf(" %d", &cl);
			server_kick(cl);
			sprintf(c, "%s kickado\n", alias[cl]);
			send_to_all(c, MAX_CLIENTS+1);
			strncpy(c, "", BUFF_SIZE);
		}
	}
	return 0;
}

void send_greet(char *greet, int cl){
	sprintf(greet, "Voce recebeu id %d", cl);
	send(cl_socket[cl], (void *)greet, strlen(greet), 0);
}

void send_to_all(char *msg, int id){
	int i;
	pthread_mutex_lock(&active_index);

	for(i = 0; i < MAX_CLIENTS; ++i)
		if(active[i] && i != id)
				send(cl_socket[i], (void *)msg, strlen(msg), 0);
	
	pthread_mutex_unlock(&active_index);
}

void list_everyone(char *buff, int id){
	int i;
	int n =	sprintf(buff, "Clientes conectados:");
	for(i = 0; i < MAX_CLIENTS; ++i)
		if(active[i] && i != id)
			n += sprintf(buff + n, " %d", i);
	sprintf(buff + n, "\n");
	
	send(cl_socket[id], (void *)buff, strlen(buff), 0);
}

void server_list(){
	int i;
	printf("Clientes conectados:");
	for(i = 0; i < MAX_CLIENTS; ++i)
		if(active[i])
			printf(" %s", alias[i]);
	printf("\n");
}

void server_kick(int id){
	char *kick = (char *) malloc(BUFF_SIZE+1);
	
	strncpy(kick, "__sv_exit_", BUFF_SIZE);

	if(active[id]){
		pthread_cancel(handler[id]);
		pthread_mutex_unlock(&active_index);
		pthread_mutex_lock(&active_index);
		active[id] = 0;
		pthread_mutex_unlock(&active_index);
		send(cl_socket[id], (void *)kick, strlen(kick), 0);
		close(cl_socket[id]);
		printf("%s kickado\n", alias[id]);
		reset_alias(id);
		pthread_mutex_unlock(&queue_line);
	}
}

void reset_alias(int id){
	char def_alias[50];
	switch(id){
		case 0: strcpy(def_alias, "Cliente 0"); break;
		case 1: strcpy(def_alias, "Cliente 1"); break;
		case 2: strcpy(def_alias, "Cliente 2"); break;
		case 3: strcpy(def_alias, "Cliente 3"); break;
		case 4: strcpy(def_alias, "Cliente 4"); break;
		case 5: strcpy(def_alias, "Cliente 5"); break;
		case 6: strcpy(def_alias, "Cliente 6"); break;
		case 7: strcpy(def_alias, "Cliente 7"); break;
		case 8: strcpy(def_alias, "Cliente 8"); break;
		case 9: strcpy(def_alias, "Cliente 9"); break;
	}
}
