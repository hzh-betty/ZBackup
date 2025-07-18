cmake_minimum_required(VERSION 3.15)
project(ZBackup)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")

# 添加子目录，先编译ZHttpServer
add_subdirectory(ZHttpServer)

find_package(nlohmann_json 3.2.0 REQUIRED)

# 查找snappy库
find_library(SNAPPY_LIB snappy REQUIRED)

# 递归收集backup/source目录下的所有.cpp文件
file(GLOB_RECURSE SERVER_SRC
    ${CMAKE_SOURCE_DIR}/backup/source/*.cpp
)

# 添加主程序文件
set(MAIN_SRC ${CMAKE_SOURCE_DIR}/backup/main.cc)


# backup_server可执行文件
add_executable(backup_server ${MAIN_SRC} ${SERVER_SRC})

target_include_directories(backup_server 
    PRIVATE 
    ${PROJECT_SOURCE_DIR}/backup/include
    ${PROJECT_SOURCE_DIR}/ZHttpServer/include
)

target_link_libraries(backup_server
    PRIVATE
    zhttpserver
    ${SNAPPY_LIB}
     nlohmann_json::nlohmann_json
)

