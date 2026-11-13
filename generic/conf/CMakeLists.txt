add_library(generic_conf STATIC
    src/conf.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(generic_conf PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(generic_conf PUBLIC
    pico_stdlib
    generic_pin
)
