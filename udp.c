#include "udp.h"


int recv_udp_packet(int udp_socket_fd, struct udp_packet *packet, 
                    struct sockaddr_in *udp_client_addr) {
    socklen_t udp_client_len = sizeof(struct sockaddr_in);
    int bytes_received = recvfrom(udp_socket_fd, packet, sizeof(struct udp_packet), 0,
                      (struct sockaddr *)udp_client_addr, &udp_client_len);
    DIE(bytes_received < 0, "recvfrom failed");

    return bytes_received;
}
