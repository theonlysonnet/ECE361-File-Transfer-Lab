#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <UDP listen port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int udp_port = atoi(argv[1]); // convert server port from string to int
    int sockfd; // socket descriptor used to create UDP socket later
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr; // two addresses
    socklen_t addr_len = sizeof(client_addr);

    // Create a UDP socket with IPv4 and Datagram Socket and the 0 is because UDP only uses IPv4
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    server_addr.sin_port = htons(udp_port);

    // Bind the socket to the specified port
    // bind associates a socket with an address (IP address + port number)
    // returns -1 on fail
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {  
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", udp_port);

    // Wait for a message from the client
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                                (struct sockaddr *)&client_addr, &addr_len); // gets address info of sender to use later
        if (recv_len < 0) {
            perror("recvfrom failed");
            continue;
        }

        printf("Received message: %s\n", buffer);

        // Respond based on the message
        const char *response;
        if (strcmp(buffer, "ftp") == 0) {
            response = "yes";
        } else {
            response = "no";
        }

        if (sendto(sockfd, response, strlen(response), 0, 
                   (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("sendto failed");
        } else {
            printf("Sent response: %s\n", response);
            break;
        }
        break;
    }
    close(sockfd);
    return 0;
}
