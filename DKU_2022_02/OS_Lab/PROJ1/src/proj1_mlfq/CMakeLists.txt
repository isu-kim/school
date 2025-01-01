cmake_minimum_required(VERSION 3.22)
project(mlfq C)

set(CMAKE_C_STANDARD 99)

add_executable(mlfq main.c common.h process.h process.c utils.h utils.c
        child.c child.h parent.c parent.h)
