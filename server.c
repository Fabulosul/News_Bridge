
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "helpers.h"
#include "udp.h"
#include "tcp.h"
#include "server.h"
#include "list.h"

#define MAX_CONNECTIONS 1024
#define MAX_INPUT_BUFFER_SIZE 1500
#define TOPIC_SIZE 51 // 50 + 1 for the null terminator
 

bool add_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, int socket_fd) {
	// find the client in the tcp_clients array
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the current client mathes the given id
		if ((*tcp_clients)[i].socket_fd == socket_fd) {
			printf("********Clientul %d s-a abonat la topicul %s\n", (*tcp_clients)[i].id, topic);
			// add null terminator
			topic[strlen(topic)] = '\0';
			// add the topic to the client's list of topics
			insert_node((*tcp_clients)[i].topics, topic, TOPIC_SIZE);
			return true;
		}
	}
	
	return false;
}

bool remove_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, int socket_fd) {
	// find the client in the tcp_clients array
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the current client mathes the given id
		if ((*tcp_clients)[i].socket_fd == socket_fd) {
			printf("********Clientul %d s-a dezabonat la topicul %s\n", (*tcp_clients)[i].id, topic);
			// remove the topic from the client's list of topics
			topic[strlen(topic)] = '\0';
			delete_node((*tcp_clients)[i].topics, topic, TOPIC_SIZE);
			return true;
		}
	}
	
	return false;
}

bool add_tcp_client(struct tcp_client **tcp_clients, int *nr_tcp_clients, int *max_clients,
		int socket_fd, int id) {
	
	// check if the client is already in the list
	for (int i = 0; i < *nr_tcp_clients; i++) {
		if ((*tcp_clients)[i].socket_fd == socket_fd) {
			// client already exists, no need to add it again
			return false;
		}
	}
	
	// check if we need to increase the size of the tcp_clients array
	if (*nr_tcp_clients >= *max_clients) {
		// double the size of the tcp_clients array
		*max_clients *= 2;
		*tcp_clients = realloc(*tcp_clients, (*max_clients) * sizeof(struct tcp_client));
		DIE(*tcp_clients == NULL, "realloc failed");
	}

	// add the new client to the tcp_clients array
	(*tcp_clients)[*nr_tcp_clients].socket_fd = socket_fd;
	(*tcp_clients)[*nr_tcp_clients].id = id;
	// create a new list to hold the topics for the client
	(*tcp_clients)[*nr_tcp_clients].topics = create_list();
	DIE((*tcp_clients)[*nr_tcp_clients].topics == NULL, "create_list failed");
	
	(*nr_tcp_clients)++;

	return true;
}

void add_socket(struct pollfd **poll_fds, int *num_sockets, int *max_sockets, int new_fd) {
    // check if we need to increase the size of the poll_fds array
	if (*num_sockets >= *max_sockets) {
        // double the size of the poll_fds array
        *max_sockets *= 2;
        *poll_fds = realloc(*poll_fds, (*max_sockets) * sizeof(struct pollfd));
        DIE(*poll_fds == NULL, "realloc failed");
    }

	// add the new socket to the poll_fds array
    (*poll_fds)[*num_sockets].fd = new_fd;
    (*poll_fds)[*num_sockets].events = POLLIN;
    (*num_sockets)++;
}

