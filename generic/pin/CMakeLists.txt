add_library(generic_pin STATIC
    src/pin.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(generic_pin PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(generic_pin PUBLIC
    pico_stdlib
)
