cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(pico_ir_keyboard C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_subdirectory(nec_receive_library)

add_executable(pico_ir_keyboard)

target_sources(pico_ir_keyboard PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
        )

target_include_directories(pico_ir_keyboard PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

target_link_libraries(pico_ir_keyboard LINK_PUBLIC
        pico_stdlib
        hardware_pio
        nec_receive_library
        )

pico_add_extra_outputs(pico_ir_keyboard)

family_configure_device_example(pico_ir_keyboard noos)
