CC = gcc
CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

server: server.c udp.o tcp.o doubly_linked_list.o server_utils.o
	$(CC) $(CFLAGS) -o $@ $^

subscriber: tcp_client.c tcp.o doubly_linked_list.o tcp_client_utils.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

server_utils.o: server_utils.c server_utils.h
	$(CC) $(CFLAGS) -c server_utils.c

tcp_client_utils.o: tcp_client_utils.c tcp_client_utils.h
	$(CC) $(CFLAGS) -c tcp_client_utils.c

udp.o: udp.c udp.h
	$(CC) $(CFLAGS) -c udp.c

tcp.o: tcp.c tcp.h
	$(CC) $(CFLAGS) -c tcp.c

doubly_linked_list.o: doubly_linked_list.c doubly_linked_list.h
	$(CC) $(CFLAGS) -c doubly_linked_list.c

.PHONY: all clean

clean:
	rm -rf server subscriber *.o
