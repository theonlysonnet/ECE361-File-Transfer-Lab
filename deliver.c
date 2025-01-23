#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server address> <server port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_address = argv[1];
    int server_port = atoi(argv[2]);
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Ask the user for file name
    printf("ftp ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

    //printf("Buffer length: %zu\n", strlen(buffer));

    // Check if the file exists
    if (access(buffer, F_OK) != 0) {
        perror("File does not exist");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    const char *message = "ftp";
    // Send the message to the server
    if (sendto(sockfd, message, strlen(buffer), 0, 
               (const struct sockaddr *)&server_addr, addr_len) < 0) {
        perror("sendto failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Message sent to server: %s\n", buffer);

    // Receive the server's response
    memset(buffer, 0, BUFFER_SIZE);
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                            (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len < 0) {
        perror("recvfrom failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    buffer[recv_len] = '\0'; // Null-terminate the received string
    printf("Server response: %s\n", buffer);

    if (strcmp(buffer, "yes") == 0) {
        printf("A file transfer can start.\n");
    }

    close(sockfd);
    return 0;
}
