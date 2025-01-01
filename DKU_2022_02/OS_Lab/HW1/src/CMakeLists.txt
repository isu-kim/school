cmake_minimum_required(VERSION 3.22)
project(sish C)

set(CMAKE_C_STANDARD 99)

add_executable(sish main.c utils.h utils.c sish.h sish.c)
