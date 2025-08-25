**Tudor Robert Fabian**
**322CAa**

# News Bridge - TCP and UDP client-server application for message management

## The TCP Protocol
- The TCP protocol is used to communicate between the server and the TCP clients.
- It uses the folowinf structure:
```
struct tcp_packet {
    int content_len; // the number of bytes in the content
    int topic_len; // the number of bytes in the topic
    int ip; // IPv4 adress of a client (UDP / TCP) 
    short int port; // port of a client (UDP / TCP)
    char topic[51]; // the topic of the message
    unsigned char data_type; // the type of data in the content
    char content[1501]; // the actual message 
};
```
- This protocol is used in two ways: <br>
    1. To send data from a UDP client to TCP one through the server.
        - The server receives an UDP packet from a client and copies the data from the it 
        to a new TCP packet, then sends it to the TCP client.
    2. To send data from a TCP client to the server.
        - There are 4 possible types of interactions with the server:
            1. **Client ID**
                - After a client establishes a connection with the server, it sends a pachet with 
                the message ***Client id*** present in the content field, the actual id in the topic
                and the ip and port of the client in the right fields.
                - This interaction is used to create a new tcp_client entry in the server if the 
                client never connected before, to update the socket if the client reconnected and to
                 check if a client with the same id is already connected.
            2. **Subscribe**
                - The client sends a packet to the server with the message ***Subscribe*** in the 
                content field, the topic received from the input in the right field
                so that it puts the topic in the client's list of topics.
            3. **Unsubscribe**
                - The client sends a packet to the server with the message ***Unsubscribe*** in the
                content field, the topic received from the input in the right field so that it 
                removes the topic from the client's list of topics.
            4. **Exit**
                - This is the case when the client receives an ***Exit*** message from the input,
                so it closes the sockets, deallocates the memory and exits the program.
                - It is no need to send a specific packet to the server because by default it is 
                send an empty packet and the server handles it.
- The protocol uses the ***content_len*** and ***topic_len*** fields to determine the length of the
content and topic fields. Keeping in mind that this type of protocol can concatanate or truncate
packets, by using the two previously mentioned fields, we not only make sure that the server always
reads the each packet correctly and the client receives the data entirely, but also that 
unnecessary bytes are not sent over the network.
- Sending a TCP packet works like this:
    - First, we calculate the length of the content and topic fields.
    - Then, we send the ***topic_len***, ***content_len***, ***ip*** and ***port*** fields to the 
    server since they have a fixed size.
    - After that, we send the ***topic*** in a while loop, so that we make sure exactly 
    ***topic_len*** bytes are sent.
    - Finally, we send the ***data_type*** and ***content*** fields in the same way, 
   by making sure we send exactly ***sizeof(data_type) + content_len*** bytes.
- Receiving a TCP packet has a similar logic:
    - First, we read the ***topic_len***, ***content_len***, ***ip*** and ***port*** fields from the 
    server, so that we know when to stop reading the topic and content fields.
    - Then, we read the ***topic*** in a while loop, so that we make sure exactly 
    ***topic_len*** bytes are received.
    - After that, we read the ***data_type*** and ***content*** fields since now we know how many
    bytes we need to receive.
- It is also important to mention that the ***content_len***, ***topic_len***, ***ip*** and
***port*** fields are kept in ***network byte order*** in the TCP packets, so we need to convert
them to ***host byte order*** before using them in server or TCP clients.

<br>

## The UDP Protocol
- This type of protocol is used to send topics from the UDP clients to the server.
- In order to receive the UDP packets in the right format, I used the following structure:
```
struct udp_packet {
    char topic[50];
    unsigned char data_type;
    char content[1500];
};
```
- The UDP protocol doesn't care about reliability, so when a UDP packet is received, the server
will read it directly, without checking if the packet is complete or not:
```
    int bytes_received = recvfrom(udp_socket_fd, packet, sizeof(struct udp_packet), 0,
                      (struct sockaddr *)udp_client_addr, &udp_client_len);
    DIE(bytes_received < 0, "recvfrom failed");
```
- Also, to explain why the TCP protocol uses a struct with 51 bytes for the topic and 1501 
bytes for the content, while the UDP protocol uses 50 bytes for the topic and 1500 bytes for the
content, I can say that the UDP protocol might fill up both of those fields without checking if
the strings hava a null terminator or not. In order to avoid a problem when I try to read from those
fields, I added 1 extra byte where I put a null terminator, so in case the UDP packet does not
put it, I can still read the data correctly.
- This protocol is used in only one direction which means that the server will only receive
the packets, do some logic and then send them to the right TCP clients. 

