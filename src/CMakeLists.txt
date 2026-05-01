# 收集所有源文件
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)

# 创建静态库
add_library(LouisNetLib STATIC ${SOURCES})

# 设置包含路径
target_include_directories(LouisNetLib PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(LouisNetLib spdlog::spdlog pthread)
