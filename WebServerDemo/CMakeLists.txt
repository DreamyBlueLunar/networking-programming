cmake_minimum_required(VERSION 3.0)
project(WebServerDemo)

set(CMAKE_CXX_STANDARD 20)

add_executable(WebServerDemo main.cpp
        Server.cpp
        Server.h
        ConnItem/ConnItem.cpp
        ConnItem/ConnItem.h
        ConnItem/ConnItemListen.cpp
        ConnItem/ConnItemListen.h
        ConnItem/ConnItemRecv.cpp
        ConnItem/ConnItemRecv.h)
