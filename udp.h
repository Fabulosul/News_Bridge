#ifndef UDP_H
#define UDP_H

#include <arpa/inet.h>

#include "helpers.h"

struct udp_packet {
    char topic[50];
    unsigned char data_type;
    char content[1500];
};


/**
 * @brief Read a UDP packet from a given socket into a udp_packet struct.
 * 
 * @param udp_socket_fd the file descriptor of the UDP socket
 * @param packet a pointer to the struct where the packet data will be stored
 * @param udp_client_addr a pointer that will be filled with the address of the UDP client
 * 
 * @return the number of bytes received on the socket or -1 if an error occurred
 */
int recv_udp_packet(int udp_socket_fd, struct udp_packet *packet, 
                    struct sockaddr_in *udp_client_addr);

#endif
