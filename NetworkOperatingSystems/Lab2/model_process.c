#include "model_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <string.h>


// Объявления для разделяемой памяти
struct SharedMemory {
    int num_readers;
    int num_writers_waiting;
    int read_count;
    int write_count;
};

int PATIENCE_THRESHOLD_READ = 3;
int PATIENCE_THRESHOLD_WRITE = 5;

// Инициализация именованных семафоров
sem_t *write_mutex;
sem_t *read_mutex;
sem_t *priority_mutex;
sem_t *write_priority;
sem_t *read_priority;

struct SharedMemory *shm, *phm;

void *reader_process(int arg) {
    while (1) {
        sem_wait(priority_mutex);
        while (phm->write_count > 0 || (phm->num_writers_waiting > 0 && phm->read_count >= PATIENCE_THRESHOLD_READ)) {
            sem_post(priority_mutex);
            sem_wait(read_priority);
            sem_wait(priority_mutex);
        }
        phm->read_count++;
        sem_post(priority_mutex);

        sem_wait(read_mutex);
        phm->num_readers++;
        if (phm->num_readers == 1) {
            sem_wait(write_mutex);
        }
        sem_post(read_mutex);

        // Чтение
        printf("[READ] -> Читатель %d читает в библиотеке\n", arg);
        usleep(rand() % 1000000);
        printf("[READ] -> Читатель %d прочитал книгу в библиотеке\n", arg);

        sem_wait(read_mutex);
        phm->num_readers--;
        if (phm->num_readers == 0) {
            sem_post(write_mutex);
        }
        sem_post(read_mutex);

        sem_wait(priority_mutex);
        phm->read_count--;
        // if (shm->read_count == 0) {
        //     sem_post(write_mutex);
        // }
        if (phm->num_writers_waiting > 0 && phm->read_count < PATIENCE_THRESHOLD_WRITE) {
            sem_post(write_priority);
        } else {
            sem_post(read_priority); // Позволяет другим читателям продолжить
        }
        sem_post(priority_mutex);

        usleep(rand() % 1000000);
    }
}


void *writer_process(int arg) {
    while (1) {
        sem_wait(priority_mutex);
        phm->num_writers_waiting++;
        while (phm->num_readers > 0 || phm->write_count > 0) {
            sem_post(priority_mutex);
            sem_wait(write_priority);
            sem_wait(priority_mutex);
        }
        phm->num_writers_waiting--;
        phm->write_count++;
        sem_post(priority_mutex);

        sem_wait(write_mutex);

        // sem_wait(priority_mutex);
        // shm->num_writers_waiting--;
        // shm->write_count++;
        // sem_post(priority_mutex);

        // Запись
        printf("[WRITE] -> Писатель %d пишет в библиотеке\n", arg);
        usleep(rand() % 3000000);
        printf("[WRITE] -> Писатель %d закончил писать в библиотеке\n", arg);
        sem_post(write_mutex);

        sem_wait(priority_mutex);
        phm->write_count--;
        if (phm->write_count == 0 && phm->num_writers_waiting > 0 && phm->read_count >= PATIENCE_THRESHOLD_READ) {
            sem_post(write_priority);
        } else if (phm->write_count == 0) {
            for (int i = 0; i < PATIENCE_THRESHOLD_WRITE; i++) { // Разрешаем максимум трём читателям
                sem_post(read_priority);
            }
        }
        sem_post(priority_mutex);

        usleep(rand() % 3000000);
    }
}


void start_process() {
    // Создание разделяемой памяти
    int shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    ftruncate(shm_fd, sizeof(struct SharedMemory));
    shm = mmap(0, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Инициализация семафоров
    write_mutex = sem_open("/write_mutex", O_CREAT, 0644, 1);
    read_mutex = sem_open("/read_mutex", O_CREAT, 0644, 1);
    priority_mutex = sem_open("/priority_mutex", O_CREAT, 0644, 1);
    write_priority = sem_open("/write_priority", O_CREAT, 0644, 0);
    read_priority = sem_open("/read_priority", O_CREAT, 0644, 0);

    // Инициализация разделяемой памяти
    shm->num_readers = 0;
    shm->num_writers_waiting = 0;
    shm->read_count = 0;
    shm->write_count = 0;
    // shm->patience_threshold_read = 3;
    // shm->patience_threshold_write = 5;

    pid_t pid;
    char queues[] = "wwrrwwrrrrrrrrrrwwwwrrrrwwwwrrrrwwwwrrrrwwwwrrrrwww";
    int queues_length = strlen(queues);

    for (int i = 0; i < queues_length; i++) {
        pid = fork();
        phm = mmap(0, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (pid > 0) { // Child process
            if (queues[i] == 'r') {
                reader_process(i);
            } else {
                writer_process(i);
            }
            exit(0); // Завершение дочернего процесса
        }
    }

    for (int i = 0; i < queues_length; i++) {
        wait(NULL); // Ожидание завершения всех дочерних процессов
    }

    // Очистка
    sem_close(write_mutex);
    sem_close(read_mutex);
    sem_close(priority_mutex);
    sem_close(write_priority);
    sem_close(read_priority);
    sem_unlink("/write_mutex");
    sem_unlink("/read_mutex");
    sem_unlink("/priority_mutex");
    sem_unlink("/write_priority");
    sem_unlink("/read_priority");
    munmap(shm, sizeof(struct SharedMemory));
    shm_unlink("/my_shm");
}