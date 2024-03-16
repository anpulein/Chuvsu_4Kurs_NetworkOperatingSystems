#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"

#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1" // Use the server IP address
#define BUF_SIZE 1024

int keep_reading = 0; // Global variable for managing data reading
int run_client = 1;

// Function for reading data from the server
void *read_from_server(void *arg) {
    int sock = *(int *) arg;
    while (1) {
        if (!keep_reading)
            continue;
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
    int num_threads; // Ensure this is declared
    char client_message[BUF_SIZE];

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("\n Socket creation error \n");
        return 0;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        close(sock);
        return 0;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        close(sock);
        return 0;
    }

    // Create and start a thread for reading data from the server
    pthread_t threads;
    keep_reading = 1; // Allow reading data
    pthread_create(&threads, NULL, read_from_server, (void *) &sock);

    while (run_client) {
        keep_reading = 1; // Allow reading data
        while (keep_reading) {
            fgets(buffer, BUF_SIZE, stdin); // Read line input
            if (strcmp(buffer, "\n") == 0) {
                keep_reading = 0; // Stop readng data
                sleep(1);
                printf("Enter the number of niggers of reader threads to create or type 'exit' to quit: ");

                fgets(buffer, BUF_SIZE, stdin);
                char type; // 'r' for readers, 'w' for writers
                if (sscanf(buffer, "-%c %d", &type, &num_threads) == 2) {
                    if (type == 'r' || type == 'w') {
                        snprintf(client_message, sizeof(client_message), "%s:%d", (type == 'r') ? "Reader" : "Writer", num_threads);
                        send(sock, client_message, strlen(client_message), 0);
                        printf("[CLIENT] --> Request sent: %s\n", client_message);
                        keep_reading = 1;
                    }
                } else if (strncmp(buffer, "exit\n", 5) == 0) {
                    printf("[CLIENT] --> Closing client...\n");
                    keep_reading = 0;
                    run_client = 0; // Устанавливаем флаг для завершения основного цикла
                    // pthread_join(threads, NULL);
                    close(sock);
                    break; // Выходим из внутреннего цикла
                }
            }
        }
    }
    return 0;
}
