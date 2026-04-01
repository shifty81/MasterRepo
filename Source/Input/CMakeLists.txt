add_library(NFInput STATIC
    Devices/Keyboard.cpp
    Devices/Mouse.cpp
    Devices/Gamepad.cpp
    Actions/InputAction.cpp
)

add_library(NF::Input ALIAS NFInput)

target_include_directories(NFInput PUBLIC ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(NFInput PUBLIC NF::Core)

message(STATUS "  NF::Input configured")
