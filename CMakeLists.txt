cmake_minimum_required(VERSION 3.12)

include(lib/pico-sdk/pico_sdk_init.cmake)

project(radio_module_v2)

pico_sdk_init()

add_subdirectory(lib/RadioLib)
add_subdirectory(lib/datalink-c)

add_executable(radio_module src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/lib/RadioLib/src/hal/RPiPico/PicoHal.cpp)
target_link_libraries(radio_module pico_stdlib hardware_spi hardware_gpio hardware_timer pico_multicore hardware_pwm RadioLib datalink)
target_compile_definitions(radio_module PRIVATE RADIOLIB_BUILD_RPI_PICO)

pico_enable_stdio_usb(radio_module 1)
pico_enable_stdio_uart(radio_module 0)
pico_add_extra_outputs(radio_module)