cmake_minimum_required(VERSION 3.5)

project(proxy-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# std::thread dependencies
#find_package(Threads REQUIRED)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread")

# MySql dependencies
set(FULL_PATH_TO_MYSQL_CONNECTOR_CPP_DIR your_path_to_mysql_connectorcpp)
include_directories(${FULL_PATH_TO_MYSQL_CONNECTOR_CPP_DIR}/include)
link_directories(${FULL_PATH_TO_MYSQL_CONNECTOR_CPP_DIR}/lib)

#find_package(X11 REQUIRED)
#link_libraries(${X11_LIBRARIES})
#include_directories(${X11_INCLUDE_DIR})

add_executable(proxy-server main.cpp proxyserver.h proxyserver.cpp urls.h urls.cpp)
