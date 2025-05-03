#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>

#include "udp.h"
#include "tcp.h"
#include "helpers.h"
#include "doubly_linked_list.h"

struct tcp_client {
    int socket_fd; // the socket file descriptor of the client
    char id[11]; // the id of the client
    bool is_connected; // true if the client is connected, false otherwise
    struct doubly_linked_list *topics; // list of topics to which the client is subscribed
};

#define TOPIC_SIZE 51 // 50 + 1 for the null terminator
#define MAX_CONNECTIONS 1024
#define MAX_INPUT_BUFFER_SIZE 1000
#define NUM_SOCKETS 3 // the number of sockets that the server can handle initially


/**
 * @brief Adds a new socket to the poll_fds array. 
 * If the array is full, it doubles its size, then adds the new socket.
 * 
 * @param poll_fds the poll_fds array that holds the sockets to be monitored
 * @param num_sockets the current number of sockets in the poll_fds array
 * @param max_sockets the maximum number of sockets that can be handled by the poll_fds array
 * @param new_fd the file descriptor of the new socket
 */
void add_socket(struct pollfd **poll_fds, int *num_sockets, int *max_sockets, int new_fd);

/**
 * @brief Removes a socket from the poll_fds array. 
 * If the number of sockets is less or equal to half the maximum number of sockets,
 * it reduces the size of the poll_fds array by half.
 * 
 * @param poll_fds the poll_fds array that holds the sockets to be monitored
 * @param num_sockets the current number of sockets in the poll_fds array
 * @param max_sockets the maximum number of sockets that can be handled by the poll_fds array
 * @param socket_fd the file descriptor of the socket to be removed
 */
void remove_socket(struct pollfd **poll_fds, int *num_sockets, int *max_sockets, int socket_fd);

/**
 * @brief Adds a new topic to a client's list of topics.
 * 
 * @param tcp_clients the array of TCP clients
 * @param nr_tcp_clients the current number of TCP clients
 * @param topic the topic to be added
 * @param id the id of the client
 * @return True if the topic was added successfully, false otherwise
 */
bool add_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, char *id);

/**
 * @brief Removes a topic from a client's list of topics.
 * 
 * @param tcp_clients the array of TCP clients
 * @param nr_tcp_clients the current number of TCP clients
 * @param topic the topic to be removed
 * @param id the id of the client
 * @return True if the topic was removed successfully, false otherwise
 */
bool remove_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, char *id);

/**
 * @brief Adds a new TCP client to the array of TCP clients.
 * If the array is full, it doubles its size, then adds the new client.
 * IF the client is already in the array it, it updates the file descriptor of the client and 
 * sets the is_connected field to true.
 * 
 * @param tcp_clients the array of TCP clients
 * @param nr_tcp_clients the current number of TCP clients
 * @param max_clients the maximum number of TCP clients that can fit in the array
 * @param socket_fd the socket file descriptor of the new client
 * @param id the id of the new client
 */
void add_tcp_client(struct tcp_client **tcp_clients, int *nr_tcp_clients, int *max_clients,
    int socket_fd, char *id);

/**
 * @brief Finds the id of a client using a given socket file descriptor.
 * 
 * @param tcp_clients the array of TCP clients
 * @param nr_tcp_clients the current number of TCP clients
 * @param socket_fd the socket file descriptor of the client
 * 
 * @return The id of the client if it was found, NULL otherwise
 */
char *find_client_id(struct tcp_client *tcp_clients, int nr_tcp_clients, int socket_fd);

/**
 * @brief Marks a client as disconnected in the array of TCP clients.
 * 
 * @param tcp_clients the array of TCP clients
 * @param nr_tcp_clients the current number of TCP clients
 * @param id the id of the client that needs to be disconnected
 */
void disconnect_client(struct tcp_client **tcp_clients, int *nr_tcp_clients, char *id);

/**
 * @brief Checks if a client is connected or not using a given id.
 * 
 * @param tcp_clients the array of TCP clients
 * @param nr_tcp_clients the current number of TCP clients
 * @param id the id of the client
 * 
 * @return True if the client is connected, false otherwise
 */
bool is_connected(struct tcp_client *tcp_clients, int nr_tcp_clients, char *id);

/**
 * @brief Checks if a topic received from the UDP clients matches a pattern from the list of topics
 *  of a TCP client.
 * 
 * @param topic the topic received from the UDP client
 * @param pattern the pattern to be matched (the topic from the list of topics of the TCP client)
 * 
 * @return True if the topic matches the pattern, false otherwise
 */
bool matches(char *topic, char *pattern);

/**
 * @brief Sends a TCP packet to all clients are subscribed to a given topic.
 * It iterates through the list of TCP clients, checks if the current client is subscribed to the 
 * topic and connected, in which case it sends the packet to him.
 * 
 * @param tcp_clients the array of TCP clients
 * @param nr_tcp_clients the current number of TCP clients
 * @param tcp_packet the TCP packet to be sent
 */
void send_packet_to_subscribers(struct tcp_client **tcp_clients, int nr_tcp_clients,
    struct tcp_packet *tcp_packet);

#endif
