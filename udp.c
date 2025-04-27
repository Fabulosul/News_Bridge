#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include "udp.h"


int recv_udp_packet(int udp_socket_fd, struct udp_packet *packet, 
                    struct sockaddr_in *udp_client_addr) {
    socklen_t udp_client_len = sizeof(struct sockaddr_in);
    int bytes_received = recvfrom(udp_socket_fd, packet, sizeof(struct udp_packet), 0,
                      (struct sockaddr *)udp_client_addr, &udp_client_len);
    if (bytes_received < 0) {
        fprintf(stderr, "recvfrom failed");
        return -1;
    }

    return bytes_received;
}

