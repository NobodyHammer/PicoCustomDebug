add_library(application_timer STATIC
    src/timer.c
)

target_include_directories(application_timer PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(application_timer
    PUBLIC pico_stdlib
    PRIVATE application_dispatcher generic_cmd generic_trigger
)
