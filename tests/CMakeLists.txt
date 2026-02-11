# 查找Google Test
find_package(GTest REQUIRED)

# 添加可执行目标
add_executable(test_inet_address test_inet_address.cc)

# 链接Google Test和必要的库
target_link_libraries(test_inet_address ${GTEST_BOTH_LIBRARIES} pthread MyWebServer)

# 包含头文件目录
target_include_directories(test_inet_address PRIVATE "${PROJECT_SOURCE_DIR}/src")

# 添加测试
include(GoogleTest)
gtest_discover_tests(test_inet_address)