cmake_minimum_required(VERSION 3.21)
project(LouisNet LANGUAGES CXX)

# 生成 compile_commands.json 文件供 clangd 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# 添加子目录
add_subdirectory(src)
add_subdirectory(example)

# 测试相关配置（默认关闭）
option(BUILD_TESTS "Build tests" ON)

enable_testing()

if(BUILD_TESTS)
  # 添加测试子目录
  add_subdirectory(tests)
endif()