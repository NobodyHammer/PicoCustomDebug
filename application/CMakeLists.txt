# Application folder: groups the project-specific modules.
# To add a module: create a subfolder and add it below.

add_subdirectory(dispatcher)
add_subdirectory(system)
add_subdirectory(link)
add_subdirectory(config)
add_subdirectory(swd)
add_subdirectory(jtag)
add_subdirectory(gpio)
add_subdirectory(timer)

# Aggregate library, linked by the main program
add_library(application INTERFACE)

target_link_libraries(application INTERFACE
    application_dispatcher
    application_system
    application_link
    application_config
    application_swd
    application_jtag
    application_gpio
    application_timer
)
