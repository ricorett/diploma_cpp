cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0144 NEW)

set(CMAKE_CXX_STANDARD 23)

add_executable(HttpServerApp
		main.cpp
		http_connection.h
		http_connection.cpp
		../spider/html_parser.cpp
)

target_compile_features(HttpServerApp PRIVATE cxx_std_23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)
message(STATUS "LIBPQXX_LIBRARIES: ${LIBPQXX_LIBRARIES}")
message(STATUS "LIBPQXX_LIBRARY_DIRS: ${LIBPQXX_LIBRARY_DIRS}")

include_directories(
		/opt/homebrew/include
		/opt/homebrew/Cellar/gumbo-parser/0.13.0/include
)

link_directories(
		/opt/homebrew/lib
		/opt/homebrew/Cellar/gumbo-parser/0.13.0/lib
)

set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl)
find_package(OpenSSL REQUIRED)

set(BOOST_ROOT /opt/homebrew/opt/boost)
find_package(Boost REQUIRED COMPONENTS system thread locale)

find_package(PostgreSQL REQUIRED)

target_link_libraries(HttpServerApp
		PRIVATE
		DatabaseLib
		${Boost_LIBRARIES}
		OpenSSL::SSL
		OpenSSL::Crypto
		gumbo
		PostgreSQL::PostgreSQL
		${LIBPQXX_LIBRARIES}
)

if(IS_LINUX OR IS_MAC)
	target_compile_options(HttpServerApp PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()