<br>

## Application Workflow

### Server Logic
- The server is the middleman between the UDP clients and the TCP clients.
- He is the first component that needs to be started in order to be able to use the application.
- To start the server, you need to run the following commands:
```
1. make server // to compile the server
2. ./server <port> // to run the server on a specific port
```
- Then, we create a resizeable array of ***poll_fds***, which will be used to store the sockets
that we want to monitor. For the beginning, we will add the STDIN, the UDP socket and the TCP 
listening socket to the array, but each time a new TCP client connects, we will accept him using 
the listening socket, create him a new socket and add it to the array.
- It is also important to mention that each time we receive a new connection from a TCP client,
we firstly check if the client already exists in the list of clients: 
    - If it does, but he is disconnected, we will update the socket and set the status to
    ***connected***.
    - If it does and he is already connected, we close its socket and by doing so, he will receive
    and empty packet and will automatically close the connection.
    - If it does not, we will create a new entry in the list of clients and set the status to
    ***connected*** and add the socket to the array of ***poll_fds***.
- The server was also designed to handle any number of TCP clients, so when the array of
TCP clients is full, we will double its size and reallocate the memory. The same thing happens with 
the ***tcp_clients*** array, which is used to store the TCP clients, it doubles each time we 
ran out of space. 
- Not only that, but the server uses the memory efficiently because each time
the ***poll_fds*** array has the number of active sockets lower or equal to half of its size,
it reallocates the memory and reduces its size to half. 
- Also, the ***tcp_clients*** array is keeps specially designed arrays to use as less 
memory as possible: 
```
struct tcp_client {
    int socket_fd; // the socket file descriptor of the client
    char id[11]; // the id of the client
    bool is_connected; // true if the client is connected, false otherwise
    struct doubly_linked_list *topics; // list of topics to which the client is subscribed
};
```
- The ***topics*** field is a doubly linked list that stores the topics to which the client is
subscribed. This choice was made so that the addition of new topics is done in O(1) time and the
removal of topics is done in O(n) time worst case. This helps us to keep the memory usage
to a minimum while having a good performance.
- The server uses the ***poll()*** function to monitor the sockets and to check if there are any
any new events on the sockets. When receiving a new event, there are 4 possible cases:
    1. The server receives input from **STDIN**
        - It reads the input and checks if the message is ***exit***. If it is, it closes the 
        all the sockets, frees the memory and exits the program. If it is not, it prints a message
        stating that the command is not valid.
    2. The server receives a packet on the **UDP socket**
        - It reads the packet, iterates through the topics of each TCP client until it finds a
        matching topic with the one received in the UDP packet and if it does, it creates a new TCP 
        packet with the data from the UDP packet and sends it to the TCP client.
        - For a particular topic, if it founds a matching topic, it does not continue the search 
        in the remaining topics of the client, it just sends the packet to the client, and starts 
        the search in the topics of the next client.
    3. The server receives a new connection on the **TCP listening socket**
        - It acceps the new connection, creates a new socket and adds it to the array of
        ***poll_fds*** if the client is not already connected or has just reconnected.
        - It also creates a new entry in the ***tcp_clients*** array and sets the status to
        ***connected*** if the client has an id that has never connected before or updates the socket
        and sets the status to ***connected*** if the client has reconnected.
        - It also waits for a TCP packet from the client with the id of the client in the topic
        field and the message ***Client id*** in the content field. This is used to check
        if the clients has connected before, if he has reconnected or if he is a new client because 
        the verification is handled using the id of the client.
    4. The server receives a packet on a **TCP socket**
        - It can receive 2 types of packets:
            1. ***subscribe***
                - The server adds the topic received in the packet to the client's list of topics.
            2. ***unsubscribe***
                - The server removes the topic received in the packet from the client's list of 
                topics.
        - If other types of packets are received, the server will ignore them and will print a
        message stating that the packet is not valid.
        - When a TCP client closes the connection an empty packet is sent to the server and this 
        is the moment when the server sets a client as ***disconnected*** and removes the 
        socket from the ***poll_fds*** array.

