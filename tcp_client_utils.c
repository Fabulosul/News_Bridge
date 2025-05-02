#include "tcp_client_utils.h"
 
void print_tcp_packet(struct tcp_packet *tcp_packet) {
	printf("%s:%d - %s", inet_ntoa(*(struct in_addr *)&tcp_packet->ip),
	ntohs(tcp_packet->port), tcp_packet->topic);

	uint8_t sign;
	uint32_t number;
	switch(tcp_packet->data_type) {
		// INT
		case 0: 
			// get the first byte to find out the sign
			sign = *(uint8_t *) tcp_packet->content;

			// get the number from the content
			number = ntohl(*(uint32_t *)(((void *)tcp_packet->content) + 1));

			if(number == 0) {
				// the number is 0
				printf(" - INT - 0\n");
				break;
			}
			
			// the number is positive
			if(sign == 0) {
				printf(" - INT - %u\n", number);
			}
			// the number is negative
			if(sign == 1) {
				printf(" - INT - -%u\n", number);
			}
			break;
		// SHORT_REAL
		case 1:
			uint16_t short_number = ntohs(*(uint16_t *)(tcp_packet->content));
			printf(" - SHORT_REAL - %.2f\n", short_number / 100.0);
			break;
		// FLOAT
		case 2:
			// get the components from the content 
			sign = *(uint8_t *) tcp_packet->content;
			uint32_t number = ntohl(*(uint32_t *)(((void *)tcp_packet->content) + 1));
			uint8_t exponent = *(uint8_t *)(((void *)tcp_packet->content) + 5);

			double original_number = number * pow(-1, sign) / pow(10, exponent);

			if(original_number == (int)original_number) {
				// the number is an integer
				printf(" - FLOAT - %.0f\n", original_number);
			} else {
				// the number is a float
				// put the number in a buffer
				char number_as_string[100];
				memset(number_as_string, 0, sizeof(number_as_string));
				sprintf(number_as_string, "%f", original_number);
				number_as_string[strlen(number_as_string)] = '\0';
				
				// find the first character that is not 0 from end
				int length = strlen(number_as_string) - 1;
				while (number_as_string[length] == '0') {
					length--;
				}

				length++;

				// eliminate the trailing zeros
				number_as_string[length] = '\0';

				// print the number
				printf(" - FLOAT - %s\n", number_as_string);
			}

			break;
		// STRING
		case 3:
			printf(" - STRING - %s\n", tcp_packet->content);
			break;
		default:
			fprintf(stderr, "Invalid data type\n");
			break;
	}
}