cmake_minimum_required(VERSION 3.15)
project(ZBackup)
set(CMAKE_CXX_STANDARD 17)

# 添加子目录，先编译ZHttpServer
add_subdirectory(ZHttpServer)

# nlohmann_json头文件（假设用的是单头文件版）
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann) # 如有需要

# 查找依赖库
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# 递归收集server和client源码（支持.cpp和.cc）
file(GLOB_RECURSE SERVER_SRC
    ${CMAKE_SOURCE_DIR}/server/*.cc
)
file(GLOB_RECURSE CLIENT_SRC
    ${CMAKE_SOURCE_DIR}/client/*.cc
)

# server可执行文件
add_executable(backup_server ${SERVER_SRC})
target_include_directories(backup_server PRIVATE
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}/ZHttpServer/include
)

target_link_libraries(backup_server
    zhttpserver
    fmt::fmt
    nlohmann_json::nlohmann_json
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    snappy
)

# client可执行文件
add_executable(backup_client ${CLIENT_SRC})
target_include_directories(backup_client PRIVATE
    ${CMAKE_SOURCE_DIR}/client
)
target_link_libraries(backup_client
    fmt::fmt
    nlohmann_json::nlohmann_json
    Threads::Threads
    snappy
)

