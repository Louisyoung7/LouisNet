# 添加可执行文件
add_executable(echo echo.cc)

# 链接主库
target_link_libraries(echo PRIVATE MyWebServer)

# 设置包含路径
target_include_directories(echo PRIVATE ${PROJECT_SOURCE_DIR}/src)