cmake_minimum_required(VERSION 3.13)

# Target board: Raspberry Pi Pico 2 (RP2350)
set(PICO_BOARD pico2 CACHE STRING "Board type")
set(PICO_PLATFORM rp2350 CACHE STRING "Platform")

include(pico_sdk_import.cmake)

project(PicoDebugProbe VERSION 0.1.0 LANGUAGES C CXX ASM)

# Hardware revision of the probe board, reported by the system info command
set(BOARD_VERSION_MAJOR 1)
set(BOARD_VERSION_MINOR 0)
set(BOARD_VERSION_PATCH 0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Modules
add_subdirectory(generic)
add_subdirectory(application)

# Layering check: application may use generic, but generic must never
# depend on application. Fails the configuration if it does.
foreach(lib IN ITEMS generic generic_cmd generic_trigger generic_protocol
                     generic_conf)
    get_target_property(lib_deps ${lib} LINK_LIBRARIES)
    if(lib_deps)
        foreach(dep IN LISTS lib_deps)
            if(dep MATCHES "^application")
                message(FATAL_ERROR
                    "Layering violation: ${lib} depends on ${dep}. "
                    "generic modules must not depend on application modules.")
            endif()
        endforeach()
    endif()
endforeach()

# Main executable
add_executable(PicoDebugProbe
    main.c
)

target_link_libraries(PicoDebugProbe
    application
    generic
    pico_stdlib
)

# USB/UART outputs
# Binary frames: stdio must not translate \n into \r\n on USB
target_compile_definitions(PicoDebugProbe PRIVATE PICO_STDIO_USB_DEFAULT_CRLF=0)
pico_enable_stdio_usb(PicoDebugProbe 1)
pico_enable_stdio_uart(PicoDebugProbe 0)

pico_add_extra_outputs(PicoDebugProbe)
