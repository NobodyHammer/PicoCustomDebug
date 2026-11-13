add_library(application_gpio STATIC
    src/gpio.c
)

target_include_directories(application_gpio PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(application_gpio
    PUBLIC pico_stdlib
    PRIVATE application_dispatcher generic_cmd generic_conf generic_trigger
)
