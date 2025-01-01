cmake_minimum_required(VERSION 3.0)
project(hw2_final C)

set(CMAKE_C_STANDARD 99)

add_executable(hw2_final main.c utils.h utils.c threads.c threads.h)
