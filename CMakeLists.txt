cmake_minimum_required(VERSION 3.14)
project(MyWebServer LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# 使用 file(GLOB ...) 收集匹配模式的源文件
file(GLOB_RECURSE REACTOR_SRCS "${PROJECT_SOURCE_DIR}/src/reactor/*.cc")
file(GLOB_RECURSE SERVER_SRCS "${PROJECT_SOURCE_DIR}/src/server/*.cc")
file(GLOB_RECURSE SOCKET_SRCS "${PROJECT_SOURCE_DIR}/src/socket/*.cc")

# 合并所有源文件
set(ALL_SRCS ${REACTOR_SRCS} ${SERVER_SRCS} ${SOCKET_SRCS})

add_executable(MyWebServer ${ALL_SRCS})

target_include_directories(MyWebServer PRIVATE "${PROJECT_SOURCE_DIR}/src")