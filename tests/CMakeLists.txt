# 查找Google Test
find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_SOURCES test_buffer.cc test_inet_address.cc test_channel.cc test_poller.cc)

# 添加可执行目标
add_executable(tests ${TEST_SOURCES})

# 链接Google Test和必要的库
target_link_libraries(tests ${GTEST_BOTH_LIBRARIES} pthread MyWebServer)

# 包含头文件目录
target_include_directories(tests PRIVATE "${PROJECT_SOURCE_DIR}/src")

# 添加测试
include(GoogleTest)
gtest_discover_tests(tests)