CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 5000

# Adresa IP a serverului
IP_SERVER = 192.168.0.2

all: server subscriber

common.o: common.c

udp.o: udp.c udp.h

tcp.o: tcp.c tcp.h

server: server.c common.o udp.o tcp.o
	$(CC) $(CFLAGS) -o $@ $^

subscriber: tcp_client.c common.o tcp.o
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean run_server run_client

run_server:
	./server ${IP_SERVER} ${PORT}

run_client:
	./client ${IP_SERVER} ${PORT}

clean:
	rm -rf server subscriber *.o *.dSYM
