//
// Created by Dmitry Galkin on 05.03.2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"

#include <signal.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUF_SIZE 1024


// #define NUM_READERS 5
// #define NUM_WRITERS 5

struct thread_data {
    int sock;
    int index;
    volatile int *active; // Указатель на переменную, контролирующую активность потока
};

pthread_mutex_t write_mutex;
pthread_mutex_t read_mutex;
pthread_mutex_t priority_mutex;
pthread_cond_t write_priority;
pthread_cond_t read_priority;

void init_server_resources() {
    pthread_mutex_init(&write_mutex, NULL);
    pthread_mutex_init(&read_mutex, NULL);
    pthread_mutex_init(&priority_mutex, NULL);
    pthread_cond_init(&write_priority, NULL);
    pthread_cond_init(&read_priority, NULL);
}

void destroy_server_resources() {
    pthread_mutex_destroy(&write_mutex);
    pthread_mutex_destroy(&read_mutex);
    pthread_mutex_destroy(&priority_mutex);
    pthread_cond_destroy(&write_priority);
    pthread_cond_destroy(&read_priority);
}

void *reader_thread(void *arg);
void *writer_thread(void *arg);

int main() {
    setbuf(stdout, NULL);
    init_server_resources();

    int server_fd, new_socket;
    pthread_t tid;

    if (setup_server_socket(&server_fd) < 0) {
        destroy_server_resources();
        exit(1); // Возвращаем ошибку, но уже после освобождения ресурсов
    }

    printf("Server is running and waiting for connections on port %d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, NULL, NULL)) < 0) {
            perror("accept");
            continue; // Вместо выхода продолжаем работу сервера
        }
        pthread_create(&tid, NULL, handle_client, (void *)(intptr_t)new_socket);
    }

}

int setup_server_socket(int *server_fd) {
    struct sockaddr_in address;
    int opt = 1;


    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return -1;
    }

    // Настройка сокета для повторного использования порта
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ opt }, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(*server_fd);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Привязка ко всем интерфейсам
    address.sin_port = htons(PORT);

    if (bind(*server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(*server_fd);
        return -1;
    }

    if (listen(*server_fd, 10) < 0) {
        perror("listen");
        close(*server_fd);
        return -1;
    }

    return 0; // Успешное завершение
}


// void *handle_client(void *arg) {
//     int sock = (intptr_t)arg;
//     char buffer[BUF_SIZE] = {0};
//     int read_val, num_threads;
//     char *token, *ptr;
//     volatile int active = 1; // Флаг активности потоков
//
//     // Чтение сообщения от клиента
//     read_val = read(sock, buffer, BUF_SIZE);
//     if (read_val > 0) {
//         // Разделение сообщения на токены
//         token = strtok(buffer, ":");
//         if (token != NULL) {
//             ptr = strtok(NULL, ":");
//             if (ptr != NULL) {
//                 num_threads = atoi(ptr); // Преобразование строки в число
//
//                 printf("[CLIENT] --> Client %s create threads: %d\n", strcmp(token, "Reader") ==0 ? "Reader" : "Writer", num_threads);
//
//                 struct thread_data *data;
//                 pthread_t threads[num_threads];
//                 for (int i = 0; i < num_threads; i++) {
//                     data = malloc(sizeof(struct thread_data)); // Выделяем память для данных потока
//                     data->sock = sock;
//                     data->index = i;
//                     // data->active = &active;
//                     if (strcmp(token, "Reader") == 0) {
//                         // Создание потоков читателей
//                         pthread_create(&threads[i], NULL, reader_thread, (void *)data);
//                     } else if (strcmp(token, "Writer") == 0) {
//                         // Создание потоков писателей
//                         pthread_create(&threads[i], NULL, writer_thread, (void *)data);
//                     }
//                 }
//
//                 // usleep(100000);
//                 // active = 0;
//                 for (int i = 0; i < num_threads; i++) {
//                     pthread_join(threads[i], NULL);
//                 }
//
//                 // Отправляем подтверждение клиенту
//                 // char *message = "Threads created successfully";
//                 // send(sock, message, strlen(message), 0);
//             }
//         }
//     } else {
//         if (read_val == 0) {
//             printf("Client disconnected\n");
//         } else {
//             perror("recv failed");
//         }
//     }
//
//     close(sock);
//     return NULL;
// }

