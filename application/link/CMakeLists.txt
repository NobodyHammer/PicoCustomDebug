add_library(application_link STATIC
    src/link.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(application_link PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(application_link
    PUBLIC pico_stdlib
    PRIVATE generic_protocol
)
