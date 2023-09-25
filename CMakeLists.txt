cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
# set the project name (change to suit)
project(mag_counter)

pico_sdk_init()

# add_subdirectory()

# rest of your project
add_executable(${CMAKE_PROJECT_NAME}
        main.cpp
        Vfd12.cpp L6219.cpp Keypad.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
        )

target_link_libraries(${CMAKE_PROJECT_NAME} 
        pico_stdlib
        hardware_i2c
        )

# adjust to enable stdio via usb, or uart
pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})
