cmake_minimum_required(VERSION 3.28)
project(Thread_Pool)

set(CMAKE_CXX_STANDARD 20)

add_executable(pool
        TaskQueue/TaskQueue.h
        TaskQueue/TaskQueue.cpp
        ThreadPool/ThreadPool.h
        ThreadPool/ThreadPool.cpp
        main.cpp)
