cmake_minimum_required(VERSION 3.27)
project(NetworkOperatingSystems C)

set(CMAKE_C_STANDARD 11)

add_executable(NetworkOperatingSystems main.c
        Lab1/model_thread.h
        Lab1/model_thread.c
        Lab2/model_process.h
        Lab2/model_process.c
)
