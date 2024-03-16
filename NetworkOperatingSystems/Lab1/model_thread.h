//
// Created by Dmitry Galkin on 17.02.2024.
//

#ifndef MODEL_THREAD_H
#define MODEL_THREAD_H

// Поток для читателей
void *reader_thread(void *arg);
// Поток для писателей
void *writer_thread(void *arg);

// Запуск программы
void start_thread();

#endif //MODEL_THREAD_H
