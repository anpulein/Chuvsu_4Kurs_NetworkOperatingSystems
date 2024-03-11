//
// Created by Dmitry Galkin on 05.03.2024.
//
// client_reader.c - код клиента читателя
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1" // Используйте IP-адрес сервера
#define BUF_SIZE 1024


int main() {
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};
    char reader_message[BUF_SIZE];

    while(1) {
        printf("Enter the number of writer threads to create or type 'exit' to quit: ");
        fgets(buffer, BUF_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        int num_readers = atoi(buffer);
        if (num_readers > 0) {
            // Перемещаем создание сокета и подключение к серверу внутрь цикла
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                printf("\n Socket creation error \n");
                continue; // Переходим к следующей итерации цикла
            }

            memset(&serv_addr, '0', sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(SERVER_PORT);

            if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
                printf("\nInvalid address/ Address not supported \n");
                close(sock);
                continue;
            }

            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nConnection Failed \n");
                close(sock);
                continue;
            }

            snprintf(reader_message, sizeof(reader_message), "Writer:%d", num_readers);
            send(sock, reader_message, strlen(reader_message), 0);
            printf("Writer request sent: %s\n", reader_message);

            while(1) {
                char server_response[BUF_SIZE] = {0};
                int bytes_read = recv(sock, server_response, BUF_SIZE, 0); // Чтение сообщений от сервера
                if(bytes_read <= 0) {
                    break; // Выходим из цикла, если соединение закрыто или произошла ошибка
                }
                printf("%s", server_response); // Выводим сообщение от сервера
            }

            close(sock); // Закрываем сокет после завершения передачи
        } else {
            printf("Please enter a valid number of writers or type 'exit'.\n");
        }
    }

    return 0;
}
