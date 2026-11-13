add_library(application_dispatcher STATIC
    src/dispatcher.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(application_dispatcher PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# The public API uses the cmd types
target_link_libraries(application_dispatcher PUBLIC
    generic_cmd
)
