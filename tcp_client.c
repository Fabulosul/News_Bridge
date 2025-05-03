#include "tcp_client_utils.h"


void run_tcp_client(int tcp_socket_fd, char *client_id) {
	// allocate memory for the poll_fds array
	// only use 2 sockets - one for the standard input and one for the TCP socket
	struct pollfd *poll_fds = malloc(NUM_SOCKETS * sizeof(struct pollfd));
	DIE(poll_fds == NULL, "malloc failed");
	
	// socket used to receive messages from the standard input
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	
	// socket used to communicate with the server
	poll_fds[1].fd = tcp_socket_fd;
	poll_fds[1].events = POLLIN;
	
	while(1) {
		// wait for an event on any of the sockets
		int rc = poll(poll_fds, NUM_SOCKETS, -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < NUM_SOCKETS; i++) {
			// check which socket has an event
			if (poll_fds[i].revents & POLLIN) {
				// the client received data from the standard input
				if(poll_fds[i].fd == STDIN_FILENO) {
					// buffer used to hold the message received from the standard input
					char buffer[MAX_BUFFER_SIZE];
					void *ret = memset(buffer, 0, sizeof(buffer));
					DIE(ret == NULL, "memset failed");

					// read the data from the standard input
					int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
					DIE(bytes_read < 0, "read failed");
					// add null terminator to the buffer
					buffer[strlen(buffer) - 1] = '\0';
					
					// extract the command
					char *command = strtok(buffer, " ");

					// check which command was received
					if(strcmp(command, "subscribe") == 0) {
						// extract the topic
						char *topic = strtok(NULL, " ");
						if(topic == NULL) {
							fprintf(stderr, "Error: Topic is missing\n");
							continue;
						}

						// print a suggestive message
						printf("Subscribed to topic %s\n", topic);

						/* create a TCP packet to notify the server that the client has subscribed
						 * to a topic */
						struct tcp_packet subscribe_packet = create_tcp_packet(0, 0,
							topic, 3, "Subscribe");

						// send the TCP packet to the server
						int bytes_sent = send_tcp_packet(tcp_socket_fd, &subscribe_packet);
						DIE(bytes_sent < 0, "send failed");

					} else if(strcmp(command, "unsubscribe") == 0) {
						// extract the topic from the buffer
						char *topic = strtok(NULL, " ");
						if(topic == NULL) {
							fprintf(stderr, "Error: Topic is missing\n");
							continue;
						}
						
						// print a suggestive message
						printf("Unsubscribed from topic %s\n", topic);

						/* create a TCP packet to notify the server that the current client has 
						 * unsubscribed from a topic */
						struct tcp_packet unsubscribe_packet = create_tcp_packet(0, 0,
							topic, 3, "Unsubscribe");
						
						// send the TCP packet to the server
						int bytes_sent = send_tcp_packet(tcp_socket_fd, &unsubscribe_packet);
						DIE(bytes_sent < 0, "send failed");

					} else if(strcmp(command, "exit") == 0) {
						if(strtok(NULL, " ") != NULL) {
							fprintf(stderr, "Error: Invalid command\n");
							continue;
						}
						
						// close the socket
						int rc = close(tcp_socket_fd);
						DIE(rc < 0, "close failed");
						
						// free the poll_fds array
						free(poll_fds);
						
						// exit the program
						exit(EXIT_SUCCESS);

					} else {
						fprintf(stderr, "Error: Invalid command\n");
					}
				}
				// the client received data from the TCP socket
				if(poll_fds[i].fd == tcp_socket_fd) {
					// struct used to hold the received TCP packet
					struct tcp_packet tcp_packet;
					void *ret = memset(&tcp_packet, 0, sizeof(tcp_packet));
					DIE(ret == NULL, "memset failed");

					// read the TCP packet into the tcp_packet struct
					int bytes_read = recv_tcp_packet(tcp_socket_fd, &tcp_packet);
					DIE(bytes_read < 0, "recv failed");

					// the server has closed the connection since we received an empty packet
					if(bytes_read == 0) {
						printf("Client %s disconnected.\n", client_id);
						// close the socket
						int rc = close(tcp_socket_fd);
						DIE(rc < 0, "close failed");
						// free the array
						free(poll_fds);
						// exit the program
						exit(EXIT_SUCCESS);
					}

					// print the received message in a suggestive format
					print_tcp_packet(&tcp_packet);
				}
			}
		}
	}
 }
 
int main(int argc, char *argv[]) {
	if(argc != 4) {
		fprintf(stderr, "Usage: %s <client_id> <server_ip> <server_port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// parse the client id
	char client_id[ID_LENGTH + 1]; // 10 + 1 for the null terminator
	void *ret = memset(client_id, 0, sizeof(client_id));
	DIE(ret == NULL, "memset failed");

	int rc = sscanf(argv[1], "%s", client_id);
	DIE(rc != 1, "Given client id is invalid");

	// check if the client id has the right length
	if(strlen(client_id) > ID_LENGTH) {
		fprintf(stderr, "Client id is too long\n");
		exit(EXIT_FAILURE);
	}

	// add the null terminator to the client id
	client_id[strlen(client_id)] = '\0';

	// parse the port number
	uint16_t port;
	rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// create a new tcp_socket to receive packets
	const int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket_fd < 0, "socket creation failed");

	// deactivate Nagle's algorithm
	int enable = 1;
	rc = setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, 
					sizeof(int));
	DIE(rc < 0, "setsockopt failed");

	// create a struct to hold the server address
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	ret = memset(&serv_addr, 0, socket_len);
	DIE(ret == NULL, "memset failed");
	// set the server to work on IPv4
	serv_addr.sin_family = AF_INET;
	// set the port in the network byte order
	serv_addr.sin_port = htons(port);
	// set the server ip address
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton failed");

	// connect the socket to the server
	rc = connect(tcp_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect failed");

	// create a TCP packet to add the client id to the server
	char topic[TOPIC_SIZE];
	ret = memset(topic, 0, sizeof(topic));
	DIE(ret == NULL, "memset failed");
	
	// copy the client id to the topic
	ret = memcpy(topic, client_id, strlen(client_id) + 1);
	DIE(ret == NULL, "memcpy failed");

	// create a TCP packet to send the client id to the server
	struct tcp_packet new_subscriber_packet = create_tcp_packet(serv_addr.sin_addr.s_addr, 
												htons(port), topic, 3, "Client Id");
	
	// send the TCP packet to the server
	int bytes_sent = send_tcp_packet(tcp_socket_fd, &new_subscriber_packet);
	DIE(bytes_sent < 0, "send failed");

	// deactivate buffering for stdout
	rc = setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	DIE(rc < 0, "setvbuf failed");

	// run the client
	run_tcp_client(tcp_socket_fd, client_id);

	// close the socket
	rc = close(tcp_socket_fd);
	DIE(rc < 0, "close failed");

	return EXIT_SUCCESS;
 }
 