<br>

### TCP Client Logic
- This type of client can be started by running the following commands:
```
1. make subscriber // to compile the TCP client
2. ./subscriber <client_id> <server_ip> <server_port> // to connect the client to the server
```
- Initially, it creates a new socket to communicate with the server and sends a packet with the
***client id*** to the server. Then it creates a new ***poll_fds*** array to monitor the 
***STDIN*** and server sockets.
- There are two secenarios when the client can receive a new event:
    1. The client receives input from **STDIN**
        - It reads the input and checks if the message is one of the following:
            1. ***subscribe***
                - The client sends a packet to the server with the message ***Subscribe*** in the 
                content field and the topic received from the input in the right field.
            2. ***unsubscribe***
                - The client sends a packet to the server with the message ***Unsubscribe*** in the 
                content field and the topic received from the input in the right field.
            3. ***exit***
                - The client closes the socket, deallocates the memory and exits the program.
                - This automatically sends an empty packet to the server stating that the client
                closed connection.
        - If any of these commands are not present in the input, the client will print a message
        stating that the command is not valid.

    2. The client receives a packet from the **server**
        - The client reads the packet and checks:
            1. If the packet is empty, it means that the server closed the connection, so it will
            close the socket, deallocate the memory and exit the program.
            2. If the packet is not empty, it will print the message received from the server in the
            following format:
                ```
                <udp_client_ip>:<udp_client_port> - <topic> - <data_type> - <content>
                ```
                - The content is printed after the logic presented in the text of the homework
                is applied:
                    - If the data_type is 0, the content is an INT.
                    - If the data_type is 1, the content is a SHORT_REAL.
                    - If the data_type is 2, the content is a FLOAT.
                    - If the data_type is 3, the content is a STRING.
- It is also important to mention that reading a TCP packet is done in a while loop, so that we
can make sure the packet is read entirely.
- The ***client_id*** is used to identify the client and must be a string of maximum 10 characters.
If this size is exceeded, the client will print a message stating that the id is too long.
- The same thing happens if an invalid command is received from the input, the client will highlight
this thing.

<br>

## Displayed Messages
- The TCP client will print the following messages:
    - ***Subscribed to topic X*** -> when a client subscribes to a topic
    - ***Unsubscribed from topic X*** -> when a client unsubscribes from a topic
    - ***udp_client_ip:udp_client_port - topic - data_type - content*** -> 
    when a UDP client send a topic to which the TCP client is subscribed

- The server will print the following messages:
    - ***New client <client_id> connected from ip:port.*** -> when a new TCP client connects/
    reconnects to the server
    - ***Client <client_id> disconnected.*** -> when a TCP client disconnects from the server
    - ***Client <client_id> already connected.*** -> when a TCP client tries to connect to the server
    while a client with the same id is already connected

- Besides these messages, there are also some error message that are printed to ***STDERR*** in case
of edge cases so that the user can easily spot the problem: ***Given client id is invalid***, 
***Id is too long***, ***Invalid command*** and so on.

<br>

## General Observations
- The application is written in C.
- The code is full of comments to better understand the implementation and the logic behind it.
- I stayed consistent in terms of naming conventions and code style, so the code is easy to read and
understand. The variables and functions are named in English and the comments and README are in
the same language as well.
- As a starting point for my application, I drew inspiration from the 6th and 7th labs of the 
Communication Protocols course, but the implementation was developed entirely from scratch.
- The doubly linked circular list data structure used for managing the topics was originally 
implemented for a Data Structures and Algorithms assignment and has been modified and 
reused here in a different context.
- The code was written to use the memory as efficiently as possible, while also having a good
performance and being easy to understand.
- The code uses defensive programming, so it checks for all possible errors and edge cases, so that 
when one of them occurs, if the program can continue, it will do so while printing an error message,
but if it cannot, it will close the sockets, free the memory and exit the program while showing
the user an error message.
