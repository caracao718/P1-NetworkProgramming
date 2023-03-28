#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>


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
    int pre_tcp_server_socket, pre_tcp_client_socket, pre_tcp_addr_len; 
    struct sockaddr_in pre_tcp_server_address, pre_tcp_client_address;

    // PHASE 1: pre-probing TCP connection

    // 1. create socket
    pre_tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (pre_tcp_server_socket == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");
    
    memset(&pre_tcp_server_address, 0, sizeof(pre_tcp_server_address));

    // 2. Define the Server Address and Port Number
    pre_tcp_server_address.sin_family = AF_INET;
    pre_tcp_server_address.sin_addr.s_addr = INADDR_ANY; 
    pre_tcp_server_address.sin_port = htons(LISTEN_PORT);

    // 3. Bind the Server Socket to the Address and Port Number. Bind newly created socket to given IP and verification
    if (bind(pre_tcp_server_socket, (struct sockaddr *)&pre_tcp_server_address, sizeof(pre_tcp_server_address)) < 0) {
        perror("Bind failed\n");
        abort();
    } else {
        printf("Bind successful\n");
    }

    // 4. Listen for Connections
    if (listen(pre_tcp_server_socket, 1) < 0) {
        perror("Pre Probing TCP Listen failed\n");
        abort();
    } else {
        printf("Pre Probing TCP Listening...\n");
    }
    pre_tcp_client_socket, pre_tcp_addr_len = sizeof(pre_tcp_client_address);

    // 5. Accept a Connection
    pre_tcp_client_socket = accept(pre_tcp_server_socket, (struct sockaddr *)&pre_tcp_client_address, &pre_tcp_addr_len);
    if (pre_tcp_client_socket < 0) {
        perror("Pre Probing TCP server accept failed...\n");
        abort();
    }
    else
        printf("Pre Probing TCP server accepted the client...\n");

    // 6. Receive Data
    struct Config config;
    // read the message from client and copy it in buffer
    int pre_tcp_byte_received = recv(pre_tcp_client_socket, &config, sizeof(struct Config), 0);
    if (pre_tcp_byte_received < 0) {
        perror("Receive failed\n");
        abort();
    } else if (pre_tcp_byte_received == 0) {
        printf("Client disconnected\n");
    } else {
        printf("Received %d bytes\n", pre_tcp_byte_received);
    }
    // print buffer which contains the client contents
    printf("Pre Probing TCP Server address from client: %s\n", config.server_ip_address);

    // 7. Close the Connection
    printf("Now closing TCP pre-probing sockets\n");
    if (close(pre_tcp_server_socket) < 0) {
        perror("Pre Probing TCP Close failed\n");
        return -1;
    } else {
        printf("Pre Probing TCP Close successful\n");
    }
    if (close(pre_tcp_client_socket) < 0) {
        perror("Pre Probing TCP Close failed\n");
        return -1;
    } else {
        printf("Pre Probing TCP Close successful\n");
    }


    // PHASE 2: UDP probing

    // 1. create socket
    struct sockaddr_in udp_addr;
    int udp_addr_len;
    int zero_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (zero_udp_socket == -1) {
        printf("UDP socket creation failed...\n");
        exit(0);
    } else
        printf("UDP Socket successfully created..\n");

    // increase the socket buffer size
    int buffer_size = 1024 * 1024;
    if (setsockopt(zero_udp_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("couldn't set socket buffer size");
        abort();
    }


    // 2. Define the Server Address and Port Number
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(config.dest_port_UDP);

    // Reuse Port
    int opt = 1;
    if (setsockopt(zero_udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("couldn't reuse address");
        abort();
    }

    // 3. Bind the Server Socket to the Address and Port Number. Bind newly created socket to given IP and verification
    if (bind(zero_udp_socket, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP Bind failed\n");
        abort();
    } else {
        printf("UDP Bind successful\n");
    }

    // Set up the timeout value
    struct timeval timeout;
    timeout.tv_sec = config.inter_measurement_time_seconds;
    timeout.tv_usec = 0;

    // Set the socket to non-blocking mode
    int flags = fcntl(zero_udp_socket, F_GETFL, 0);
    fcntl(zero_udp_socket, F_SETFL, flags | O_NONBLOCK);


    // 4. Receive Data
    long zeros_duration_ms = 0;
    long random_duration_ms = 0;
    int udp_byte_received = 0;
    struct timeval zeros_start_time;
    gettimeofday(&zeros_start_time, NULL);

    // Use select() to wait for incoming packets with timeout
    fd_set read_fds;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(zero_udp_socket, &read_fds);

        int ret = select(zero_udp_socket + 1, &read_fds, NULL, NULL, &timeout);
        if (ret < 0) {
            perror("select() failed\n");
        } else if (ret == 0) {
            printf("ZERO: Timeout\n");
            break;
        } else {
            // Data is available to be read from the socket
            char buffer[config.size_UDP_payload];
            udp_byte_received = recvfrom(zero_udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&udp_addr, &udp_addr_len);
            printf("ZERO packetes: received %d bytes from client\n", udp_byte_received);
        }
    }
    struct timeval zeros_end_time;
    gettimeofday(&zeros_end_time, NULL);
    zeros_duration_ms = (zeros_end_time.tv_sec - zeros_start_time.tv_sec) * 1000 + (zeros_end_time.tv_usec - zeros_start_time.tv_usec) / 1000;
    
    // 5. Close the Connection
    printf("Now closing UDP ZEROS sockets\n");
    if (close(zero_udp_socket) < 0) {
        perror("UDP Close failed\n");
        return -1;
    } else {
        printf("UDP socket for ZEROS closed successful\n");
    }
    
    // receive random data
    // create socket
    int udp_random_addr_len;
    struct sockaddr_in udp_random_addr;
    int random_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (random_udp_socket == -1) {
        printf("UDP socket creation failed...\n");
        exit(0);
    } else
        printf("UDP Socket successfully created..\n");
    
    // increase the socket buffer size
    if (setsockopt(random_udp_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("couldn't set socket buffer size");
        abort();
    }

    // Reuse Port
    if (setsockopt(random_udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("couldn't reuse address");
        abort();
    }

    // Define the server address and port number
    memset(&udp_random_addr, 0, sizeof(udp_random_addr));
    udp_random_addr.sin_family = AF_INET;
    udp_random_addr.sin_addr.s_addr = inet_addr(config.server_ip_address);
    udp_random_addr.sin_port = htons(config.dest_port_UDP);

    // Bind newly created socket to given IP and verification
    if (bind(random_udp_socket, (struct sockaddr *)&udp_random_addr, sizeof(udp_random_addr)) < 0) {
        perror("UDP Bind failed\n");
        abort();
    } else {
        printf("UDP Bind successful\n");
    }

    // Set the socket to non-blocking mode
    flags = fcntl(random_udp_socket, F_GETFL, 0);
    fcntl(random_udp_socket, F_SETFL, flags | O_NONBLOCK);

    sleep(5);
    printf("Slept for 5 seconds between ZEROS and RANDOM\n");

    // Set up the timeout value
    struct timeval timeout_random;
    timeout_random.tv_sec = config.inter_measurement_time_seconds;
    timeout_random.tv_usec = 0;

    struct timeval random_start_time;
    gettimeofday(&random_start_time, NULL);
    fd_set read_fds_random;
    while (1) {
        FD_ZERO(&read_fds_random);
        FD_SET(random_udp_socket, &read_fds_random);

        int ret = select(random_udp_socket + 1, &read_fds_random, NULL, NULL, &timeout_random);
        if (ret < 0) {
            perror("select() failed\n");
        } else if (ret == 0) {
            printf("RANDOM: Timeout\n");
            break;
        } else {
            // Data is available to be read from the socket
            char buffer[config.size_UDP_payload];
            udp_byte_received = recvfrom(random_udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&udp_random_addr, &udp_random_addr_len);
            if (udp_byte_received == -1) {
                perror("recvfrom() failed\n");
            }
            printf("RANDOM packets: received %d bytes from client\n", udp_byte_received);
        }
    }
    struct timeval random_end_time;
    gettimeofday(&random_end_time, NULL);
    random_duration_ms = (random_end_time.tv_sec - random_start_time.tv_sec) * 1000 + (random_end_time.tv_usec - random_start_time.tv_usec) / 1000;


    
    // 5. Close the Connection
    printf("Now closing UDP RANDOM probing sockets\n");
    if (close(random_udp_socket) < 0) {
        perror("Close failed\n");
        return -1;
    } else {
        printf("UDP socket for random closed successful\n");
    }

    long delta_t = random_duration_ms - zeros_duration_ms;
    printf("delta_t is %ld\n", delta_t);

    if (delta_t > 100) {
        printf("Compression detected!");
    } else {
        printf("Compression not detected!");
    }
    


    return 0;
}