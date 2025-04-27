struct doubly_linked_list {
    struct node *head;
    int size;
};

struct node {
    struct node *next;
    struct node *prev;
    int data_size;
    void *data;
};


bool is_empty(struct doubly_linked_list *list);
struct doubly_linked_list *create_list();
bool insert_node(struct doubly_linked_list *list, void *data, int data_size);
bool delete_node(struct doubly_linked_list *list, void *data, int data_size);
void delete_list(struct doubly_linked_list *list);