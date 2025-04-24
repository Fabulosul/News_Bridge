#include <stdio.h>
#include <stdlib.h>

#include "tcp.h"

int send_tcp_packet(int tcp_client_fd, struct tcp_packet *packet) {
    int packet_len = packet->total_len;
    int bytes_sent = 0;
    int bytes_remaining = packet_len;

    while (bytes_remaining > 0) {
        bytes_sent = send(tcp_client_fd, (char *)packet + packet_len - bytes_remaining, bytes_remaining, 0);
        if (bytes_sent < 0) {
            fprintf(stderr, "send failed");
            return -1;
        }
        bytes_remaining -= bytes_sent;
    }

    return packet_len;
}