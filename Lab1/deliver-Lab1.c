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

    char *server_address = argv[1]; // assigning args that were input to strings
    int server_port = atoi(argv[2]); // convert server port from string to int
    int sockfd; // socket descriptor used to create UDP socket later
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr; // sockaddr struct as mentioned by Beej and it is used for IPv4 "the in stands for internet"
    socklen_t addr_len = sizeof(server_addr);

    // Create a UDP socket with IPv4 and Datagram Socket and the 0 is because UDP only uses IPv4
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket failed"); // error checking
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port); // host to network short converts a 16-bit value (e.g., a port number) from the host byte order to the network byte order

    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {  // function converts the string IP address into its binary format and stores in sin_addr
        perror("Invalid server address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //printf("Buffer length: %zu\n", strlen(buffer));
    // Ask the user for the file name
    printf("Enter message (e.g., ftp <file name>): ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

    // Split the input into command and file name
    char *command = strtok(buffer, " ");
    char *file_name = strtok(NULL, " ");

    // Validate the input
    if (!command || strcmp(command, "ftp") != 0 || !file_name) {
        fprintf(stderr, "Invalid input. Usage: ftp <file name>\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Check if the file exists
    if (access(file_name, F_OK) != 0) {
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
                            (struct sockaddr *)&server_addr, &addr_len); // blocking call that receives message from server_addr
    if (recv_len < 0) {  // returns number of bytes received
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
