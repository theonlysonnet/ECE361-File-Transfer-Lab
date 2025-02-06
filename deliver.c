#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MESSAGE_SIZE 1283 // theoretical max size for a message string created from a packet
#define MAX_FILENAME 255
int RTT = 50;

// define a packet struct
struct packet {
    unsigned int total_frag; // let's assume max 2^32 which is 10 digits
    unsigned int frag_no; // let's assume max 2^32 which is 10 digits
    unsigned int size; // max 1000 so 4 digits
    char* filename; // let's assume max 255 chars
    char filedata[1000]; 
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server address> <server port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_address = argv[1]; // assigning args that were input to strings
    int server_port = atoi(argv[2]); // convert server port from string to int
    int sockfd; // socket descriptor used to create UDP socket later
    char file_name[MAX_FILENAME];
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

    int timeout = 1.5 * RTT; // timeout value used for retransmission

    //printf("Buffer length: %zu\n", strlen(buffer));
    // Ask the user for the file name
    printf("Enter filename to send: ");
    fgets(file_name, MAX_FILENAME, stdin);
    int file_name_length = strlen(file_name); // calculate file_name length before removing newline char
    file_name[strcspn(file_name, "\n")] = '\0'; // Remove newline character

    // Check if the file exists
    if (access(file_name, F_OK) != 0) {
        perror("File does not exist");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // open file in reading mode and error check
    FILE *file = fopen(file_name, "r");
    if (file== NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char packet_data[1000];
    int frag_count = 0; 
    //int total_count = 0;
    struct packet* packet_list[10486]; //upto 10 MB

    // construct packets and send one by one nininini
    // reads upto 1000 characters and puts it in packet_data
    while( 1 /*checks to see if fully read or not*/ ) {
        int size = fread(packet_data, 1, 1000, file);
        if (size <= 0)
            break;
        
        frag_count++; // if there is a new fragment then add one as check by size
        // reallocate packetlist array to be bigger if more frags than initially 10MB worth
        if (frag_count > 10486 /*double check number*/) {
            
        }

        memcpy(packet_list[frag_count-1]->filedata, packet_data, size); // insert into string for message
        memcpy(packet_list[frag_count-1]->filename, file_name, file_name_length);
        packet_list[frag_count-1]->frag_no = frag_count;
        packet_list[frag_count-1]->size;
        packet_list[frag_count-1]->total_frag++;

        memset(&packet_data, 0, sizeof(packet_data)); // reset string for next read
    }

    int count = 0;
    //sending packets in packet_list
    while (count < packet_list[0]->total_frag) {
        // construct message string from packet in packet_list
        char message[MESSAGE_SIZE];

        char total_count_string[10];
        snprintf(total_count_string, sizeof(total_count_string), "%d", packet_list[count]->total_frag);
        char frag_count_string[10];
        snprintf(frag_count_string, sizeof(frag_count_string), "%d", packet_list[count]->frag_no);
        char size_string[4];
        snprintf(size_string, sizeof(size_string), "%d", packet_list[count]->size);

        //make message
        strncat(message, total_count_string, sizeof(message) - strlen(message) - 1);
        strcat(":", total_count_string);
        strncat(message, frag_count_string, sizeof(message) - strlen(message) - 1);
        strcat(":", total_count_string);
        strncat(message, size_string, sizeof(message) - strlen(message) - 1);
        strcat(":", total_count_string);
        strncat(message, file_name, sizeof(message) - strlen(message) - 1);
        strcat(":", total_count_string);
        memcpy(message + strlen(message), packet_list[count]->filedata, packet_list[count]->size + 1);  // Append manually // double check size + 1 thing

        // send message string
        if (sendto(sockfd, message, sizeof(message), 0, 
                (const struct sockaddr *)&server_addr, addr_len) < 0) {
            perror("sendto failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Packet %d sent to server...\n", packet_list[count]->frag_no);

        char response[20];
        // Receive the server's response
        int recv_len = recvfrom(sockfd, response, 20, 0, 
                                (struct sockaddr *)&server_addr, &addr_len); // blocking call that receives message from server_addr
        if (recv_len < 0) {  // returns number of bytes received
            perror("recvfrom failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        response[recv_len] = '\0'; // Null-terminate the received string
        printf("Server response: %s\n", response);

        if (strcmp(response, "ACK") == 0) {
            printf("Packet %d acknowledged by server...will process next packet.\n", packet_list[count]->frag_no);
            count++;
        }
        else {
            printf("Packet %d NOT acknowledged by server...will process packet again.\n", packet_list[count]->frag_no);
        }
    }
    
    close(sockfd);
    return 0;
}
