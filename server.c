
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"
#include "udp.h"
#include "tcp.h"

#define MAX_CONNECTIONS 32

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
	struct pollfd *poll_fds = malloc(2 * sizeof(struct pollfd));
	/* initially, set the number of sockets to 2 (tcp listening port to accept connections
	 * and the udp socket to receive packets) */
	int num_sockets = 2;
	// the maximum number of sockets that can be handled by the server
	int max_sockets = 2;
	int rc;
	
	// make the server wait for new connections on the listening socket
	rc = listen(tcp_listen_fd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen failed");
	
	// add the UDP socket to the poll_fds array
	poll_fds[0].fd = udp_socket_fd;
	poll_fds[0].events = POLLIN;

	// add the listening socket to the poll_fds array
	poll_fds[1].fd = tcp_listen_fd;
	poll_fds[1].events = POLLIN;
	
	while (1) {
		// wait for an event on any of the sockets
		rc = poll(poll_fds, num_sockets, -1);
		DIE(rc < 0, "poll failed");
		
		// check which socket has an event
		for (int i = 0; i < num_sockets; i++) {
			// check from which socket we can read data 
			if (poll_fds[i].revents & POLLIN) {
				// check if the server received a new UDP packet
				if(poll_fds[i].fd == udp_socket_fd) {
					// struct used to hold the client address and port
					struct sockaddr_in udp_client_addr;
					// struct used to hold the received UDP packet
					struct udp_packet udp_packet;
					// read the UDP packet into the received_packet struct and get the client address
					int bytes_received = recv_udp_packet(udp_socket_fd, &udp_packet, &udp_client_addr);

					struct tcp_packet tcp_packet;
					// set the TCP packet fields
					tcp_packet.total_len = bytes_received;
					tcp_packet.ip = udp_client_addr.sin_addr.s_addr;
					tcp_packet.port = udp_client_addr.sin_port;
					strcpy(tcp_packet.topic, udp_packet.topic);
					tcp_packet.data_type = udp_packet.data_type;
					strcpy(tcp_packet.content, udp_packet.content);
					
					for (int j = 1; j < num_sockets; j++) {
						// check if the socket is a TCP socket
						if (poll_fds[j].fd != tcp_listen_fd) {
							// send the UDP packet to the TCP socket
							send_tcp_packet(poll_fds[j].fd, &tcp_packet);
						}
					}
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
					// we received data from an existing connection
					int rc = recv_packet(poll_fds[i].fd, &received_packet,
										sizeof(received_packet));
					DIE(rc < 0, "recv failed");
			
					if (rc == 0) {
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						close(poll_fds[i].fd);
			
						// eliminate the socket from the poll_fds array
						for (int j = i; j < num_sockets - 1; j++) {
						poll_fds[j] = poll_fds[j + 1];
						}
						
						// decrease the number of sockets
						num_sockets--;
					} else {
						printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n",
								poll_fds[i].fd, received_packet.content);
						
						for (int j = 1; j < num_sockets; j++) {
							if(poll_fds[i].fd != poll_fds[j].fd)
								send_packet(poll_fds[j].fd, &received_packet, sizeof(received_packet));
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
	int rc = sscanf(argv[2], "%hu", &port);
	DIE(rc != 1, "The given port is not valid");
	
	// create a new network TCP socket
	const int tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_listen_fd < 0, "could not create socket");
	
	// create a struct to hold the server address
	struct sockaddr_in tcp_server_adress;
	socklen_t socket_len = sizeof(struct sockaddr_in);
	
	// make the socket reusable
	const int enable = 1;
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
	int enable = 1;
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
	
	// open the server
	run_server(udp_socket_fd, tcp_listen_fd);
	
	// close the socket
	close(tcp_listen_fd);
	
	return 0;
}
	