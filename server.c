#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>


#define BUFFER_SIZE 1283

// definre a packet struct
struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

double uniform_rand() {
    return (double)rand() / (double)RAND_MAX;
}

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

    FILE *file = NULL;  // File pointer for writing data
    unsigned int expected_frag_no = 1;  // Track expected fragment number
    unsigned int total_fragments = 0;   // Track total number of fragments
    file = fopen("tempname", "wb");

    // Wait for a message from the client
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                                (struct sockaddr *)&client_addr, &addr_len); // gets address info of sender to use later
        if (recv_len < 0) {
            perror("recvfrom failed");
            continue;
        }

        //printf("Received message: %s\n", buffer);
        printf("Received message.\n");
        
        struct packet pkt;

        // make packet with the message
        pkt.total_frag = atoi(strtok(buffer, ":"));
        pkt.frag_no = atoi(strtok(NULL, ":"));
        pkt.size = atoi(strtok(NULL, ":"));
        // Copy filename safely
        char *filename_token = strtok(NULL, ":");
        pkt.filename = malloc(strlen(filename_token) + 1);
        strcpy(pkt.filename, filename_token);
        
        // Find start of binary file data
        char *data_start = filename_token + strlen(filename_token) + 1;  // Move past null terminator

        // Copy binary data safely
        memcpy(pkt.filedata, data_start, pkt.size);

        // Open file if first fragment
        if (pkt.frag_no == 1) {
            if (!file) {
                perror("File creation failed");
                return 1;
            }
            total_fragments = pkt.total_frag;  // Store total expected fragments
            printf("Receiving file: %s (%u fragments)\n", pkt.filename, total_fragments);
        }

        // Ensure correct fragment order
        if (uniform_rand() > 0.01) {
            if (pkt.frag_no == expected_frag_no) {
                fwrite(pkt.filedata, 1, pkt.size, file);
                expected_frag_no++;  // Move to next expected fragment
                printf("Correct fragment received: %u ...sent ACK to send next: %u\n", pkt.frag_no, expected_frag_no);
                const char* response = "ACK";
                if (sendto(sockfd, response, strlen(response), 0, 
                    (struct sockaddr *)&client_addr, addr_len) < 0) {
                    perror("sendto failed");
                } else {
                    printf("Sent response: %s\n", response);
                }
            } else {
                printf("Out-of-order fragment received: %u (expected %u)...sent NACK to resend\n", pkt.frag_no, expected_frag_no);
                const char* response = "NACK";
                if (sendto(sockfd, response, strlen(response), 0, 
                    (struct sockaddr *)&client_addr, addr_len) < 0) {
                    perror("sendto failed");
                } else {
                    printf("Sent response: %s\n", response);
                }
            }
        }
        else {
            printf("Packet %d dropped (simulated packet loss).\n", pkt.frag_no);
        }

        // Close file when all fragments received
        if (pkt.frag_no == total_fragments) {
            fclose(file);
            rename("tempname", pkt.filename);
            printf("File transfer complete: %s\n", pkt.filename);
            break;
        }

        free(pkt.filename);
    }

    close(sockfd);
    return 0;
}
