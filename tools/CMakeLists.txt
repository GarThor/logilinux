cmake_minimum_required(VERSION 3.10)

# Tool executables for command-line interface

# Device discovery tool
add_executable(logilinux-devices logilinux-devices.cpp)
target_link_libraries(logilinux-devices PRIVATE logilinux)

# Dialpad tools
add_executable(dialpad-monitor dialpad-monitor.cpp)
target_link_libraries(dialpad-monitor PRIVATE logilinux)

add_executable(dialpad-grab dialpad-grab.cpp)
target_link_libraries(dialpad-grab PRIVATE logilinux)

# MX Keypad tools
add_executable(keypad-monitor keypad-monitor.cpp)
target_link_libraries(keypad-monitor PRIVATE logilinux)

add_executable(keypad-set-image keypad-set-image.cpp)
target_link_libraries(keypad-set-image PRIVATE logilinux)

add_executable(keypad-set-color keypad-set-color.cpp)
target_link_libraries(keypad-set-color PRIVATE logilinux)

add_executable(keypad-set-gif keypad-set-gif.cpp)
target_link_libraries(keypad-set-gif PRIVATE logilinux)

# Install tools
install(TARGETS 
    logilinux-devices
    dialpad-monitor
    dialpad-grab
    keypad-monitor
    keypad-set-image
    keypad-set-color
    keypad-set-gif
    RUNTIME DESTINATION bin
)
