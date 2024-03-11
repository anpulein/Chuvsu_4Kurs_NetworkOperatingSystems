cmake_minimum_required(VERSION 3.28)
project(ClientServerApp)

set(CMAKE_CXX_STANDARD 17)

add_executable(ClientServerApp src/client_reader.c
        src/server.c
        src/client_writer.c
        src/server.h
)
