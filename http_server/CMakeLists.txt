add_executable(HttpServerApp
    main.cpp
    http_server.cpp
    http_connection.cpp
)

target_include_directories(HttpServerApp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(
    ${BOOST_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBPQXX_INCLUDE_DIRS}
)

target_link_libraries(HttpServerApp PRIVATE 
    CommonLib
    DatabaseLib
    ${OPENSSL_LIBRARIES}
    ${LIBPQXX_LIBRARIES}
)

 
if(APPLE)
    target_include_directories(HttpServerApp PRIVATE 
        ${HOMEBREW_PREFIX}/opt/boost/include
        ${HOMEBREW_PREFIX}/opt/openssl/include
    )
    target_link_directories(HttpServerApp PRIVATE 
        ${HOMEBREW_PREFIX}/opt/boost/lib
        ${HOMEBREW_PREFIX}/opt/openssl/lib
    )
endif()

configure_file(${CMAKE_SOURCE_DIR}/config.ini ${CMAKE_CURRENT_BINARY_DIR}/config.ini COPYONLY)