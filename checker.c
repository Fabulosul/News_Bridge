#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define TOPIC_SIZE 51

bool matches(char *topic, char *pattern) {
	char topic_copy[TOPIC_SIZE];
	char pattern_copy[TOPIC_SIZE];

	// make a copy of the topic and pattern strings
	memcpy(topic_copy, topic, TOPIC_SIZE);
	memcpy(pattern_copy, pattern, TOPIC_SIZE);
	
	// declare pointers to hold the current positions in the strings
	char *topic_ptr_copy;
	char *pattern_ptr_copy;

	// get the first word from the topic and pattern strings
	char *topic_word = strtok_r(topic_copy, "/", &topic_ptr_copy);
	char *pattern_word = strtok_r(pattern_copy, "/", &pattern_ptr_copy);

	while (topic_word != NULL && pattern_word != NULL) {
		// pattern has the current word equal to "*"
		if (strcmp(pattern_word, "*") == 0) {
			char *remaining_pattern = strdup(pattern_ptr_copy);
			// get the next word from pattern
			char *next_pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
			// the pattern ends in "*" which means it can match the rest of the topic
			if(next_pattern_word == NULL) {
				return true;
			}
			
			while (topic_word != NULL) {
				if(matches(topic_ptr_copy, remaining_pattern)) {
					return true;
				}
				topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
			}
			return false;
		}
		// pattern has the current word equal to "+"
		if (strcmp(pattern_word, "+") == 0) {
			// skip one word from both topic and pattern
			topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
			pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
			continue;
		}
		// pattern has no wildcard in the current word
		// check if the current word from topic is equal to the current word from pattern
		if (strcmp(topic_word, pattern_word) != 0) {
			return false;
		}
		// get the next word from both topic and pattern
		topic_word = strtok_r(NULL, "/", &topic_ptr_copy);
		pattern_word = strtok_r(NULL, "/", &pattern_ptr_copy);
	}

	// if the pattern ended in "*" and the topic is not finished, it is a match anyway
	if(pattern_word != NULL && strcmp(pattern_word, "*") == 0 && topic_word == NULL) {
		return true;
	}

	// if both topic and pattern are NULL, they are a match
	return topic_word == NULL && pattern_word == NULL;
}

int main() {
    char topic[TOPIC_SIZE];
    char pattern[TOPIC_SIZE];

    printf("Enter topic: ");
    fgets(topic, TOPIC_SIZE, stdin);
    topic[strcspn(topic, "\n")] = '\0'; // remove newline

    printf("Enter pattern: ");
    fgets(pattern, TOPIC_SIZE, stdin);
    pattern[strcspn(pattern, "\n")] = '\0'; // remove newline

    if (matches(topic, pattern)) {
        printf("Match\n");
    } else {
        printf("No match\n");
    }

    return 0;
}
