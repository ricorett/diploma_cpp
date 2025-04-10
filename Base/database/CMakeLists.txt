cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0144 NEW)
set(CMAKE_CXX_STANDARD 23)


add_library(DatabaseLib STATIC
        database.cpp
        database.h
)
find_package(fmt REQUIRED)
# Настройка зависимостей
target_include_directories(DatabaseLib PUBLIC
        ${PostgreSQL_INCLUDE_DIRS}  # Передается из корневого CMakeLists.txt
        ${Boost_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
)

target_link_libraries(DatabaseLib PRIVATE
        pqxx
        fmt::fmt
        Boost::program_options
)

if(IS_LINUX OR IS_MAC)
    target_compile_options(DatabaseLib PRIVATE
            -Wall -Wextra -Wpedantic -Wno-unknown-attributes
    )
endif()