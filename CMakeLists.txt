cmake_minimum_required(VERSION 3.14)
project(MyWebServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

add_executable(echo_server example/echo_server.cc)
add_executable(echo_client example/echo_client.cc)