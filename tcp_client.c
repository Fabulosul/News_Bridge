#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "helpers.h"
#include "tcp.h"

#define NUM_SOCKETS 2
#define MAX_BUFFER_SIZE 50 + 20 // sizeof(topic) + memory for the actual command(subscribe, unsubscribe...)
 
void print_tcp_packet(struct tcp_packet *tcp_packet) {
	printf("%s:%d - %s", inet_ntoa(*(struct in_addr *)&tcp_packet->ip),
	ntohs(tcp_packet->port), tcp_packet->topic);

	uint8_t sign;
	switch(tcp_packet->data_type) {
		// INT
		case 0: 
			// get the first byte to find out the sign
			sign = *(uint8_t *) tcp_packet->content;
			// the number is positive
			if(sign == 0) {
				printf(" - INT - %u\n", ntohl(*(uint32_t *)(((void *)tcp_packet->content) + 1)));
			}
			// the number is negative
			if(sign == 1) {
				printf(" - INT - -%u\n", ntohl(*(uint32_t *)(((void *)tcp_packet->content) + 1)));
			}
			break;
		// SHORT_REAL
		case 1:
			uint16_t short_number = ntohs(*(uint16_t *)(tcp_packet->content));
			printf(" - SHORT_REAL - %.2f\n", short_number / 100.0);
			break;
		// FLOAT
		case 2:
			// get the components from the content 
			sign = *(uint8_t *) tcp_packet->content;
			uint32_t number = ntohl(*(uint32_t *)(((void *)tcp_packet->content) + 1));
			uint8_t exponent = *(uint8_t *)(((void *)tcp_packet->content) + 5);

			double original_number = number * pow(-1, sign) / pow(10, exponent);

			printf(" - FLOAT - %.4f\n", (float)original_number);

			break;
		// STRING
		case 3:
			printf(" - STRING - %s\n", tcp_packet->content);
			break;
		default:
			fprintf(stderr, "Invalid data type\n");
			break;
	}

}

void run_tcp_client(int tcp_socket_fd) {
	// allocate memory for the poll_fds array
	/* initially, set the number of sockets to 2 (tcp socket to receive packets and 
	 * standard input to close the client) */
	struct pollfd *poll_fds = malloc(NUM_SOCKETS * sizeof(struct pollfd));
	
	// socket used to receive messages from the standard input
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	
	// socket used to receive and send packets
	poll_fds[1].fd = tcp_socket_fd;
	poll_fds[1].events = POLLIN;
	
	while(1) {
		// wait for an event on any of the sockets
		int rc = poll(poll_fds, NUM_SOCKETS, -1);
		DIE(rc < 0, "poll");
	

		for (int i = 0; i < NUM_SOCKETS; i++) {
			// check which socket has an event
			if (poll_fds[i].revents & POLLIN) {
				// check if we received data from the standard input
				if(poll_fds[i].fd == STDIN_FILENO) {
					char buffer[MAX_BUFFER_SIZE];
					memset(buffer, 0, sizeof(buffer));
					// read the data from the standard input
					int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
					DIE(bytes_read < 0, "read failed");
					// eliminate the new line character from the buffer
					buffer[strlen(buffer) - 1] = '\0';
					
					// extract the command
					char *command = strtok(buffer, " ");

					if(strcmp(command, "subscribe") == 0) {
						// extract the topic
						char *topic = strtok(NULL, " ");
						DIE(topic == NULL, "topic is missing");

						// print the topic
						printf("Subscribed to topic %s\n", topic);

						// create a TCP packet to notify the server
						struct tcp_packet subscribe_packet = create_tcp_packet(0, 0,
							topic, 3, "Subscribe");

						// send the TCP packet to the server
						int bytes_sent = send_tcp_packet(tcp_socket_fd, &subscribe_packet);
						DIE(bytes_sent < 0, "send failed");
						
					} else if(strcmp(command, "unsubscribe") == 0) {
						// extract the topic from the buffer
						char *topic = strtok(NULL, " ");
						DIE(topic == NULL, "topic is missing");
						
						// print the topic
						printf("Unsubscribed from topic %s\n", topic);

						// create a TCP packet to notify the server
						struct tcp_packet unsubscribe_packet = create_tcp_packet(0, 0,
							topic, 3, "Unsubscribe");
						
						// send the TCP packet to the server
						int bytes_sent = send_tcp_packet(tcp_socket_fd, &unsubscribe_packet);
						DIE(bytes_sent < 0, "send failed");

					} else if(strcmp(command, "exit") == 0) {
						// print a suggestive message
						printf("The client is closing ...\n");

						// create a TCP packet to notify the server
						struct tcp_packet exit_packet = create_tcp_packet(0, 0,
							"", 3, "Close connection");

						// send the TCP packet to the server to notify the connection is closed
						int bytes_sent = send_tcp_packet(tcp_socket_fd, &exit_packet);
						DIE(bytes_sent < 0, "send failed");
						
						// close the socket
						close(tcp_socket_fd);
						
						// free the array
						free(poll_fds);
						
						// exit the program
						exit(0);

					} else {
						fprintf(stderr, "Invalid command\n");
					}
				}
				// check if we received data from the TCP socket
				if(poll_fds[i].fd == tcp_socket_fd) {
					printf("Received a TCP packet from the server\n");
					// struct used to hold the received TCP packet
					struct tcp_packet tcp_packet;

					// read the TCP packet into the received_packet struct
					int bytes_read = recv_tcp_packet(tcp_socket_fd, &tcp_packet);
					DIE(bytes_read < 0, "recv failed");

					// print the received message
					print_tcp_packet(&tcp_packet);
				}
			}
		}
	
	}
 }
 
int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("\n Usage: %s <id_client > <ip_server> <port_server>\n", argv[0]);
		return 1;
	}

	// parse the client id
	char client_id[11];
	memset(client_id, 0, sizeof(client_id));
	int rc = sscanf(argv[1], "%s", client_id);
	DIE(rc != 1, "Given client id is invalid");

	// parse the port number
	uint16_t port;
	rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// create a new tcp_socket to receive packets
	const int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket_fd < 0, "socket creation failed");

	// create a struct to hold the server address
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	// set the server address
	memset(&serv_addr, 0, socket_len);
	// set the server to work on IPv4
	serv_addr.sin_family = AF_INET;
	// set the port in the network byte order
	serv_addr.sin_port = htons(port);
	// set the server ip address
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	// connect the socket to the server
	rc = connect(tcp_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	// create a TCP packet to add the client id to the server
	char topic[50];
	memset(topic, 0, sizeof(topic));
	
	uint8_t sign = 0;
	uint32_t number = htonl(atoi(client_id));
	
	// copy the sign to the content
	memcpy(topic, &sign, sizeof(uint8_t));
	// copy the client id to the content
	memcpy(((void *)topic) + 1, &number, sizeof(uint32_t));

	struct tcp_packet new_subscriber_packet = create_tcp_packet(serv_addr.sin_addr.s_addr, htons(port),
		topic, 3, "Client Id");
	
	// send the TCP packet to the server
	int bytes_sent = send_tcp_packet(tcp_socket_fd, &new_subscriber_packet);
	DIE(bytes_sent < 0, "send failed");

	// deactivate buffering for stdout
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// run the client
	run_tcp_client(tcp_socket_fd);

	// close the socket
	close(tcp_socket_fd);

	return 0;
 }
 