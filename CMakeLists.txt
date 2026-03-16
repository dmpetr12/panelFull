cmake_minimum_required(VERSION 3.16)

project(EmergencyLighting LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    SerialPort
    SerialBus
)

qt_standard_project_setup()

add_subdirectory(core)
add_subdirectory(hmi)
add_subdirectory(service)
