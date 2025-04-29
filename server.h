#define MAX_TOPIC_SIZE 100

struct tcp_client {
    int socket_fd; // the socket file descriptor of the client
    char id[11]; // the id of the client
    bool is_connected; // true if the client is connected, false otherwise
    struct doubly_linked_list *topics; // list of topics to which the client is subscribed
};