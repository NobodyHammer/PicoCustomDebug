add_library(application_swd STATIC
    src/swd.c
)

target_include_directories(application_swd PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(application_swd
    PUBLIC pico_stdlib
    PRIVATE application_dispatcher generic_cmd generic_conf generic_trigger
)
