cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)

add_executable(SpiderApp
	main.cpp
	http_utils.h
	http_utils.cpp
	link.h
		../database/database.cpp
		../database/database.h
	html_parser.cpp
	html_parser.h
		../config/config_utils.cpp
		../config/config_utils.h
)

target_compile_features(SpiderApp PRIVATE cxx_std_17) 


set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl)
set(CURL_ROOT /opt/homebrew/opt/curl)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread locale)
find_package(Boost 1.81.0 REQUIRED COMPONENTS url)
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Gumbo REQUIRED gumbo)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)
target_include_directories(SpiderApp PRIVATE
		${Boost_INCLUDE_DIRS}
		${CURL_INCLUDE_DIRS})

target_include_directories(SpiderApp PUBLIC
		${PostgreSQL_INCLUDE_DIRS}
		${Gumbo_INCLUDE_DIRS})

target_link_libraries(SpiderApp
		PRIVATE
		OpenSSL::SSL
		OpenSSL::Crypto
		Boost::system
		Boost::thread
		Boost::locale
		Boost::url
		pqxx
		gumbo
		PostgreSQL::PostgreSQL
		ConfigUtils
		${CURL_LIBRARIES}
)

if(IS_LINUX OR IS_MAC)
	target_compile_options(SpiderApp PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()