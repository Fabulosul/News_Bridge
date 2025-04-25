#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "tcp.h"

int recv_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet) {
    int total_len;
    int bytes_read = 0;
    int total_bytes_read = 0;

    // read the first 4 bytes to get the total length of the packet
    do {
        bytes_read = recv(tcp_socket_fd, (char *)&total_len + total_bytes_read, 
                              sizeof(total_len) - total_bytes_read, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "recv failed");
            return -1;
        }
        total_bytes_read += bytes_read;
    } while (total_bytes_read < sizeof(total_len));

    // convert the total length to the right format
    total_len = ntohl(total_len);

    // read the rest of the packet
    while (total_bytes_read < total_len) {
        bytes_read = recv(tcp_socket_fd, (char *)packet + total_bytes_read, 
                              total_len - total_bytes_read, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "recv failed");
            return -1;
        }
        total_bytes_read += bytes_read;
    }

    return total_len;
}

int send_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet) {
    int packet_len = packet->total_len;
    int bytes_sent = 0;
    int bytes_remaining = packet_len;

    while (bytes_remaining > 0) {
        bytes_sent = send(tcp_socket_fd, (char *)packet + packet_len - bytes_remaining, 
                            bytes_remaining, 0);
        if (bytes_sent < 0) {
            fprintf(stderr, "send failed");
            return -1;
        }
        bytes_remaining -= bytes_sent;
    }

    return packet_len;
}

struct tcp_packet create_tcp_packet(int ip, short int port, char *topic, 
                                    unsigned int data_type, char *content) {
    // Create a TCP packet with the given parameters
    struct tcp_packet packet;
    packet.ip = ip;
    packet.port = port;
    
    strncpy(packet.topic, topic, strlen(topic));
    packet.topic[strlen(packet.topic)] = '\0';
    
    packet.data_type = data_type;
    
    strncpy(packet.content, content, strlen(content));
    packet.content[strlen(packet.content)] = '\0';

    /* the total length includes the the IP, port, topic, data type, and content
     * plus 2 bytes for the null terminators for topic and content */
    packet.total_len = sizeof(packet.ip) + sizeof(packet.port) + 
                       strlen(packet.topic) + sizeof(packet.data_type) + 
                       strlen(packet.content) + 2;
    
    return packet;
}