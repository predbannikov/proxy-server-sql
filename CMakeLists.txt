cmake_minimum_required(VERSION 3.5)

project(proxy-server LANGUAGES CXX)




set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

#SET(CMAKE_CXX_FLAGS_DEBUG "-O0")
#SET(CMAKE_C_FLAGS_DEBUG "-O0")


# Search OpenSSL
#find_package(PkgConfig REQUIRED)
#pkg_search_module(OPENSSL REQUIRED openssl)

#if( OPENSSL_FOUND )
#	include_directories(${OPENSSL_INCLUDE_DIRS})
#	message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
#else()
    # Error; with REQUIRED, pkg_search_module() will throw an error by it's own
#endif()






# std::thread dependencies
#find_package(Threads REQUIRED)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread")


# MySql dependencies
#set(FULL_PATH_TO_MYSQL_CONNECTOR_CPP_DIR your_path_to_mysql_connectorcpp)
#include_directories(${FULL_PATH_TO_MYSQL_CONNECTOR_CPP_DIR}/include)
#link_directories(${FULL_PATH_TO_MYSQL_CONNECTOR_CPP_DIR}/lib)

#find_package(X11 REQUIRED)
#link_libraries(${X11_LIBRARIES})
#include_directories(${X11_INCLUDE_DIR})

add_executable(proxy-server main.cpp proxyserver.h proxyserver.cpp handshakes.h handshakes.cpp)

target_link_libraries(proxy-server /usr/lib64/libssl.so /usr/lib64/libcrypto.so)
