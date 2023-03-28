#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define IP_SIZE 16


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

struct Config parseJson(char *json_string) {
    struct Config config;
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

    FILE* config_file = fopen("../config.json", "r");
    if (!config_file) {
        printf("Error opening config file\n");
        exit(1);
    }
    char temp[100];
    fscanf(config_file, "%s", temp);
    
    fscanf(config_file, "%s", temp);
    int server_values = fscanf(config_file, "%s", server_ip_address);
    for (int i = 0; i < strlen(server_ip_address); i++) {
        server_ip_address[i] = server_ip_address[i + 1];
        if (server_ip_address[i] == '"') {
            server_ip_address[i] = '\0';
            break;
        }
    }
    printf("Server IP: %s\n", server_ip_address);
    strncpy(config.server_ip_address, server_ip_address, sizeof(server_ip_address));
    config.server_ip_address[sizeof(config.server_ip_address)] = '\0';
    printf("Stored Server IP: %s\n", config.server_ip_address);

    fscanf(config_file, "%s", temp);
    int client_values = fscanf(config_file, "%s", client_ip_address);
    for (int i = 0; i < strlen(client_ip_address); i++) {
        client_ip_address[i] = client_ip_address[i + 1];
        if (client_ip_address[i] == '"') {
            client_ip_address[i] = '\0';
            break;
        }
    }
    printf("Client IP: %s\n", client_ip_address);
    strncpy(config.client_ip_address, client_ip_address, sizeof(client_ip_address));
    config.client_ip_address[sizeof(config.client_ip_address)] = '\0';
    printf("Stored Client IP: %s\n", config.client_ip_address);

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
 
    // PHASE 1: Pre-Probing TCP

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
    } else {
        printf("Successfully cloased TCP connection\n");
    }

    // PHASE 2: Probing UDP
    // 1. Create the Socket
    struct sockaddr_in udp_sin;
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (udp_socket == -1) {
        perror("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    memset (&udp_sin, 0, sizeof(udp_sin)); // set all bytes to 0
    
    // 2. set DF bit in IP header
    int on = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, IP_MTU_DISCOVER, &on, sizeof(on)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // 3. Determine server address and port number
    udp_sin.sin_family = AF_INET;
    int udp_ip_valid = inet_pton(AF_INET, config.server_ip_address, &udp_sin.sin_addr);
    if (udp_ip_valid == 0) {
        fprintf(stderr, "inet_pton error: invalid IP address format\n");
        abort();
    } else if (udp_ip_valid < 0) {
        fprintf(stderr, "inet_pton error: %s\n", strerror(errno));
        abort();
    }
    unsigned short udp_port = config.dest_port_UDP;
    udp_sin.sin_port = htons(udp_port);

    // 4. Send Data

    // send all 0
    char udp_message[config.size_UDP_payload];
    int udp_sent_bytes = 0;
    uint16_t packet_id = 0;
    for (int i = 0; i < config.num_UDP_packets; i++) {
        memcpy(udp_message, &packet_id, sizeof(packet_id));
        for (int i = 2; i < config.size_UDP_payload; i++) {
            udp_message[i] = 0x00;
        }
        udp_sent_bytes = sendto(udp_socket, &udp_message, config.size_UDP_payload, 0, (struct sockaddr *)&udp_sin, sizeof(udp_sin));
        if (udp_sent_bytes < 0) {
            perror("send failed");
            abort();
        }
        printf("Sent %d bytes to server\n", udp_sent_bytes);
        packet_id++;
    }

    // send random data
    sleep(config.inter_measurement_time_seconds);
    packet_id = 0;
    for (int i = 0; i < config.num_UDP_packets; i++) {
        memcpy(udp_message, &packet_id, sizeof(packet_id));
        for (int i = 2; i < config.size_UDP_payload; i++) {
            udp_message[i] = rand() % 256;
        }
        udp_sent_bytes = sendto(udp_socket, &udp_message, config.size_UDP_payload, 0, (struct sockaddr *)&udp_sin, sizeof(udp_sin));
        if (udp_sent_bytes < 0) {
            perror("send failed");
            abort();
        }
        printf("Sent %d bytes to server\n", udp_sent_bytes);
        packet_id++;
    }

    // 4. Close the Connection
    if (close(udp_socket) < 0) {
        perror("close failed");
        return -1;
    } else {
        printf("Successfully closed udp_socket\n");
    }

    // PHASE 3: Post-Probing TCP



    return 0;
}