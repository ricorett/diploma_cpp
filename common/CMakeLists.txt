add_library(CommonLib
    ini_parser.cpp
)

target_include_directories(CommonLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Для macOS добавим пути к Boost
if(APPLE)
    target_include_directories(CommonLib PRIVATE ${HOMEBREW_PREFIX}/opt/boost/include)
endif()