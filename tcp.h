#ifndef TCP_H
#define TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <math.h>

#include "helpers.h"

struct tcp_packet {
    int content_len;
    int topic_len;
    int ip;
    short int port;
    char topic[51];
    unsigned char data_type;
    char content[1501];
};

#define SHORT_REAL_PAYLOAD_SIZE 2
#define INT_PAYLOAD_SIZE 5
#define FLOAT_PAYLOAD_SIZE 6


/**
 * @brief Receives a TCP packet from a socket and fills the data into the packet struct.
 * 
 * @param tcp_socket_fd the file descriptor of the TCP socket from which data is received
 * @param packet pointer to a tcp_packet struct where the received data will be stored
 * 
 * @return The number of bytes of the packet received or -1 on error
 */
int recv_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet);

/**
 * @brief Sends a TCP packet from a tcp_packet struct to a socket.
 * 
 * @param tcp_client_fd the file descriptor of the TCP socket to which data is sent
 * @param packet pointer to a tcp_packet struct that has the data to be sent
 * 
 * @return The number of bytes of the packet sent or -1 on error
 */
int send_tcp_packet(int tcp_client_fd, struct tcp_packet *packet);

/**
 * @brief Creates a TCP packet using the given parameters.
 * 
 * @param ip the IP address of the sender
 * @param port the port number of the sender
 * @param topic the message topic
 * @param data_type the type of data stored in the content
 * @param content the content of the message
 * 
 * @return A tcp_packet struct filled with the given parameters
 */
struct tcp_packet create_tcp_packet(int ip, short int port, char *topic, 
    unsigned char data_type, char *content);

#endif
