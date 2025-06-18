add_library(HtmlParserLib
    html_parser.cpp
)

target_include_directories(HtmlParserLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HtmlParserLib PRIVATE 
    Boost::regex 
    Boost::locale
)

# Для macOS
if(APPLE)
    target_include_directories(HtmlParserLib PRIVATE ${HOMEBREW_PREFIX}/opt/boost/include)
    target_link_directories(HtmlParserLib PRIVATE ${HOMEBREW_PREFIX}/opt/boost/lib)
endif()