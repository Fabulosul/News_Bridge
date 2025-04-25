struct tcp_packet {
    int total_len;
    int ip;
    short int port;
    char topic[50];
    unsigned int data_type;
    char content[1500];
};

struct tcp_client {
    int socket_fd;
    char id[11];
};

int recv_tcp_packet(int tcp_socket_fd, struct tcp_packet *packet);
int send_tcp_packet(int tcp_client_fd, struct tcp_packet *packet);
struct tcp_packet create_tcp_packet(int ip, short int port, char *topic, 
    unsigned int data_type, char *content);