add_library(application_system STATIC
    src/system.c
)

# Only the include/ folder is public; src/ stays private to this module
target_include_directories(application_system PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Versions reported by the info command, taken from the top-level CMakeLists.txt
target_compile_definitions(application_system PRIVATE
    FW_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    FW_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    FW_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    BOARD_VERSION_MAJOR=${BOARD_VERSION_MAJOR}
    BOARD_VERSION_MINOR=${BOARD_VERSION_MINOR}
    BOARD_VERSION_PATCH=${BOARD_VERSION_PATCH}
)

target_link_libraries(application_system PRIVATE
    application_dispatcher
    generic_cmd
)
