cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)

add_library(ConfigUtils STATIC config_utils.cpp)

target_include_directories(ConfigUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(config.ini ${CMAKE_CURRENT_BINARY_DIR}/config.ini COPYONLY)

if(IS_LINUX OR IS_MAC)
    target_compile_options(ConfigUtils PRIVATE -Wall -Wextra -Wpedantic)
endif()
