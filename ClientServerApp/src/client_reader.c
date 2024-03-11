#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"

#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1" // Use the server IP address
#define BUF_SIZE 1024

volatile int keep_reading = 0; // Global variable for managing data reading

// Function for reading data from the server
void *read_from_server(void *arg) {
    int sock = *(int *) arg;
    while (keep_reading) {
        char server_response[BUF_SIZE] = {0};
        int bytes_read = recv(sock, server_response, BUF_SIZE, 0); // Reading messages from the server
        if (bytes_read <= 0) {
            break; // Exit the loop if the connection is closed or an error occurred
        }
        printf("%s", server_response); // Print message from the server
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};
    int num_readers; // Ensure this is declared
    char reader_message[BUF_SIZE];

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            printf("\n Socket creation error \n");
            continue;
        }

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);

        if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported \n");
            close(sock);
            continue;
        }

        if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            printf("\nConnection Failed \n");
            close(sock);
            continue;
        }

        // Create and start a thread for reading data from the server
        pthread_t reader_thread;
        keep_reading = 1; // Allow reading data
        pthread_create(&reader_thread, NULL, read_from_server, (void *) &sock);

        while (keep_reading) {
            fgets(buffer, BUF_SIZE, stdin); // Read line input
            if (strcmp(buffer, "\n") == 0) {
                keep_reading = 0; // Stop reading data
                printf("Enter the number of reader threads to create or type 'exit' to quit: ");

                fgets(buffer, BUF_SIZE, stdin);

                // Handle commands
                if (strncmp(buffer, "exit\n", 5) == 0) {
                    printf("[CLIENT] --> Client close...");
                    break;
                }

                num_readers = atoi(buffer);
                if (num_readers > 0) {
                    snprintf(reader_message, sizeof(reader_message), "Reader:%d", num_readers);
                    send(sock, reader_message, strlen(reader_message), 0);
                    printf("[CLIENT] --> Reader request sent: %s\n", reader_message);
                    keep_reading = 1; // Only allow reading again if input was correct
                }
            }
        }

        // Wait for the reading thread to finish
        pthread_join(reader_thread, NULL);

        close(sock); // Close the socket
    }
    return 0;
}
