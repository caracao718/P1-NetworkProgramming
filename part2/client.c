#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
// #include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define IP_SIZE 16


#define MAX_RETRIES 3
#define TIMEOUT_SEC 5


struct tcphdr {
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
    uint16_t res1:4, doff:4, fin:1, syn:1, rst:1, psh:1, ack:1, urg:1, ece:1, cwr:1;
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};

// Define the packet structure
struct packet {
    struct iphdr ip;
    struct tcphdr tcp;
};

// Helper function to calculate the checksum of a packet
unsigned short checksum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

// Helper function to print an error message and terminate the program
void error(const char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

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

    FILE* config_file = fopen(json_string, "r");
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./client <config_file>\n");
        exit(1);
    }
    char *str = argv[1];
    struct Config config = parseJson(str);
    printf("Finished parsing json\n");

    // Resolve the server IP address
    struct sockaddr_in server_addr;
    // if (inet_aton(config.server_ip_address, &server_addr.sin_addr) == 0)
    //     error("Invalid server IP address");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.dest_port_UDP);
    server_addr.sin_addr.s_addr = inet_addr(config.server_ip_address);


    // Create the socket
    int sock;
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) == -1)
        error("Failed to create socket");
    
    // Set the socket options
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1)
        error("Failed to set socket option");
    
    // Construct the SYN packet to port x
    struct packet syn_packet_x;
    memset(&syn_packet_x, 0, sizeof(syn_packet_x));
    syn_packet_x.ip.version = 4;
    syn_packet_x.ip.ihl = 5;
    syn_packet_x.ip.tos = 0;
    syn_packet_x.ip.tot_len = htons(sizeof(struct iphdr) + sizeof(syn_packet_x.tcp));
    syn_packet_x.ip.id = htons(54321);
    syn_packet_x.ip.frag_off = htons(16384);
    syn_packet_x.ip.ttl = config.TTL_UDP;
    syn_packet_x.ip.protocol = IPPROTO_TCP;
    syn_packet_x.ip.saddr = inet_addr(config.client_ip_address);
    syn_packet_x.ip.daddr = inet_addr(config.server_ip_address);
    syn_packet_x.tcp.source = htons(1234);
    syn_packet_x.tcp.dest = htons(config.dest_port_TCP_Head);
    syn_packet_x.tcp.seq = htonl(1);
    syn_packet_x.tcp.doff = 5;
    syn_packet_x.tcp.syn = 1;
    syn_packet_x.tcp.window = htons(5840);
    syn_packet_x.tcp.check = 0;
    syn_packet_x.tcp.urg_ptr = 0;
    syn_packet_x.tcp.check = checksum((unsigned short *)&syn_packet_x.ip, sizeof(struct iphdr) + sizeof(syn_packet_x.tcp));


    // Construct the SYN packet to port y
    struct packet syn_packet_y;
    memset(&syn_packet_y, 0, sizeof(syn_packet_y));
    syn_packet_y.ip.version = 4;
    syn_packet_y.ip.ihl = 5;
    syn_packet_y.ip.tos = 0;
    syn_packet_y.ip.tot_len = htons(sizeof(struct iphdr) + sizeof(syn_packet_y.tcp));
    syn_packet_y.ip.id = htons(54321);
    syn_packet_y.ip.frag_off = htons(16384);
    syn_packet_y.ip.ttl = config.TTL_UDP;
    syn_packet_y.ip.protocol = IPPROTO_TCP;
    syn_packet_y.ip.saddr = inet_addr(config.client_ip_address);
    syn_packet_y.ip.daddr = inet_addr(config.server_ip_address);
    syn_packet_y.tcp.source = htons(1234);
    syn_packet_y.tcp.dest = htons(config.dest_port_TCP_Tail);
    syn_packet_y.tcp.seq = htonl(1);
    syn_packet_y.tcp.doff = 5;
    syn_packet_y.tcp.syn = 1;
    syn_packet_y.tcp.window = htons(5840);
    syn_packet_y.tcp.check = 0;
    syn_packet_y.tcp.urg_ptr = 0;
    syn_packet_y.tcp.check = checksum((unsigned short *)&syn_packet_y.ip, sizeof(struct iphdr) + sizeof(syn_packet_y.tcp));


    // Create UDP socket
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

    // set TTL value
    if (setsockopt(sock, SOL_SOCKET, IP_TTL, &config.TTL_UDP, sizeof(config.TTL_UDP)) < 0) {
        perror("Failed to set TTL value");
        exit(1);
    }
    
    // 2. set DF bit in IP header
    int on = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, IP_MTU_DISCOVER, &on, sizeof(on)) < 0) {
        perror("setsockopt failed");
        exit(1);
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


    // Send the packets
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    if (sendto(sock, &syn_packet_x, sizeof(syn_packet_x), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        error("Failed to send SYN packet to port x");
    
    // Send UDP packet
    int udp_sent_bytes = 0;
    uint16_t packet_id = 0;
    for (int i = 0; i < config.num_UDP_packets; i++) {
        char udp_message[config.size_UDP_payload];
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
        printf("Next packet ID: %d\n", packet_id);
    }

    if (sendto(sock, &syn_packet_y, sizeof(syn_packet_y), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        error("Failed to send SYN packet to port y");

    // Wait for the RST packets
    struct timeval timeout = {TIMEOUT_SEC, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    int retries = 0;
    struct timeval rst_time1, rst_time2;
    while (retries < MAX_RETRIES) {
        if (select(sock+1, &fds, NULL, NULL, &timeout) == 1) {
            struct packet rst_packet;
            memset(&rst_packet, 0, sizeof(rst_packet));
            ssize_t len = recv(sock, &rst_packet, sizeof(rst_packet), 0);
            if (len == -1)
                error("Failed to receive packet");
            if (len < sizeof(struct iphdr) + sizeof(syn_packet_x.tcp))
                continue;
            if (rst_packet.tcp.rst && rst_packet.tcp.source == htons(config.dest_port_TCP_Head)) {
                gettimeofday(&rst_time1, NULL);
                retries++;
            }
            if (rst_packet.tcp.rst && rst_packet.tcp.source == htons(config.dest_port_TCP_Tail)) {
                gettimeofday(&rst_time2, NULL);
                retries++;
            }
        } else {
            retries++;
        }
    }


    // Calculate the difference between the RST packet arrival times
    if (retries < MAX_RETRIES) {
        long time_diff_us = (rst_time2.tv_sec - rst_time1.tv_sec) * 1000000 + (rst_time2.tv_usec - rst_time1.tv_usec);
        printf("Detected network compression (RST time difference: %ld microseconds)\n", time_diff_us);
    } else {
        printf("Failed to detect due to insufficient information\n");
    }

    // Close the socket
    if (close(sock) < 0) {
        error("Failed to close socket");
    } else {
        printf("Closed socket\n");
    }

    if (close(udp_socket) < 0) {
        error("Failed to close UDP socket");
    } else {
        printf("Closed UDP socket\n");
    }

    return 0;

}