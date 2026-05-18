file(GLOB_RECURSE SERVER_SRCS ${PROJECT_SOURCE_DIR}/example/echo/server/*.cc)
file(GLOB_RECURSE CLIENT_SRCS ${PROJECT_SOURCE_DIR}/example/echo/client/*.cc)

add_executable(echo_server ${SERVER_SRCS})
add_executable(echo_client ${CLIENT_SRCS})

target_link_libraries(echo_server PRIVATE LouisNetLib spdlog::spdlog pthread)
target_link_libraries(echo_client PRIVATE LouisNetLib spdlog::spdlog pthread)


target_include_directories(echo_server PRIVATE 
    ${PROJECT_SOURCE_DIR}/src 
    ${PROJECT_SOURCE_DIR}/example/echo/server)
    
target_include_directories(echo_client PRIVATE 
    ${PROJECT_SOURCE_DIR}/src 
    ${PROJECT_SOURCE_DIR}/example/echo/client)
