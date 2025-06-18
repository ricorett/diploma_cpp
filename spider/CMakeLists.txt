add_executable(SpiderApp
    main.cpp
    spider.cpp
    http_utils.cpp
)

target_include_directories(SpiderApp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(
    ${BOOST_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBPQXX_INCLUDE_DIRS}
)

target_link_libraries(SpiderApp PRIVATE 
    CommonLib
    DatabaseLib
    HtmlParserLib
    ${BOOST_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBPQXX_LIBRARIES}
)

# Для macOS
if(APPLE)
    target_include_directories(SpiderApp PRIVATE 
        ${HOMEBREW_PREFIX}/opt/boost/include
        ${HOMEBREW_PREFIX}/opt/openssl/include
    )
    target_link_directories(SpiderApp PRIVATE 
        ${HOMEBREW_PREFIX}/opt/boost/lib
        ${HOMEBREW_PREFIX}/opt/openssl/lib
    )
endif()

configure_file(${CMAKE_SOURCE_DIR}/config.ini ${CMAKE_CURRENT_BINARY_DIR}/config.ini COPYONLY)