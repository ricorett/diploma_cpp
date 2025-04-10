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
)

target_compile_features(SpiderApp PRIVATE cxx_std_17) 

#target_link_libraries(SpiderApp pqxx)

set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl)
find_package(OpenSSL REQUIRED)

#target_link_libraries(SpiderApp ${Boost_LIBRARIES})

target_include_directories(SpiderApp PRIVATE ${Boost_INCLUDE_DIRS})
find_package(PkgConfig REQUIRED)
pkg_check_modules(Gumbo REQUIRED gumbo)

target_include_directories(SpiderApp PRIVATE ${Gumbo_INCLUDE_DIRS})
#target_link_libraries(SpiderApp ${Gumbo_LIBRARIES})

target_link_libraries(SpiderApp
		PRIVATE
		OpenSSL::SSL
		OpenSSL::Crypto
		Boost::system
		Boost::thread
		Boost::locale
		pqxx
		gumbo
		PostgreSQL::PostgreSQL
)

if(IS_LINUX OR IS_MAC)
	target_compile_options(SpiderApp PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()