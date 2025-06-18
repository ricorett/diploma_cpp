cmake_minimum_required(VERSION 3.20)
project(CppDiplomProject)

# Установка политик для совместимости
cmake_policy(SET CMP0144 NEW)  # Для BOOST_ROOT
cmake_policy(SET CMP0167 NEW)  # Для FindBoost
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Определяем путь к Homebrew
execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE HOMEBREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Корректные пути к зависимостям
set(OPENSSL_ROOT_DIR "${HOMEBREW_PREFIX}/opt/openssl@3")
set(BOOST_ROOT "${HOMEBREW_PREFIX}/opt/boost")

# Boost: добавляем недостающие компоненты
find_package(Boost 1.88.0 REQUIRED COMPONENTS 
    system
    filesystem  # Добавлено
    regex
    thread
    locale
    program_options  # Добавлено
    chrono  # Добавлено
    atomic  # Добавлено
)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

# OpenSSL
find_package(OpenSSL REQUIRED)

# Libpqxx через pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

# Сообщаем о найденных зависимостях
message(STATUS "Boost found: ${Boost_VERSION}")
message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
message(STATUS "libpqxx found: ${LIBPQXX_VERSION}")

# Передаем переменные в подпроекты
set(BOOST_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(BOOST_LIBRARIES ${Boost_LIBRARIES})
set(OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
set(OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES})
set(LIBPQXX_INCLUDE_DIRS ${LIBPQXX_INCLUDE_DIRS})
set(LIBPQXX_LIBRARIES ${LIBPQXX_LINK_LIBRARIES})

# Поддиректории
add_subdirectory(common)
add_subdirectory(database)
add_subdirectory(html_parser)
add_subdirectory(spider)
add_subdirectory(http_server)
add_subdirectory(web)