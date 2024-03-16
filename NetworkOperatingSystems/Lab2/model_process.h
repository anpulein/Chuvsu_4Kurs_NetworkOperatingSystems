//
// Created by Dmitry Galkin on 17.02.2024.
//

#ifndef MODEL_PROCESS_H
#define MODEL_PROCESS_H
#include <sys/semaphore.h>

typedef struct SharedData {
    int num_readers;
    int num_writers_waiting;
    int read_count;
    int write_count;
    int patience_threshold_read;
    int patience_threshold_write;
} SharedData;

// Процесс для читателей
void *reader_process(int arg);
// void reader_process(int id, SharedData *data, sem_t *sem_write, sem_t *sem_read, sem_t *sem_priority);
// Процесс для писателей
void *writer_process(int arg);
// void writer_process(int id, SharedData *data, sem_t *sem_write, sem_t *sem_read, sem_t *sem_priority);

void initialize_shared_resources(SharedData **data, sem_t **sem_write, sem_t **sem_read, sem_t **sem_priority);

void destroy_shared_resources(SharedData *data, sem_t *sem_write, sem_t *sem_read, sem_t *sem_priority);

// Запуск программы
void start_process();

#endif //MODEL_PROCESS_H
