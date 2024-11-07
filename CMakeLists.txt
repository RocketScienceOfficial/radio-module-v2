cmake_minimum_required(VERSION 3.12)

include(lib/pico-sdk/pico_sdk_init.cmake)

project(radio_module_v2)

pico_sdk_init()

add_subdirectory(lib/RadioLib)

add_executable(radio_module src/main.cpp)
target_link_libraries(radio_module pico_stdlib hardware_spi hardware_gpio hardware_timer pico_multicore hardware_pwm RadioLib)

pico_enable_stdio_usb(radio_module 1)
pico_enable_stdio_uart(radio_module 0)
pico_add_extra_outputs(radio_module)