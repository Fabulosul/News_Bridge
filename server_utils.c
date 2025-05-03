#include "server_utils.h"


void add_socket(struct pollfd **poll_fds, int *num_sockets, int *max_sockets, int new_fd) {
	// check if the poll_fds array is full
	if (*num_sockets >= *max_sockets) {
		// double the size of the poll_fds array
		*max_sockets *= 2;
		*poll_fds = realloc(*poll_fds, (*max_sockets) * sizeof(struct pollfd));
		DIE(*poll_fds == NULL, "realloc failed");
	}

	// add the new socket to the poll_fds array
	(*poll_fds)[*num_sockets].fd = new_fd;
	(*poll_fds)[*num_sockets].events = POLLIN;
	(*num_sockets)++;
}

void remove_socket(struct pollfd **poll_fds, int *num_sockets, int *max_sockets, int socket_fd) {
	// find the socket in the poll_fds array
	for (int i = 0; i < *num_sockets; i++) {
		if ((*poll_fds)[i].fd == socket_fd) {
			// close the socket
			int rc = close(socket_fd);
			DIE(rc < 0, "close failed");
			// remove the socket from the poll_fds array and replace it with the last socket
			(*poll_fds)[i] = (*poll_fds)[*num_sockets - 1];
			// set the last socket to -1 stating that it is not used
			(*poll_fds)[*num_sockets - 1].fd = -1;
			(*poll_fds)[*num_sockets - 1].events = 0;
			(*poll_fds)[*num_sockets - 1].revents = 0;
			// decrease the number of sockets
			(*num_sockets)--;
			break;
		}
	}

	// check if the poll_fds array is less or equal to half the maximum number of sockets
	if (*num_sockets <= *max_sockets / 2) {
		// reduce the size of the poll_fds array by half
		*max_sockets /= 2;
		*poll_fds = realloc(*poll_fds, (*max_sockets) * sizeof(struct pollfd));
		DIE(*poll_fds == NULL, "realloc failed");
	}
}

bool add_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, char *id) {
	// find the client in the tcp_clients array
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the current client mathes the given id
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			// add null terminator
			topic[TOPIC_SIZE] = '\0';
			// add the topic to the client's list of topics
			insert_node((*tcp_clients)[i].topics, topic, TOPIC_SIZE);
			return true;
		}
	}
	
	return false;
}

bool remove_topic(struct tcp_client **tcp_clients, int nr_tcp_clients, char *topic, char *id) {
	// find the client in the tcp_clients array
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the current client mathes the given id
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			// add null terminator
			topic[TOPIC_SIZE] = '\0';
			// remove the topic from the client's list of topics
			delete_node((*tcp_clients)[i].topics, topic, TOPIC_SIZE);
			return true;
		}
	}
	
	return false;
}

void add_tcp_client(struct tcp_client **tcp_clients, int *nr_tcp_clients, int *max_clients,
	int socket_fd, char *id) {
	// check if the client is in array
	for (int i = 0; i < *nr_tcp_clients; i++) {
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			// update the socket file descriptor and set the client as connected
			(*tcp_clients)[i].socket_fd = socket_fd;
			(*tcp_clients)[i].is_connected = true;
			return;
		}
	}

	// check if we need to increase the size of the tcp_clients array
	if (*nr_tcp_clients >= *max_clients) {
		// double the size of the tcp_clients array
		*max_clients *= 2;
		*tcp_clients = realloc(*tcp_clients, (*max_clients) * sizeof(struct tcp_client));
		DIE(*tcp_clients == NULL, "realloc failed");
	}

	// add the new client to the tcp_clients array
	(*tcp_clients)[*nr_tcp_clients].socket_fd = socket_fd;
	void *rc = memcpy((*tcp_clients)[*nr_tcp_clients].id, id, strlen(id) + 1);
	DIE(rc == NULL, "memcpy failed");
	(*tcp_clients)[*nr_tcp_clients].is_connected = true;
	// create a new list to hold the topics for the client
	(*tcp_clients)[*nr_tcp_clients].topics = create_list();
	DIE((*tcp_clients)[*nr_tcp_clients].topics == NULL, "create_list failed");
	// increment the number of TCP clients
	(*nr_tcp_clients)++;
}

