struct udp_packet {
    char topic[50];
    unsigned char data_type;
    char content[1500];
};

int recv_udp_packet(int udp_socket_fd, struct udp_packet *packet, 
                    struct sockaddr_in *udp_client_addr);
