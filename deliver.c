#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#define MESSAGE_SIZE 2048      // maximum message size
#define MAX_FILENAME 254       // 255 including null terminator
#define TIMEOUT_INTERVAL 2000  // timeout in milliseconds (2 seconds)

// Get the current time in milliseconds
long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server address> <server port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_address = argv[1];
    int server_port = atoi(argv[2]);
    int sockfd;
    char file_name[MAX_FILENAME];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) < 0) {
        perror("fcntl failed");
        close(sockfd);
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

    // Ask the user for the file name
    printf("Enter filename to send: ");
    fgets(file_name, MAX_FILENAME, stdin);
    file_name[strcspn(file_name, "\n")] = '\0';

    // Check if file exists
    if (access(file_name, F_OK) != 0) {
        perror("File does not exist");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Open file in binary mode
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char packet_data[1000];
    int frag_count = 0;
    int total_count = 0;
    // Pre-allocate an array for up to 10 MB (adjust if needed)
    struct packet* packet_list[10486] = {0};

    // Read the file and create packets
    while (1) {
        int size = fread(packet_data, 1, 1000, file);
        if (size <= 0)
            break;
        frag_count++;
        if (frag_count > 10486) {
            fprintf(stderr, "File too large!\n");
            break;
        }
        packet_list[frag_count - 1] = malloc(sizeof(struct packet));
        memcpy(packet_list[frag_count - 1]->filedata, packet_data, size);
        packet_list[frag_count - 1]->filename = malloc(MAX_FILENAME);
        strncpy(packet_list[frag_count - 1]->filename, file_name, MAX_FILENAME - 1);
        packet_list[frag_count - 1]->filename[MAX_FILENAME - 1] = '\0';
        packet_list[frag_count - 1]->frag_no = frag_count;
        packet_list[frag_count - 1]->size = size;
        total_count++;
        memset(packet_data, 0, sizeof(packet_data));
    }
    // Set the total_frag field for every packet
    for (int i = 0; i < total_count; i++) {
        packet_list[i]->total_frag = total_count;
    }

    int count = 0;
    // Sending packets with a simple stop-and-wait protocol
    while (count < total_count) {
        char message[MESSAGE_SIZE];
        // Build the packet header (fields separated by colons)
        sprintf(message, "%u:%u:%u:%s:",
                packet_list[count]->total_frag,
                packet_list[count]->frag_no,
                packet_list[count]->size,
                packet_list[count]->filename);
        int header_size = strlen(message);
        // Append file data
        memcpy(message + header_size, packet_list[count]->filedata, packet_list[count]->size);

        // Send the packet
        long send_time = get_current_time_ms();
        if (sendto(sockfd, message, header_size + packet_list[count]->size, 0,
                   (struct sockaddr *)&server_addr, addr_len) < 0) {
            perror("sendto failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Packet %d sent to server...\n", packet_list[count]->frag_no);

        // Wait for ACK with fixed timeout
        int ack_received = 0;
        while (!ack_received) {
            long current_time = get_current_time_ms();
            if (current_time - send_time > TIMEOUT_INTERVAL) {
                // Timeout occurred; retransmit the same packet
                printf("Timeout: No ACK received for packet %d. Retransmitting...\n", packet_list[count]->frag_no);
                break;
            }
            char response[20];
            int recv_len = recvfrom(sockfd, response, sizeof(response) - 1, 0,
                                    (struct sockaddr *)&server_addr, &addr_len);
            if (recv_len > 0) {
                response[recv_len] = '\0';
                printf("Server response: %s\n", response);
                if (strcmp(response, "ACK") == 0) {
                    printf("Packet %d acknowledged by server. Moving to next packet.\n", packet_list[count]->frag_no);
                    ack_received = 1;
                    count++;
                } else if (strcmp(response, "NACK") == 0) {
                    printf("Packet %d NOT acknowledged by server. Retransmitting...\n", packet_list[count]->frag_no);
                    break;
                }
            } else {
                usleep(10000);  // sleep briefly to avoid busy waiting
            }
        }
    }

    // Clean up memory and close the file/socket
    for (int i = 0; i < total_count; i++) {
        free(packet_list[i]->filename);
        free(packet_list[i]);
    }
    fclose(file);
    close(sockfd);
    return 0;
}
