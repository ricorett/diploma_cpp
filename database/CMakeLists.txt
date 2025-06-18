add_library(DatabaseLib
    database.cpp
)

target_include_directories(DatabaseLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
include_directories(
    ${LIBPQXX_INCLUDE_DIRS}
)

target_link_libraries(DatabaseLib PRIVATE 
    CommonLib
)

# Для macOS
if(APPLE)
    target_include_directories(DatabaseLib PRIVATE ${HOMEBREW_PREFIX}/opt/boost/include)
    target_link_directories(DatabaseLib PRIVATE ${HOMEBREW_PREFIX}/opt/boost/lib)
endif()