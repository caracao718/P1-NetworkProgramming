#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define LISTEN_PORT 8787
#define BUFFER_SIZE 1024
#define IP_SIZE 16 // my VM IP address is 10.0.0.214

struct Config {
    char server_ip_address[IP_SIZE]; 
    char client_ip_address[IP_SIZE];
    int source_port_UDP;
    int dest_port_UDP;
    int dest_port_TCP_Head;
    int dest_port_TCP_Tail;
    int port_TCP;
    int size_UDP_payload;
    int inter_measurement_time_seconds;
    int num_UDP_packets;
    int TTL_UDP;
};

int main() {
    int server_socket, client_socket, addr_len; 
    struct sockaddr_in server_address, client_address;

    // PHASE 1: pre-probing TCP connection

    // 1. create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");
    
    memset(&server_address, 0, sizeof(server_address));

    // 2. Define the Server Address and Port Number
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; 
    server_address.sin_port = htons(LISTEN_PORT);

    // 3. Bind the Server Socket to the Address and Port Number. Bind newly created socket to given IP and verification
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed\n");
        abort();
    } else {
        printf("Bind successful\n");
    }

    // 4. Listen for Connections
    if (listen(server_socket, 1) < 0) {
        perror("Listen failed\n");
        abort();
    } else {
        printf("Listening...\n");
    }
    client_socket, addr_len = sizeof(client_address);

    // 5. Accept a Connection
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addr_len);
    if (client_socket < 0) {
        perror("server accept failed...\n");
        abort();
    }
    else
        printf("server accepted the client...\n");

    // 6. Receive Data
    struct Config config;
    // read the message from client and copy it in buffer
    int byte_received = recv(client_socket, &config, sizeof(struct Config), 0);
    if (byte_received < 0) {
        perror("Receive failed\n");
        abort();
    } else if (byte_received == 0) {
        printf("Client disconnected\n");
    } else {
        printf("Received %d bytes\n", byte_received);
    }
    // print buffer which contains the client contents
    printf("Server address from client: %s\n", config.server_ip_address);

    // 7. Close the Connection
    printf("Now closing TCP pre-probing sockets\n");
    if (close(server_socket) < 0) {
        perror("Close failed\n");
        return -1;
    } else {
        printf("Close successful\n");
    }
    if (close(client_socket) < 0) {
        perror("Close failed\n");
        return -1;
    } else {
        printf("Close successful\n");
    }


    // PHASE 2: UDP probing

    // 1. create socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");

    // 2. Define the Server Address and Port Number
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(config.dest_port_UDP);

    // Reuse Port
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("couldn't reuse address");
        abort();
    }

    // 3. Bind the Server Socket to the Address and Port Number. Bind newly created socket to given IP and verification
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed\n");
        abort();
    } else {
        printf("Bind successful\n");
    }

    // 4. Receive Data
    char buffer[config.size_UDP_payload];
    memset(buffer, 0, BUFFER_SIZE);
    addr_len = sizeof(client_address);
    byte_received = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &addr_len);
    if (byte_received < 0) {
        perror("Receive failed\n");
        abort();
    } else if (byte_received == 0) {
        printf("Client disconnected\n");
    } else {
        printf("Received %d bytes\n", byte_received);
    }

    // print buffer which contains the client contents
    printf("From client: %s\n", buffer);

    // 5. Close the Connection
    printf("Now closing UDP probing sockets\n");
    if (close(server_socket) < 0) {
        perror("Close failed\n");
        return -1;
    } else {
        printf("Close successful\n");
    }


    return 0;
}