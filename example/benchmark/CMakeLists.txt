file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/example/benchmark/*.cc)

# 添加可执行文件
add_executable(louis_net_benchmark ${SRCS})

# 链接主库
target_link_libraries(louis_net_benchmark PRIVATE LouisNetLib spdlog::spdlog pthread)

# 设置包含路径
target_include_directories(louis_net_benchmark PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(louis_net_benchmark PRIVATE ${PROJECT_SOURCE_DIR}/example/benchmark)



