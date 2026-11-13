add_library(generic_trigger STATIC
    src/trigger.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(generic_trigger PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(generic_trigger PUBLIC
    pico_stdlib
    generic_conf
)
