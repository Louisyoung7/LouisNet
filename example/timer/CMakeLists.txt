file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/example/timer/*.cc)

# 添加可执行文件
add_executable(timer_example ${SRCS})

# 链接主库
target_link_libraries(timer_example PRIVATE LouisNetLib spdlog::spdlog pthread)

# 设置包含路径
target_include_directories(timer_example PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(timer_example PRIVATE ${PROJECT_SOURCE_DIR}/example/timer)



