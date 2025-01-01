cmake_minimum_required(VERSION 3.22)
project(ca_hw1 C)

set(CMAKE_C_STANDARD 99)

add_executable(ca_hw1 main.c simulator.c simulator.h
        syntax.c syntax.h io.c io.h context.c context.h)
