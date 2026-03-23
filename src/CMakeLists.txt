# 收集所有源文件
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)

# 创建静态库
add_library(LouidNetLib STATIC ${SOURCES})

# 设置包含路径
target_include_directories(LouidNetLib PUBLIC ${PROJECT_SOURCE_DIR}/src)