char *find_client_id(struct tcp_client *tcp_clients, int nr_tcp_clients, int socket_fd) {
	// search the client in the tcp_clients array by the socket file descriptor
	for (int i = 0; i < nr_tcp_clients; i++) {
		if (tcp_clients[i].socket_fd == socket_fd) {
			return tcp_clients[i].id;
		}
	}
	
	return NULL;
}

void disconnect_client(struct tcp_client **tcp_clients, int *nr_tcp_clients, char *id) {
	// find the client in the tcp_clients array
	for (int i = 0; i < *nr_tcp_clients; i++) {
		if (strcmp((*tcp_clients)[i].id, id) == 0) {
			// set the client as disconnected
			(*tcp_clients)[i].is_connected = false;
			break;
		}
	}
}

bool is_connected(struct tcp_client *tcp_clients, int nr_tcp_clients, char *id) {
	for (int i = 0; i < nr_tcp_clients; i++) {
		if (strcmp(tcp_clients[i].id, id) == 0) {
			return tcp_clients[i].is_connected;
		}
	}
	
	return false;
}

bool matches(char *topic, char *pattern) {
	char topic_copy[TOPIC_SIZE];
	char pattern_copy[TOPIC_SIZE];

	// make a copy of the topic and pattern strings
	void *rc = memcpy(topic_copy, topic, TOPIC_SIZE);
	DIE(rc == NULL, "memcpy failed");
	rc = memcpy(pattern_copy, pattern, TOPIC_SIZE);
	DIE(rc == NULL, "memcpy failed");

	// declare pointers to hold the current positions in the two strings
	char *topic_ptr_copy;
	char *pattern_ptr_copy;

	// get the first word from the topic and pattern strings
	char *topic_word = strtok_r(topic_copy, "/", &topic_ptr_copy);
	char *pattern_word = strtok_r(pattern_copy, "/", &pattern_ptr_copy);

	while (topic_word != NULL && pattern_word != NULL) {
		// the current word in pattern is equal to "*"
		if (strcmp(pattern_word, "*") == 0) {
			// make a copy of the string after the "*" in the pattern
			char *remaining_pattern = strdup(pattern_ptr_copy);
			// get the next word from pattern
			char *next_pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
			// the pattern ends in "*" which means it can match the rest of the topic
			if(next_pattern_word == NULL) {
				// free tha allocated memory for the copy of the pattern
				free(remaining_pattern);
				return true;
			}
			// try to skip 0, 1, 2, 3, ... words from the topic due to "*" presence
			while (topic_word != NULL) {
				/* if by skipping a certain number of words of topic we can match the remaining 
				 * pattern, we can return true */
				if(matches(topic_ptr_copy, remaining_pattern)) {
					// free the allocated memory for the copy of the pattern
					free(remaining_pattern);
					return true;
				}
				// skip one more word from the topic
				topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
			}
			// if all the possibilities failed, we can return false
			free(remaining_pattern);
			return false;
		}

		// the current word in pattern is equal to "+"
		if (strcmp(pattern_word, "+") == 0) {
			// skip one word from both topic and pattern
			topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
			pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
			continue;
		}

		// pattern has no wildcards in the current word
		// check if the current word from topic is equal to the current word from pattern
		if (strcmp(topic_word, pattern_word) != 0) {
			return false;
		}

		// get the next word from both topic and pattern
		topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
		pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
	}

	// if both the topic and the pattern are NULL, the topic and pattern match
	return topic_word == NULL && pattern_word == NULL;
}

void send_packet_to_subscribers(struct tcp_client **tcp_clients, int nr_tcp_clients,
		struct tcp_packet *tcp_packet) {
	char *topic = tcp_packet->topic;
	// iterate through the list of TCP clients
	for (int i = 0; i < nr_tcp_clients; i++) {
		// check if the client is subscribed to the topic
		struct tcp_client *client = &(*tcp_clients)[i];
		// if the client is not subscribed to any topic or is not connected, skip it
		if(client->topics == NULL || is_empty(client->topics) || 
			!is_connected(*tcp_clients, nr_tcp_clients, client->id)) {
			continue;
		}

		struct node *current_node = client->topics->head;
		do {
			// check if the given topic matches the client's current topic
			char *client_topic = (char *)current_node->data;
			if (matches(topic, client_topic)) {
				// send the message to the client
				int rc = send_tcp_packet(client->socket_fd, tcp_packet);
				DIE(rc < 0, "send failed");
				break;
			}
			current_node = current_node->next;
		} while (current_node != client->topics->head);
	}
}
