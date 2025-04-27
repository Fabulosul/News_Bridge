struct tcp_packet {
    int content_len;
    int topic_len;
    int ip;
    short int port;
    char topic[50];
    unsigned char data_type;
    char content[1500];
};

int recv_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet);
int send_tcp_packet(int tcp_client_fd, struct tcp_packet *packet);
struct tcp_packet create_tcp_packet(int ip, short int port, char *topic, 
    unsigned char data_type, char *content);