cmake_minimum_required(VERSION 3.28)
project(ClientServerApp)

set(CMAKE_CXX_STANDARD 17)

add_executable(ClientServerApp src/client.c
        src/server.c
        src/server.h
)
