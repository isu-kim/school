cmake_minimum_required(VERSION 3.22)
project(Proj1 C)

set(CMAKE_C_STANDARD 99)

add_executable(Proj1 main.c common.h process.h process.c utils.h utils.c
        child.c child.h parent.c parent.h)
