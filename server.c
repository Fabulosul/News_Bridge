
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
#include <netinet/tcp.h>

#include "helpers.h"
#include "udp.h"
#include "tcp.h"
#include "server.h"
#include "list.h"

#define MAX_CONNECTIONS 1024
#define MAX_INPUT_BUFFER_SIZE 1500
#define TOPIC_SIZE 51 // 50 + 1 for the null terminator
 

bool matches(char *topic, char *pattern) {
    char topic_copy[TOPIC_SIZE];
	char pattern_copy[TOPIC_SIZE];

	// make a copy of the topic and pattern strings
	memcpy(topic_copy, topic, TOPIC_SIZE);
	memcpy(pattern_copy, pattern, TOPIC_SIZE);
	
	char *topic_ptr_copy;
    char *pattern_ptr_copy;

    char *topic_word = strtok_r(topic_copy, "/", &topic_ptr_copy);
    char *pattern_word = strtok_r(pattern_copy, "/", &pattern_ptr_copy);

    while (topic_word != NULL && pattern_word != NULL) {
        if (strcmp(pattern_word, "*") == 0) {
            pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
            while (topic_word != NULL && pattern_word != NULL &&
                    strcmp(topic_word, pattern_word) != 0) {
                topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
            }
            if(pattern_word == NULL) {
                return true;
            }
            continue;
        }
        if (strcmp(pattern_word, "+") == 0) {
            topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
            pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
            continue;
        }
        if (strcmp(topic_word, pattern_word) != 0) {
            return false;
        }
        topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
        pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
    }

    if(pattern_word != NULL && strcmp(pattern_word, "*") == 0 && topic_word == NULL) {
        return true;
    }

    return topic_word == NULL && pattern_word == NULL;
}

bool is_connected(struct tcp_client *tcp_clients, int nr_tcp_clients, char *id) {
	// find the client in the tcp_clients array
	for (int i = 0; i < nr_tcp_clients; i++) {
		if (strcmp(tcp_clients[i].id, id) == 0) {
			return tcp_clients[i].is_connected;
		}
	}
	
	return false;
}

void print_topics(struct tcp_client *tcp_clients, int nr_tcp_clients) {
	printf("TCP clients %d\n", nr_tcp_clients);
    printf("=== Subscribed Topics ===\n");
    for (int i = 0; i < nr_tcp_clients; i++) {
        struct tcp_client client = tcp_clients[i];
        printf("Client ID: %s\n", client.id);

        if (client.topics == NULL || is_empty(client.topics)) {
            printf("  No topics subscribed.\n");
            continue;
        }

        struct node *current_node = client.topics->head;
        if (current_node == NULL) {
            printf("  No topics subscribed.\n");
            continue;
        }

        do {
            char *topic = (char *)current_node->data;
            printf("  - %s\n", topic);
            current_node = current_node->next;
        } while (current_node != client.topics->head);
    }
    printf("=========================\n");
}


void send_message_to_subscribers(struct tcp_client **tcp_clients, int nr_tcp_clients,
		struct tcp_packet *tcp_packet) {
	char *topic = tcp_packet->topic;
	// iterate through the list of TCP clients
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the client is subscribed to the topic
		struct tcp_client *client = &(*tcp_clients)[i];
		// if the client is not subscribed to any topic, skip it
		if(client->topics == NULL || is_empty(client->topics) || 
			!is_connected(*tcp_clients, nr_tcp_clients, client->id)) {
			continue;
		}

		struct node *current_node = client->topics->head;
		do {
			// check if the topic matches the client's current topic
			char *client_topic = (char *)current_node->data;
			if (matches(topic, client_topic)) {
				// send the message to the client
				int rc = send_tcp_packet(client->socket_fd, tcp_packet);
				DIE(rc < 0, "send failed");
			}
			current_node = current_node->next;
		} while (current_node != client->topics->head);
	}
}

