file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/example/http/*.cc)

# 添加可执行文件
add_executable(http_server ${SRCS})

# 链接主库
target_link_libraries(http_server PRIVATE LouisNetLib spdlog::spdlog pthread)

# 设置包含路径
target_include_directories(http_server PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(http_server PRIVATE ${PROJECT_SOURCE_DIR}/example/http)



