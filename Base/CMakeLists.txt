cmake_minimum_required(VERSION 3.20)
project(CppDiplomProject)
cmake_policy(SET CMP0144 NEW)
cmake_policy(SET CMP0002 NEW)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/opt/homebrew/include) # macOS
link_directories(/opt/homebrew/lib)

# Пути к библиотекам
set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl)
set(BOOST_ROOT /opt/homebrew/opt/boost)

# Подключение зависимостей
find_package(PostgreSQL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread locale program_options)
find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

include_directories(
        ${PostgreSQL_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${LIBPQXX_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
)

# Добавляем подпроекты
add_subdirectory(database)
add_subdirectory(spider)
add_subdirectory(http_server)