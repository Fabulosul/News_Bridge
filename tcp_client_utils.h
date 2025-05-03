#ifndef TCP_CLIENT_UTILS_H
#define TCP_CLIENT_UTILS_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "helpers.h"
#include "tcp.h"

#define NUM_SOCKETS 2 // stdin + tcp socket
#define MAX_BUFFER_SIZE 50 + 20 // sizeof(topic) + memory for the actual command(ex: "subscribe")
#define TOPIC_SIZE 51 // 50 + 1 for the null terminator
#define ID_LENGTH 10

/**
 * @brief Prints a TCP packet in the format: <IP>:<PORT> - <TOPIC> - <DATA_TYPE> - <MESSAGE>
 * 
 * @param tcp_packet pointer to the TCP packet structure to be printed
 */
void print_tcp_packet(struct tcp_packet *tcp_packet);

#endif