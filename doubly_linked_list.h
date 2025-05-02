#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "helpers.h"

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

/**
 * @brief Checks if a list is empty or not.
 * 
 * @param list the list to check
 * 
 * @return True if the list is empty, false otherwise.
 */
bool is_empty(struct doubly_linked_list *list);

/**
 * @brief Creates a new doubly linked list.
 * 
 * @return A pointer to the new list or NULL if the list could not be created.
 */
struct doubly_linked_list *create_list();

/**
 * @brief Inserts a new node at the end of a given list.
 * 
 * @param list the list to insert the node into
 * @param data the data to that needs to be written in the new node
 * @param data_size the size of the data
 *  
 * @return True if the node was inserted successfully, false otherwise.
 */
bool insert_node(struct doubly_linked_list *list, void *data, int data_size);

/**
 * @brief Deletes a node from the list.
 * 
 * @param list the list from which the node needs to be deleted
 * @param data the data present in the node that needs to be deleted
 * @param data_size the size of the data
 * 
 * @return True if the node was deleted successfully, false otherwise.
 */
bool delete_node(struct doubly_linked_list *list, void *data, int data_size);

/**
 * @brief Deletes the entire list and frees the memory associated with it.
 * 
 * @param list the list to be deleted
 */
void delete_list(struct doubly_linked_list *list);

#endif