bool add_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, char *id) {
	// find the client in the tcp_clients array
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the current client mathes the given id
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			// add null terminator
			topic[TOPIC_SIZE] = '\0';
			// add the topic to the client's list of topics
			insert_node((*tcp_clients)[i].topics, topic, TOPIC_SIZE);
			return true;
		}
	}
	
	return false;
}

bool remove_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, char *id) {
	// find the client in the tcp_clients array
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the current client mathes the given id
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			// remove the topic from the client's list of topics
			topic[TOPIC_SIZE] = '\0';
			delete_node((*tcp_clients)[i].topics, topic, TOPIC_SIZE);
			return true;
		}
	}
	
	return false;
}

char *find_client_id(struct tcp_client *tcp_clients, int nr_tcp_clients, int socket_fd) {
	for (int i = 0; i < nr_tcp_clients; i++) {
		if (tcp_clients[i].socket_fd == socket_fd) {
			return tcp_clients[i].id;
		}
	}
	
	return NULL;
}

void add_tcp_client(struct tcp_client **tcp_clients, int *nr_tcp_clients, int *max_clients,
		int socket_fd, char *id) {
	// check if the client is in array
	for (int i = 0; i < *nr_tcp_clients; i++) {
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			(*tcp_clients)[i].socket_fd = socket_fd;
			(*tcp_clients)[i].is_connected = true;
			return;
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
	memcpy((*tcp_clients)[*nr_tcp_clients].id, id, strlen(id) + 1);
	(*tcp_clients)[*nr_tcp_clients].is_connected = true;
	// create a new list to hold the topics for the client
	(*tcp_clients)[*nr_tcp_clients].topics = create_list();
	DIE((*tcp_clients)[*nr_tcp_clients].topics == NULL, "create_list failed");
	
	(*nr_tcp_clients)++;
}

void disconnect_client(struct tcp_client **tcp_clients, int *nr_tcp_clients, char *id) {
	// find the client in the tcp_clients array
	for (int i = 0; i < *nr_tcp_clients; i++) {
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			// set the client as disconnected
			(*tcp_clients)[i].is_connected = false;
			break;
		}
	}
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

void remove_socket(struct pollfd **poll_fds, int *num_sockets, int *max_sockets, int socket_fd) {
	// find the socket in the poll_fds array
	for (int i = 0; i < *num_sockets; i++) {
		if ((*poll_fds)[i].fd == socket_fd) {
			// remove the socket from the poll_fds array and replace it with the last socket
			// close the socket
			close(socket_fd);
			(*poll_fds)[i] = (*poll_fds)[*num_sockets - 1];
			// set the last socket to -1
			(*poll_fds)[*num_sockets - 1].fd = -1;
			(*poll_fds)[*num_sockets - 1].events = 0;
			(*poll_fds)[*num_sockets - 1].revents = 0;
			// decrease the number of sockets
			(*num_sockets)--;
			break;
		}
	}

	// check if we need to decrease the size of the poll_fds array
	if (*num_sockets <= *max_sockets / 2) {
		// halve the size of the poll_fds array
		*max_sockets /= 2;
		*poll_fds = realloc(*poll_fds, (*max_sockets) * sizeof(struct pollfd));
		DIE(*poll_fds == NULL, "realloc failed");
	}
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
		
		// check which socket has an event
		for (int i = 0; i < num_sockets; i++) {
			// check from which socket we can read data 
			if (poll_fds[i].revents & POLLIN) {
				// check if we received data from the standard input
				if(poll_fds[i].fd == STDIN_FILENO) {
					/* read the data from the standard input */
					// create a buffer to hold the data
					char buffer[MAX_INPUT_BUFFER_SIZE];
					memset(buffer, 0, sizeof(buffer));
					int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
					DIE(bytes_read < 0, "read failed");
					
					// eliminate the new line character from the buffer
					buffer[strlen(buffer) - 1] = '\0';

					// check if the command is "exit"
					if (strcmp(buffer, "exit") == 0) {
						// close all the sockets
						for (int j = 0; j < num_sockets; j++) {
							close(poll_fds[j].fd);
						}
						// free poll_fds array
						free(poll_fds);

						exit(0);
					}
					break;
				}
				// check if the server received a new UDP packet
				if(poll_fds[i].fd == udp_socket_fd) {
					// struct used to hold the client address and port
					struct sockaddr_in udp_client_addr;
					// struct used to hold the received UDP packet
					struct udp_packet udp_packet;
					memset(&udp_packet, 0, sizeof(udp_packet));
					// read the UDP packet into the received_packet struct and get the client address
					int bytes_received = recv_udp_packet(udp_socket_fd, &udp_packet, &udp_client_addr);
					DIE(bytes_received < 0, "recv failed");

					// create a TCP packet from the UDP packet data
					struct tcp_packet tcp_packet = create_tcp_packet(udp_client_addr.sin_addr.s_addr,
					udp_client_addr.sin_port, udp_packet.topic, udp_packet.data_type,
					udp_packet.content);

					// send the message to all the subscribers
					send_message_to_subscribers(&tcp_clients, nr_tcp_clients, &tcp_packet);
					
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

					// deactivate Nagle's algorithm
					int enable = 1;
					rc = setsockopt(new_socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, 
									sizeof(int));
					DIE(rc < 0, "setsockopt failed");

					// receive the id of the new client
					struct tcp_packet tcp_packet;
					memset(&tcp_packet, 0, sizeof(tcp_packet));

					int bytes_read = recv_tcp_packet(new_socket_fd, &tcp_packet);
					DIE(bytes_read < 0, "recv failed");

					// check if we received the right packet
					if(strcmp(tcp_packet.content, "Client Id") == 0) {
						// parse the client id from the topic of the packet
						char client_id[11];
						memcpy(client_id, tcp_packet.topic, strlen(tcp_packet.topic) + 1);
						DIE(client_id == NULL, "client id is null");
						
						if(is_connected(tcp_clients, nr_tcp_clients, client_id)) {
							// the client is already connected
							printf("Client %s already connected.\n", client_id);

							// close the socket
							close(new_socket_fd);

							continue;
						}

						add_tcp_client(&tcp_clients, &nr_tcp_clients, &max_tcp_clients,
							new_socket_fd, client_id);

						// highlight the new client
						printf("New client %s connected from %s:%d\n",
							client_id,  inet_ntoa(*(struct in_addr *)&tcp_packet.ip), 
							ntohs(tcp_packet.port));

						// add the new socket to the poll_fds array
						add_socket(&poll_fds, &num_sockets, &max_sockets, new_socket_fd);
					}
				} else {
					// we received data from an existing TCP connection
					struct tcp_packet tcp_packet;
					memset(&tcp_packet, 0, sizeof(tcp_packet));
					int bytes_read = recv_tcp_packet(poll_fds[i].fd, &tcp_packet);
					DIE(bytes_read < 0, "recv failed");

					// check if the server was notified that a client has disconnected
					if (bytes_read == 0) {
						char *client_id = find_client_id(tcp_clients, nr_tcp_clients, poll_fds[i].fd);
						DIE(client_id == NULL, "client id is null");

						printf("Client %s disconnected.\n", client_id);
						
						// set the client as disconnected
						disconnect_client(&tcp_clients, &nr_tcp_clients, client_id);

						// close the socket and remove it from the poll_fds array
						remove_socket(&poll_fds, &num_sockets, &max_sockets, poll_fds[i].fd);

						continue;
					}

					char *client_id = find_client_id(tcp_clients, nr_tcp_clients, poll_fds[i].fd);

					if(strcmp(tcp_packet.content, "Subscribe") == 0) {
						// add the topic to the client's list of topics
						add_topic(&tcp_clients, nr_tcp_clients, tcp_packet.topic, client_id);
					} else {
						if(strcmp(tcp_packet.content, "Unsubscribe") == 0) {
							// remove the topic from the client's list of topics
							remove_topic(&tcp_clients, nr_tcp_clients, tcp_packet.topic, client_id);
						} else {
							printf("Invalid Command\n");
						}
					}
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
	