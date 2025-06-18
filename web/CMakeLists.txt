file(GLOB WEB_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.html"
)

foreach(file IN LISTS WEB_FILES)
    configure_file(${file} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach()


add_custom_target(WebFiles ALL DEPENDS ${WEB_FILES})

set(WEB_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)