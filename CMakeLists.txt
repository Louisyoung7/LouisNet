cmake_minimum_required(VERSION 3.21)
project(LouisNet LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

add_subdirectory(src)
add_subdirectory(example)

# 测试相关配置
option(BUILD_TESTS "Build tests" ON)

enable_testing()

if(BUILD_TESTS)
  # 添加测试子目录
  add_subdirectory(tests)
endif()