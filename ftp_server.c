#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "ftp_common.h"

static volatile int keepRunning = 1;

void sigint_handler(int dummy) {
	keepRunning = 0;
}

int init_server();

void ftp_service_listen(int socket_desc);

void listen_client_cli(int sclient, char *client_ip_addr, char *buffer);

void stop_server(int socket_desc);

void *client_handler(void *arguments);

typedef struct {
	int client_sock;
	struct sockaddr_in pt_client;
} client_handler_args;

int main() {

	signal_interceptor();

	int socket_desc = init_server();

	ftp_service_listen(socket_desc);

	stop_server(socket_desc);

	return 0;
}

int init_server() {

	int socket_desc = socket(PF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		perror("Unable to create socket");
		exit(1);
	}
	puts("Socket created");

	// Prepare the sockaddr_in structure
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_MASTER_PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("Unable to bind port");
		exit(1);
	}
	puts("Bind done");

	if (listen(socket_desc, 10) == -1) {
		perror("Unable to listen");
		exit(1);
	}


	return socket_desc;
}

void ftp_service_listen(int socket_desc) {
	struct sockaddr_in pt_client; // the client's endpoint information
	socklen_t len;
	int sclient; // the client's pipe descriptor


	printf("Server listening on port %d\n", SERVER_MASTER_PORT);
	while (keepRunning) {

		sclient = accept(socket_desc, (struct sockaddr *) &pt_client, &len);
		if (sclient < 0) {
			perror("Connection failed");
		} else {
			puts("Connection accepted");

			pthread_t thread_id;
			client_handler_args args = {sclient, pt_client};

			if (pthread_create(&thread_id, NULL, client_handler, (void *) &args) < 0) {
				perror("Could not create a new thread !");
				//exit(1);
			} else {
				puts("A new thread has been created for the client !");

			}

		}
	}
}

/**
 *
 * @param arguments A client_handler_args structure
 * @return
 */
void *client_handler(void *arguments) {
	// Get the socket descriptor
	client_handler_args *args = arguments;


	char buffer[CLIENT_BUFFER_LENGTH];
	//close(sock);
	char *client_ip_addr = inet_ntoa(args->pt_client.sin_addr);
	printf("New client - %s\n", client_ip_addr);
	bzero(buffer, CLIENT_BUFFER_LENGTH + 1);

	listen_client_cli(args->client_sock, client_ip_addr, buffer);

	printf("Close Client - %s\n", client_ip_addr);
	close(args->client_sock);
	pthread_exit(NULL);

}

void listen_client_cli(int sclient, char *client_ip_addr, char *buffer) {
	while (keepRunning && read(sclient, buffer, CLIENT_BUFFER_LENGTH) > 0) {
		printf("From %s: [%s]\n", client_ip_addr, buffer);
		if (strcmp(buffer, "exit") == 0) {
			printf("%s asked to close the connection\n", client_ip_addr);
			break;
		}
		write(sclient, buffer, CLIENT_BUFFER_LENGTH);
		bzero(buffer, CLIENT_BUFFER_LENGTH + 1);
	}
}

void stop_server(int socket_desc) {
	close(socket_desc);
	printf("Server killed\n");
}
