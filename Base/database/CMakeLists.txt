cmake_minimum_required(VERSION 3.20)

add_library(Database STATIC database.cpp)

target_include_directories(Database PUBLIC ${Boost_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS})

target_link_libraries(Database pqxx ${PostgreSQL_LIBRARIES})

if(IS_LINUX OR IS_MAC)
    target_compile_options(Database PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()