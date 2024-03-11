//
// Created by Dmitry Galkin on 05.03.2024.
//

#ifndef SERVER_H
#define SERVER_H
#include <sys/_pthread/_pthread_cond_t.h>
#include <sys/_pthread/_pthread_mutex_t.h>

// Глобальные переменные для управления доступом
extern pthread_mutex_t write_mutex;
extern pthread_mutex_t read_mutex;
extern pthread_mutex_t priority_mutex;
extern pthread_cond_t write_priority;
extern pthread_cond_t read_priority;

int num_readers = 0;
int num_writers_waiting = 0;
int read_count = 0;
int write_count = 0;
int patience_threshold_read = 3;
int patience_threshold_write = 10;

void *handle_client(void *arg);

// Инициализация мьютексов и условных переменных
void init_server_resources();

// Деинициализация ресурсов сервера
void destroy_server_resources();

int setup_server_socket(int *server_fd);

#endif //SERVER_H
