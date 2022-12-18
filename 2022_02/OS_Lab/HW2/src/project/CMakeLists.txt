cmake_minimum_required(VERSION 3.22)
project(project C)

set(CMAKE_C_STANDARD 99)

add_executable(project main.c common.c common.h consumer.c consumer.h producer.c producer.h)
