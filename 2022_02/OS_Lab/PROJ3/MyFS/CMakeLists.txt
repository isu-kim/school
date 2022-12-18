cmake_minimum_required(VERSION 3.22)
project(MyFS C)

set(CMAKE_C_STANDARD 99)

add_executable(MyFS main.c common.h diskutil.c diskutil.h fs.h utils.c utils.h disktree.h disktree.c ui.h ui.c)
