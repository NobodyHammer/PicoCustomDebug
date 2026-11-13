add_library(application_config STATIC
    src/config.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(application_config PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(application_config
    PRIVATE application_dispatcher generic_conf generic_cmd
)
