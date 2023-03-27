#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define IP_SIZE 10 // my VM IP address is 10.0.0.130


struct Config {
    char server_ip_address[IP_SIZE]; 
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

struct Config parseJson(char *json_string) {
    struct Config config;
    char server_ip_address[IP_SIZE];
    int source_port_UDP;
    int dest_port_UDP;
    int dest_port_TCP_Head;
    int dest_port_TCP_Tail;
    int port_TCP;
    int size_UDP_payload;
    int inter_measurement_time_seconds;
    int num_UDP_packets;
    int TTL_UDP;

    FILE* config_file = fopen("../config.json", "r");
    if (!config_file) {
        printf("Error opening config file\n");
        exit(1);
    }
    char temp[100];
    fscanf(config_file, "%s", temp);
    
    fscanf(config_file, "%s", temp);
    int num_values = fscanf(config_file, "%s", server_ip_address);
    for (int i = 0; i < strlen(server_ip_address) - 2; i++) {
        server_ip_address[i] = server_ip_address[i + 1];
    }
    printf("Server IP: %s\n", server_ip_address);
    strncpy(config.server_ip_address, server_ip_address, sizeof(server_ip_address));
    config.server_ip_address[sizeof(config.server_ip_address)] = '\0';
    printf("Stored Server IP: %s\n", config.server_ip_address);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &source_port_UDP);
    config.source_port_UDP = source_port_UDP;
    printf("Stored Source Port: %d\n", config.source_port_UDP);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &dest_port_UDP);
    config.dest_port_UDP = dest_port_UDP;
    printf("Stored Destination Port UDP: %d\n", config.dest_port_UDP);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &dest_port_TCP_Head);
    config.dest_port_TCP_Head = dest_port_TCP_Head;
    printf("Stored Destination Port TCP Head: %d\n", config.dest_port_TCP_Head);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &dest_port_TCP_Tail);
    config.dest_port_TCP_Tail = dest_port_TCP_Tail;
    printf("Stored Destination Port TCP Tail: %d\n", config.dest_port_TCP_Tail);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &port_TCP);
    config.port_TCP = port_TCP;
    printf("Stored Port TCP: %d\n", config.port_TCP);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &size_UDP_payload);
    config.size_UDP_payload = size_UDP_payload;
    printf("Stored Size UDP Payload: %d\n", config.size_UDP_payload);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &inter_measurement_time_seconds);
    config.inter_measurement_time_seconds = inter_measurement_time_seconds;
    printf("Stored Inter Measurement Time Seconds: %d\n", config.inter_measurement_time_seconds);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &num_UDP_packets);
    config.num_UDP_packets = num_UDP_packets;
    printf("Stored Number of UDP Packets: %d\n", config.num_UDP_packets);

    fscanf(config_file, "%s", temp);
    fscanf(config_file, "%d,", &TTL_UDP);
    config.TTL_UDP = TTL_UDP;
    printf("Stored TTL UDP: %d\n", config.TTL_UDP);

    fclose(config_file);
    return config;
};

int main() {
    struct Config config = parseJson("../config.json");
    printf("Finished parsing json\n");

    // slient socket/address is the socket at the client side
    int client_socket; // socket descriptors
    struct sockaddr_in sin;

    // 1. Create the Socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // TCP
    if (client_socket == -1) {
        perror("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    memset (&sin, 0, sizeof (sin)); // set all bytes to 0

    // 2. Determine server address and port number
    sin.sin_family = AF_INET;
    int ip_valid = inet_pton(AF_INET, config.server_ip_address, &sin.sin_addr);
    if (ip_valid == 0) {
        fprintf(stderr, "inet_pton error: invalid IP address format\n");
        abort();
    } else if (ip_valid < 0) {
        fprintf(stderr, "inet_pton error: %s\n", strerror(errno));
        abort();
    }
    unsigned short port = config.port_TCP; 
    sin.sin_port = htons(port);

    // 3. Connect to the Server
    if (connect(client_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("Connection with the server failed...\n");
        abort();
    }
    else
        printf("Connected to the server..\n");


    // 3. Send Data
    // char message[] = "Hello, server!";
    int sent_bytes = send(client_socket, &config, sizeof(struct Config), 0);
    if (sent_bytes < 0) {
        perror("send failed");
        abort();
    }
    printf("Sent %d bytes to server\n", sent_bytes);

    // 4. Close the Connection
    if (close(client_socket) < 0) {
        perror("close failed");
        return -1;
    }
    return 0;
}