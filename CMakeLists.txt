cmake_minimum_required(VERSION 3.5)

project(proxy-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(proxy-server main.cpp proxyserver.h proxyserver.cpp unp.h)
