# Generic folder: groups the generic modules.
# To add a module: create a subfolder and add it below.

add_subdirectory(cmd)
add_subdirectory(trigger)
add_subdirectory(pin)
add_subdirectory(conf)
add_subdirectory(protocol)

# Aggregate library, linked by the other parts of the project
add_library(generic INTERFACE)

target_link_libraries(generic INTERFACE
    generic_cmd
    generic_trigger
    generic_pin
    generic_conf
    generic_protocol
)
