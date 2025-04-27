#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "tcp.h"

#define SHORT_REAL_PAYLOAD_SIZE 2
#define INT_PAYLOAD_SIZE 5
#define FLOAT_PAYLOAD_SIZE 6


int recv_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet) {
    int topic_len, content_len;
    int bytes_read = 0;
    int total_bytes_read = 0;
    int bytes_remaining = sizeof(packet->topic_len) + sizeof(packet->content_len) +
        sizeof(packet->ip) + sizeof(packet->port);

    // read the length of the topic and the length of the content
    while (bytes_remaining > 0) {
        bytes_read = recv(tcp_socket_fd, (char *)packet + total_bytes_read, 
                              bytes_remaining, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "recv failed");
            return -1;
        }
        bytes_remaining -= bytes_read;
        total_bytes_read += bytes_read;
    } 

    // convert the lengths from network byte order to host byte order
    topic_len = ntohl(packet->topic_len);
    content_len = ntohl(packet->content_len);

    bytes_remaining = topic_len;

    // read the topic
    while (bytes_remaining > 0) {
        bytes_read = recv(tcp_socket_fd, (char *)packet + total_bytes_read, 
                              bytes_remaining, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "recv failed");
            return -1;
        }
        bytes_remaining -= bytes_read;
        total_bytes_read += bytes_read;
    }

    bytes_remaining = sizeof(packet->data_type) + content_len;
    total_bytes_read = 0;

    // read the data type and the content
    while (bytes_remaining > 0) {
        bytes_read = recv(tcp_socket_fd, (char *)&packet->data_type + total_bytes_read,
                            bytes_remaining, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "recv failed");
            return -1;
        }
        bytes_remaining -= bytes_read;
        total_bytes_read += bytes_read;
    }

    total_bytes_read = sizeof(packet->topic_len) + sizeof(packet->content_len) +
        sizeof(packet->ip) + sizeof(packet->port) + topic_len + sizeof(packet->data_type) +
        content_len;

    return total_bytes_read;
}

int send_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet) {
    int topic_len = ntohl(packet->topic_len);
    int content_len = ntohl(packet->content_len);
    int bytes_sent = 0;
    int total_bytes_sent = 0;
    int bytes_remaining = sizeof(packet->topic_len) + sizeof(packet->content_len) +
        sizeof(packet->ip) + sizeof(packet->port);
    
    // send the length of the topic, the length of the content, the ip and the port
    while (bytes_remaining > 0) {
        bytes_sent = send(tcp_socket_fd, (char *)packet + total_bytes_sent, 
                            bytes_remaining, 0);
        if (bytes_sent < 0) {
            fprintf(stderr, "send failed");
            return -1;
        }
        bytes_remaining -= bytes_sent;
        total_bytes_sent += bytes_sent;
    }
    
    bytes_remaining = topic_len;

    // send the topic
    while (bytes_remaining > 0) {
        bytes_sent = send(tcp_socket_fd, (char *)packet + total_bytes_sent, 
                            bytes_remaining, 0);
        if (bytes_sent < 0) {
            fprintf(stderr, "send failed");
            return -1;
        }
        bytes_remaining -= bytes_sent;
        total_bytes_sent += bytes_sent;
    }

    bytes_remaining = sizeof(packet->data_type) + content_len;
    total_bytes_sent = 0;
    
    // send the data type and the content
    while (bytes_remaining > 0) {
        bytes_sent = send(tcp_socket_fd, (char *)&packet->data_type + total_bytes_sent,
                            bytes_remaining, 0);
        if (bytes_sent < 0) {
            fprintf(stderr, "send failed");
            return -1;
        }
        bytes_remaining -= bytes_sent;
        total_bytes_sent += bytes_sent;
    }

    total_bytes_sent = sizeof(packet->topic_len) + sizeof(packet->content_len) +
        sizeof(packet->ip) + sizeof(packet->port) + topic_len + sizeof(packet->data_type) +
        content_len;

    return total_bytes_sent;
}

struct tcp_packet create_tcp_packet(int ip, short int port, char *topic, 
                                    unsigned char data_type, char *content) {
    // Create a TCP packet with the given parameters
    struct tcp_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.ip = ip;
    packet.port = port;
    packet.data_type = data_type;
    
    memcpy(packet.topic, topic, sizeof(packet.topic));
    packet.topic[strlen(packet.topic)] = '\0';
    packet.topic_len = htonl(strlen(packet.topic) + 1);
    memcpy(packet.content, content, sizeof(packet.content));

    printf("data_type: %u\n", data_type);

    switch(data_type) {
        // Int
        case 0:
            packet.content_len = htonl(INT_PAYLOAD_SIZE);
            break;
        // Short_real
        case 1:
            packet.content_len = htonl(SHORT_REAL_PAYLOAD_SIZE);
            break;
        // Float
        case 2:
            packet.content_len = htonl(FLOAT_PAYLOAD_SIZE);
            break;
        // String
        case 3:
            packet.content_len = htonl(strlen(packet.content) + 1);
            break;
        default:
            fprintf(stderr, "Invalid data type\n");
            exit(EXIT_FAILURE);
    }
    
    return packet;
}