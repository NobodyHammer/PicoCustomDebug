add_library(generic_cmd STATIC
    src/cmd.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(generic_cmd PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(generic_cmd PUBLIC
    pico_stdlib
)
