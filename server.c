#include "server_utils.h"


 
void run_server(int udp_socket_fd, int tcp_listen_fd) {
	// allocate memory for the poll_fds array
	struct pollfd *poll_fds = malloc(NUM_SOCKETS * sizeof(struct pollfd));
	/* initially, set the number of sockets to 3 (the standard input to receive server closure,
	 * the udp socket to receive packets and the tcp listening port to accept new connections) */
	int num_sockets = NUM_SOCKETS;
	// the maximum number of sockets that can be handled by the server
	int max_sockets = NUM_SOCKETS;
	
	// make the server wait for new connections on the listening socket
	int	rc = listen(tcp_listen_fd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen failed");

	// add the standard input to the poll_fds array
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	
	// add the UDP socket to the poll_fds array
	poll_fds[1].fd = udp_socket_fd;
	poll_fds[1].events = POLLIN;

	// add the TCP listening socket to the poll_fds array
	poll_fds[2].fd = tcp_listen_fd;
	poll_fds[2].events = POLLIN;

	// create an array to hold the information about the TCP clients and make it resizable
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
				// the server received data from the standard input
				if(poll_fds[i].fd == STDIN_FILENO) {
					// create a buffer to hold the received data
					char buffer[MAX_INPUT_BUFFER_SIZE];
					memset(buffer, 0, sizeof(buffer));
					
					// read the data from the standard input
					int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
					DIE(bytes_read < 0, "read failed");
					// eliminate the new line character from the buffer
					buffer[strlen(buffer) - 1] = '\0';

					// check if the command is "exit"
					if (strcmp(buffer, "exit") == 0) {						
						// close the UDP socket
						close(udp_socket_fd);

						// close the TCP listening socket
						close(tcp_listen_fd);
						
						// close all the TCP sockets
						for (int j = 0; j < num_sockets; j++) {
							close(poll_fds[j].fd);
						}

						// free poll_fds array
						free(poll_fds);

						// free the tcp_clients array
						for (int j = 0; j < nr_tcp_clients; j++) {
							delete_list(tcp_clients[j].topics);
						}
						free(tcp_clients);

						// exit the program
						exit(EXIT_SUCCESS);
					}
					break;
				}
				// the server received a new UDP packet
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
													udp_client_addr.sin_port, udp_packet.topic, 
													udp_packet.data_type, udp_packet.content);

					// send the message to all subscribers
					send_packet_to_subscribers(&tcp_clients, nr_tcp_clients, &tcp_packet);
					
					break;
				}
				// the server received a new connection request
				if (poll_fds[i].fd == tcp_listen_fd) {
					struct sockaddr_in tcp_client_addr;
					socklen_t tcp_client_len = sizeof(tcp_client_addr);
					
					// accept the new connection and receive the new socket
					const int new_socket_fd = accept(tcp_listen_fd, 
											  (struct sockaddr *)&tcp_client_addr, &tcp_client_len);
					DIE(new_socket_fd < 0, "accept failed");

					// deactivate Nagle's algorithm
					int enable = 1;
					rc = setsockopt(new_socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, 
									sizeof(int));
					DIE(rc < 0, "setsockopt failed");

					// we now need to receive the client id from the new socket
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

						// add the new client to the list of TCP clients
						add_tcp_client(&tcp_clients, &nr_tcp_clients, &max_tcp_clients,
							new_socket_fd, client_id);

						// show that a new client has connected
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
					
					// read the TCP packet into the tcp_packet struct
					int bytes_read = recv_tcp_packet(poll_fds[i].fd, &tcp_packet);
					DIE(bytes_read < 0, "recv failed");

					// check if the server was notified that a client has disconnected
					if (bytes_read == 0) {
						// find the client id associated with the socket to set it as disconnected
						char *client_id = find_client_id(tcp_clients, nr_tcp_clients, poll_fds[i].fd);
						DIE(client_id == NULL, "client id is null");

						printf("Client %s disconnected.\n", client_id);
						
						// set the client as disconnected
						disconnect_client(&tcp_clients, &nr_tcp_clients, client_id);

						// close the socket and remove it from the poll_fds array
						remove_socket(&poll_fds, &num_sockets, &max_sockets, poll_fds[i].fd);

						continue;
					}

					// find the client id associated with the socket
					char *client_id = find_client_id(tcp_clients, nr_tcp_clients, poll_fds[i].fd);

					// check which command was received
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
		exit(EXIT_FAILURE);
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
	struct sockaddr_in udp_server_address;
  
	// create a new UDP socket to receive packets
	int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_socket_fd < 0, "socket failed");
  
	// make the socket reusable
	enable = 1;
	rc = setsockopt(udp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt failed");
  
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
	