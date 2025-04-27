#define MAX_TOPIC_SIZE 100

struct tcp_client {
    int socket_fd; // the socket file descriptor of the client
    int id; // the id of the client
    struct doubly_linked_list *topics; // list of topics to which the client is subscribed
};