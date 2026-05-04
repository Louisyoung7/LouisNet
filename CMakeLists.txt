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

# 包含 FetchContent 模块
include(FetchContent)

# 设置 FetchContent 缓存目录
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/.cache/fetchcontent)

# 声明 spdlog 依赖
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1  
)

# 确保 spdlog 依赖被下载和编译
FetchContent_MakeAvailable(spdlog)

# 根据构建类型设置日志级别
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug 模式：保留所有日志（包括 trace/debug）
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
else()
    # Release/RelWithDebInfo/MinSizeRel 模式：仅保留 info 及以上级别
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif()

# 添加子目录
add_subdirectory(src)
add_subdirectory(example)

# 测试相关配置（默认关闭）
option(BUILD_TESTS "Build tests" OFF)

# Debug 构建时自动启用测试
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_TESTS ON CACHE BOOL "Build tests" FORCE)
endif()

if(BUILD_TESTS)
    enable_testing()
    # 添加测试子目录
    add_subdirectory(tests)
endif()