void run_server(int udp_socket_fd, int tcp_listen_fd) {
	// allocate memory for the poll_fds array
	struct pollfd *poll_fds = malloc(3 * sizeof(struct pollfd));
	/* initially, set the number of sockets to 3 (tcp listening port to accept connections,
	 * the udp socket to receive packets, and the standard input to close the server) */
	int num_sockets = 3;
	// the maximum number of sockets that can be handled by the server
	int max_sockets = 3;
	int rc;
	
	// make the server wait for new connections on the listening socket
	rc = listen(tcp_listen_fd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen failed");

	// add the standard input to the poll_fds array
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	
	// add the UDP socket to the poll_fds array
	poll_fds[1].fd = udp_socket_fd;
	poll_fds[1].events = POLLIN;

	// add the listening socket to the poll_fds array
	poll_fds[2].fd = tcp_listen_fd;
	poll_fds[2].events = POLLIN;

	// create a vector to hold the information about the TCP clients and make it resizable
	int nr_tcp_clients = 0;
	int max_tcp_clients = 2;
	struct tcp_client *tcp_clients = malloc(max_tcp_clients * sizeof(struct tcp_client));
	DIE(tcp_clients == NULL, "malloc failed");

	while (1) {
		// wait for an event on any of the sockets
		rc = poll(poll_fds, num_sockets, -1);
		DIE(rc < 0, "poll failed");

		printf("Am primit un eveniment de la un socket\n");
		
		// check which socket has an event
		for (int i = 0; i < num_sockets; i++) {
			// check from which socket we can read data 
			if (poll_fds[i].revents & POLLIN) {
				// check if we received data from the standard input
				if(poll_fds[i].fd == STDIN_FILENO) {
					printf("Am primit un mesaj de la standard input\n");
					/* read the data from the standard input */
					// create a buffer to hold the data
					char buffer[MAX_INPUT_BUFFER_SIZE];
					int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
					DIE(bytes_read < 0, "read failed");
					
					// eliminate the new line character from the buffer
					buffer[strlen(buffer) - 1] = '\0';

					// check if the command is "exit"
					if (strcmp(buffer, "exit") == 0) {
						printf("Serverul se inchide...\n");
						// close all the sockets
						for (int j = 0; j < num_sockets; j++) {
							close(poll_fds[j].fd);
						}
						free(poll_fds);
						exit(0);
					}
					break;
				}
				// check if the server received a new UDP packet
				if(poll_fds[i].fd == udp_socket_fd) {
					printf("Am primit un pachet UDP\n");
					// struct used to hold the client address and port
					struct sockaddr_in udp_client_addr;
					// struct used to hold the received UDP packet
					struct udp_packet udp_packet;
					memset(&udp_packet, 0, sizeof(udp_packet));
					// read the UDP packet into the received_packet struct and get the client address
					int bytes_received = recv_udp_packet(udp_socket_fd, &udp_packet, &udp_client_addr);
					DIE(bytes_received < 0, "recv failed");


					// print the received message
					printf("Pachetul UDP primit de la clientul %s, port %d\n",
							inet_ntoa(udp_client_addr.sin_addr), ntohs(udp_client_addr.sin_port));

					// create a TCP packet from the UDP packet data
					struct tcp_packet tcp_packet = create_tcp_packet(udp_client_addr.sin_addr.s_addr,
					udp_client_addr.sin_port, udp_packet.topic, udp_packet.data_type,
					udp_packet.content);

				
					for (int j = 3; j < num_sockets; j++) {
						// check if the socket is a TCP socket
						if (poll_fds[j].fd != tcp_listen_fd) {
							// send the UDP packet to the TCP socket
							send_tcp_packet(poll_fds[j].fd, &tcp_packet);
						}
					}
					break;
				}
				// check if the server received a new connection request
				if (poll_fds[i].fd == tcp_listen_fd) {
					struct sockaddr_in tcp_client_addr;
					socklen_t tcp_client_len = sizeof(tcp_client_addr);
					// accept the new connection and receive the new socket
					const int new_socket_fd = 
							accept(tcp_listen_fd, (struct sockaddr *)&tcp_client_addr, &tcp_client_len);
					DIE(new_socket_fd < 0, "accepting new connection failed");
			
					// add the new socket to the poll_fds array
					add_socket(&poll_fds, &num_sockets, &max_sockets, new_socket_fd);
			
					printf("Noua conexiune de la %s, port %d, socket client %d\n",
							inet_ntoa(tcp_client_addr.sin_addr), ntohs(tcp_client_addr.sin_port),
							new_socket_fd);
				} else {
					// we received data from an existing TCP connection
					printf("Am primit un pachet TCP de la un client\n");
					struct tcp_packet tcp_packet;
					memset(&tcp_packet, 0, sizeof(tcp_packet));
					int rc = recv_tcp_packet(poll_fds[i].fd, &tcp_packet);
					DIE(rc < 0, "recv failed");

					
					if(strcmp(tcp_packet.content, "Client Id") == 0) {
						// parse the client id from the topic of the packet
						uint32_t client_id = ntohl(*(uint32_t *)(((void *)tcp_packet.topic) + 1));

						printf("*****Client id: %d********\n", client_id);

						// add the client to the list of TCP clients
						add_tcp_client(&tcp_clients, &nr_tcp_clients, &max_tcp_clients,
							poll_fds[i].fd, client_id);
					} else {
						if(strcmp(tcp_packet.content, "Subscribe") == 0) {
							// add the topic to the client's list of topics
							add_topic(&tcp_clients, nr_tcp_clients, tcp_packet.topic, poll_fds[i].fd);
						} else {
							if(strcmp(tcp_packet.content, "Unsubscribe") == 0) {
								// remove the topic from the client's list of topics
								remove_topic(&tcp_clients, nr_tcp_clients, tcp_packet.topic, poll_fds[i].fd);
								printf("********Clientul %d s-a dezabonat de la topicul %s\n", poll_fds[i].fd, tcp_packet.topic);
							} else {
								printf("Invalid Command\n");
							}
						}
					}			

					// print the whole packet
					printf("Pachetul TCP primit de la clientul %d\n", poll_fds[i].fd);
					printf("Topic: %s\n", tcp_packet.topic);
					printf("Topic length: %d\n", ntohl(tcp_packet.topic_len));
					printf("Content length: %d\n", ntohl(tcp_packet.content_len));
					printf("IP: %s\n", inet_ntoa(*(struct in_addr *)&tcp_packet.ip));
					printf("Port: %d\n", ntohs(tcp_packet.port));
					printf("Data type: %u\n", tcp_packet.data_type);
					printf("Content: %s\n", tcp_packet.content);
				}
			}
		}
	}
}
	
int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "\n Usage: %s <port>\n", argv[0]);
		return 1;
	}
	
	/* handle the TCP initializations */
	// parse the port from the input
	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "The given port is not valid");
	
	// create a new network TCP socket
	const int tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_listen_fd < 0, "could not create socket");
	
	// create a struct to hold the server address
	struct sockaddr_in tcp_server_adress;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	
	// make the socket reusable
	int enable = 1;
	rc = setsockopt(tcp_listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt failed");
	
	// set the server address to zero
	memset(&tcp_server_adress, 0, socket_len);
	// set the server to work on IPv4
	tcp_server_adress.sin_family = AF_INET;
	// set the port in the network byte order
	tcp_server_adress.sin_port = htons(port);
	// set the server ip address to 0.0.0.0
	tcp_server_adress.sin_addr.s_addr = INADDR_ANY;
	
	// bind the previously created socket to the server address and port
	rc = bind(tcp_listen_fd, (const struct sockaddr *)&tcp_server_adress, sizeof(tcp_server_adress));
	DIE(rc < 0, "bind failed");


	/* handle the UDP initializations */
	int udp_socket_fd;
	struct sockaddr_in udp_server_address;
  
	// create a new UDP socket to receive packets
	if ((udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	  perror("socket creation failed");
	  exit(EXIT_FAILURE);
	}
  
	// make the socket reusable
	enable = 1;
	rc = setsockopt(udp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt failed");
  
	// set the server address to zero
	memset(&udp_server_address, 0, sizeof(udp_server_address));
	// set the server to work on IPv4
	udp_server_address.sin_family = AF_INET;
	// set the server ip address to 0.0.0.0
	udp_server_address.sin_addr.s_addr = INADDR_ANY;
	// set the port in the network byte order
	udp_server_address.sin_port = htons(port);
  
	// bind the previously created socket to the server address and port
	rc = bind(udp_socket_fd, (const struct sockaddr *)&udp_server_address, sizeof(udp_server_address));
	DIE(rc < 0, "bind failed");

	// deactivate the buffering for the standard output
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	
	// open the server
	run_server(udp_socket_fd, tcp_listen_fd);
	
	// close the socket
	close(tcp_listen_fd);
	
	return 0;
}
	