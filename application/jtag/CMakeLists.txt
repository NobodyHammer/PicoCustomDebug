add_library(application_jtag STATIC
    src/jtag.c
)

target_include_directories(application_jtag PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(application_jtag
    PUBLIC pico_stdlib
    PRIVATE application_dispatcher generic_cmd generic_conf generic_trigger
)
