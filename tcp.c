#include "tcp.h"


int recv_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet) {
    int topic_len, content_len;
    int bytes_read = 0;
    int total_bytes_read = 0;
    int bytes_remaining = sizeof(packet->topic_len) + sizeof(packet->content_len) +
        sizeof(packet->ip) + sizeof(packet->port);

    // read the length of the topic, the length of the content, the ip address and the port
    while (bytes_remaining > 0) {
        bytes_read = recv(tcp_socket_fd, (char *)packet + total_bytes_read, 
                              bytes_remaining, 0);
        DIE(bytes_read < 0, "recv failed");
        
        // check if we received an empty packet stating that the connection was closed
        if (bytes_read == 0) {
            return 0;
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
        DIE(bytes_read < 0, "recv failed");

        bytes_remaining -= bytes_read;
        total_bytes_read += bytes_read;
    }

    bytes_remaining = sizeof(packet->data_type) + content_len;
    total_bytes_read = 0;

    // read the data type and the content
    while (bytes_remaining > 0) {
        bytes_read = recv(tcp_socket_fd, (char *)&packet->data_type + total_bytes_read,
                            bytes_remaining, 0);
        DIE(bytes_read < 0, "recv failed");

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
        DIE(bytes_sent < 0, "send failed");

        bytes_remaining -= bytes_sent;
        total_bytes_sent += bytes_sent;
    }
    
    bytes_remaining = topic_len;

    // send the topic
    while (bytes_remaining > 0) {
        bytes_sent = send(tcp_socket_fd, (char *)packet + total_bytes_sent, 
                            bytes_remaining, 0);
        DIE(bytes_sent < 0, "send failed");

        bytes_remaining -= bytes_sent;
        total_bytes_sent += bytes_sent;
    }

    bytes_remaining = sizeof(packet->data_type) + content_len;
    total_bytes_sent = 0;
    
    // send the data type and the content
    while (bytes_remaining > 0) {
        bytes_sent = send(tcp_socket_fd, (char *)&packet->data_type + total_bytes_sent,
                            bytes_remaining, 0);
        DIE(bytes_sent < 0, "send failed");

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
    void *ret = memset(&packet, 0, sizeof(packet));
    
    // Copy the parameters into the packet struct
    packet.ip = ip;
    packet.port = port;
    packet.data_type = data_type;
    
    ret = memcpy(packet.topic, topic, sizeof(packet.topic));
    DIE(ret == NULL, "memcpy failed");
    packet.topic[sizeof(packet.topic) - 1] = '\0';
    packet.topic_len = htonl(strlen(packet.topic));
    
    ret = memcpy(packet.content, content, sizeof(packet.content));
    DIE(ret == NULL, "memcpy failed");
    packet.content[sizeof(packet.content) - 1] = '\0';

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
            packet.content_len = htonl(strlen(content) + 1);
            break;
        default:
            fprintf(stderr, "Invalid data type\n");
            exit(EXIT_FAILURE);
    }

    return packet;
}