void *handle_client(void *arg) {
    int sock = (intptr_t)arg;
    char buffer[BUF_SIZE] = {0};
    int read_val, num_threads;
    char *token, *ptr;
    volatile int active = 1; // Флаг активности потоков

    printf("[CLIENT]");

    while (1) { // Бесконечный цикл чтения
        memset(buffer, 0, BUF_SIZE); // Очистите буфер перед каждым чтением
        printf("[CLIENT] before read");
        int read_val = read(sock, buffer, BUF_SIZE);

        printf("[CLIENT] after read");

        if (read_val > 0) {
            // Разделение сообщения на токены
            token = strtok(buffer, ":");
            if (token != NULL) {
                ptr = strtok(NULL, ":");
                if (ptr != NULL) {
                    num_threads = atoi(ptr); // Преобразование строки в число

                    printf("[CLIENT] --> Client %s create threads: %d\n", strcmp(token, "Reader") ==0 ? "Reader" : "Writer", num_threads);

                    struct thread_data *data;
                    pthread_t threads[num_threads];
                    for (int i = 0; i < num_threads; i++) {
                        data = malloc(sizeof(struct thread_data)); // Выделяем память для данных потока
                        data->sock = sock;
                        data->index = i;
                        // data->active = &active;
                        if (strcmp(token, "Reader") == 0) {
                            // Создание потоков читателей
                            pthread_create(&threads[i], NULL, reader_thread, (void *)data);
                        } else if (strcmp(token, "Writer") == 0) {
                            // Создание потоков писателей
                            pthread_create(&threads[i], NULL, writer_thread, (void *)data);
                        }
                    }

                    // usleep(100000);
                    // active = 0;
                    for (int i = 0; i < num_threads; i++) {
                        pthread_join(threads[i], NULL);
                    }

                    // Отправляем подтверждение клиенту
                    // char *message = "Threads created successfully";
                    // send(sock, message, strlen(message), 0);
                }
            }
            printf("end...");
        } else {
            if (read_val == 0) {
                printf("Client disconnected\n");
            } else {
                perror("recv failed");
            }
            active = 0; // Установите флаг, чтобы выйти из цикла
        }
    }

    close(sock);
    return NULL;
}


void *reader_thread(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock = data->sock;
    int index = data->index;

    while (1) {
        pthread_mutex_lock(&priority_mutex);
        while (write_count > 0 || (num_writers_waiting > 0 && read_count >= patience_threshold_read)) {
            pthread_cond_wait(&read_priority, &priority_mutex);
        }
        read_count++;
        pthread_mutex_unlock(&priority_mutex);

        pthread_mutex_lock(&read_mutex);
        num_readers++;
        if (num_readers == 1) {
            pthread_mutex_lock(&write_mutex);
        }
        pthread_mutex_unlock(&read_mutex);

        char stats_message[BUF_SIZE];
        char inMessage[] = "[READER] --> Читатель %ld читает в библиотеке\n";
        char outMessage[] = "[READER] --> Читатель %ld прочитал книгу в библиотеке\n";
        // Чтение
        printf(inMessage, (intptr_t)index);
        snprintf(stats_message, sizeof(stats_message), inMessage, (intptr_t)index);
        send(sock, stats_message, strlen(stats_message), 0); // Отправляем сообщение клиенту
        usleep(rand() % 1000000);
        printf(outMessage, (intptr_t)index);
        snprintf(stats_message, sizeof(stats_message), outMessage, (intptr_t)index);
        send(sock, stats_message, strlen(stats_message), 0); // Отправляем сообщение клиенту

        pthread_mutex_lock(&read_mutex);
        num_readers--;
        if (num_readers == 0) {
            pthread_mutex_unlock(&write_mutex);
        }
        pthread_mutex_unlock(&read_mutex);

        pthread_mutex_lock(&priority_mutex);
        read_count--;
        if (num_writers_waiting > 0 && read_count < patience_threshold_read) {
            pthread_cond_signal(&write_priority);
        }
        pthread_mutex_unlock(&priority_mutex);

        usleep(rand() % 1000000);
    }

    return NULL;
}

void *writer_thread(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock = data->sock;
    int index = data->index;

    while (1) {
        pthread_mutex_lock(&priority_mutex);
        num_writers_waiting++;
        while (num_readers > 0 || write_count > 0) {
            pthread_cond_wait(&write_priority, &priority_mutex);
        }
        num_writers_waiting--;
        write_count++;
        pthread_mutex_unlock(&priority_mutex);

        pthread_mutex_lock(&write_mutex);
        char stats_message[BUF_SIZE];
        char inMessage[] = "[WRITER] --> Писатель %ld пишет в библиотеке\n";
        char outMessage[] = "[WRITER] --> Писатель %ld закончил писать в библиотеке\n";
        // Запись
        printf(inMessage, (intptr_t)index);
        snprintf(stats_message, sizeof(stats_message), inMessage, (intptr_t)index);
        send(sock, stats_message, strlen(stats_message), 0); // Отправляем сообщение клиенту
        usleep(rand() % 3000000);
        printf(outMessage, (intptr_t)index);
        snprintf(stats_message, sizeof(stats_message), outMessage, (intptr_t)index);
        send(sock, stats_message, strlen(stats_message), 0); // Отправляем сообщение клиенту
        pthread_mutex_unlock(&write_mutex);

        pthread_mutex_lock(&priority_mutex);
        write_count--;
        if (write_count == 0) {
            if (num_writers_waiting > 0 && read_count >= patience_threshold_read) {
                pthread_cond_signal(&write_priority);
            } else {
                pthread_cond_broadcast(&read_priority);
            }
        }
        pthread_mutex_unlock(&priority_mutex);

        usleep(rand() % 3000000);
    }
}