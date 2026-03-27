file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/example/*.cc)

# 添加可执行文件
add_executable(echo ${SRCS})

# 链接主库
target_link_libraries(echo PRIVATE LouidNetLib)

# 设置包含路径
target_include_directories(echo PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(echo PRIVATE ${PROJECT_SOURCE_DIR}/example)

