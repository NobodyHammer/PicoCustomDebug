add_library(generic_protocol STATIC
    src/protocol.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(generic_protocol PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# The public API uses the cmd types
target_link_libraries(generic_protocol PUBLIC
    generic_cmd
)
