struct tcp_packet {
    int total_len;
    int ip;
    short int port;
    char topic[50];
    unsigned int data_type;
    char content[1500];
};