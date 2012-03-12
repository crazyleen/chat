#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

#include "../common/chat.h"
#include "server.h"

void diep(char *s){
	perror(s);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv){

	int listen_sock;

	struct sockaddr_in listen_addr;
	memset((char *) &listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(CHAT_PORT);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		diep("listen socket");
	if(bind(listen_sock, (struct sockaddr*) &listen_addr, sizeof(listen_addr)) == -1)
		diep("listen bind");
	if(listen(listen_sock, 1024) == -1)
		diep("listen");
	fd_set fds;
	client_store_t cs;
	cs_init(&cs);
	int nfds = listen_sock;

	while(1){
        clear_fds(&cs, &fds);
        FD_SET(listen_sock, &fds);

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0; 
		select(nfds + 1, &fds, NULL, NULL, &tv);
		if(FD_ISSET(listen_sock, &fds)){
			puts("Accepted");
			struct sockaddr_in caddr;
			socklen_t caddr_size = sizeof(struct sockaddr_in);
			int cfd = accept(listen_sock, (struct sockaddr *)&caddr, &caddr_size);
			if(cfd == -1)
				diep("accept");

            write(cfd, "Hello", 6   );
		}
	}
}

void clear_fds(client_store_t *cs, fd_set *fds){
    FD_ZERO(fds);
    for(int i = 0; i < cs->num_clients; i++){
        int sock = cs->clients[i].socket;
        FD_SET(sock, fds);
    }
}

void cs_init(client_store_t * cstore){
	cstore->size = 10;
	cstore->num_clients = 0;
	cstore->clients = calloc(10, sizeof(chat_user_t));
}

int add_client(client_store_t * cs, int sock, const char *username){    
    if(get_user_by_name(cs, username) != NULL){
        return -1;
    }
	if(cs->size == cs->num_clients){
		cs->size = (cs->size * 3 / 2) + 5;
		cs->clients = realloc(cs->clients, cs->size * sizeof(chat_user_t));
	}
    int id = cs->num_clients++;
    cs->clients[id].socket = sock;
    cs->clients[id].username = strdup(username);

	return id;
}

chat_user_t * get_user_by_name(client_store_t *cs, const char *username){
    for(int i = 0; i < cs->num_clients; i++){
        if(strcmp(username, cs->clients[i].username) == 0){
            return  &cs->clients[i];
        }
    }
	return NULL;
}

void delete_client(client_store_t *cs, int sock){
    int id;
    for(id = 0; id < cs->num_clients; id++){
        if(sock == cs->clients[id].socket){
            break;
        }
    }
    cs->num_clients--;
    memmove(cs->clients + id, cs->clients + id + 1, (cs-> num_clients - id) * sizeof(chat_user_t));
}

void cs_free(client_store_t *cs){
	for(int i = 0; i < cs->num_clients; i++){
		free(cs->clients[i].username);
	}
	free(cs->clients);
	free(cs);
}

