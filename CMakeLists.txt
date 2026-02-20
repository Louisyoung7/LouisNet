cmake_minimum_required(VERSION 3.14)
project(MyWebServer LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# 收集所有源文件
file(GLOB_RECURSE SOURCES src/*.cc)

# 创建静态库
add_library(MyWebServer STATIC ${SOURCES})

# 设置包含路径
target_include_directories(MyWebServer PUBLIC ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(example)

# 测试相关配置
option(BUILD_TESTS "Build tests" OFF)

enable_testing()

if(BUILD_TESTS)
  # 添加测试子目录
  add_subdirectory(tests)
endif()