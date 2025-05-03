#include "doubly_linked_list.h"


bool is_empty(struct doubly_linked_list *list) {
    return list->head == NULL;
}

struct doubly_linked_list *create_list() {
    struct doubly_linked_list *list = malloc(sizeof(struct doubly_linked_list));
    DIE(list == NULL, "malloc failed");
    
    list->head = NULL;
    list->size = 0;
    
    return list;
}

bool insert_node(struct doubly_linked_list *list, void *data, int data_size) {
    if(list == NULL) {
        return false;
    }

    struct node *new_node = malloc(sizeof(struct node));
    DIE(new_node == NULL, "malloc failed");
    new_node->data = malloc(data_size);
    DIE(new_node->data == NULL, "malloc failed");
    void *rc = memcpy(new_node->data, data, data_size);
    DIE(rc == NULL, "memcpy failed");
    new_node->data_size = data_size;

    if(is_empty(list)) {
        list->head = new_node;
        new_node->next = new_node;
        new_node->prev = new_node;
    } else {
        struct node *last_node = list->head->prev;

        list->head->prev = new_node;
        last_node->next = new_node;
        new_node->next = list->head;
        new_node->prev = last_node;
    }

    list->size++;
    
    return true;
}

bool delete_node(struct doubly_linked_list *list, void *data, int data_size) {
    if(list == NULL || is_empty(list)) {
        return false;
    }

    if(list->size == 1) {
        if(memcmp(list->head->data, data, data_size) == 0) {
            free(list->head->data);
            free(list->head);
            list->head = NULL;
            list->size--;
            return true;
        } else {
            return false;
        }
    }

    struct node *current_node = list->head;
    do {
        if(memcmp(current_node->data, data, data_size) == 0) {
            current_node->prev->next = current_node->next;
            current_node->next->prev = current_node->prev;

            if(current_node == list->head) {
                list->head = current_node->next;
            }

            free(current_node->data);
            free(current_node);

            list->size--;

            return true;
        }

        current_node = current_node->next;
    } while(current_node != list->head);
    
    return false;
}

void delete_list(struct doubly_linked_list *list) {
    if (list == NULL) {
        return;
    }

    if (is_empty(list)) {
        free(list);
        return;
    }

    struct node *current_node = list->head;
    struct node *next_node = NULL;

    do {
        next_node = current_node->next;

        free(current_node->data);
        free(current_node);

        current_node = next_node;
    } while(current_node != list->head);

    free(list);
}
