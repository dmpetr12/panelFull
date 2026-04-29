cmake_minimum_required(VERSION 3.16)

project(EmergencyLighting LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(PANEL_ENABLE_OPCUA_SERVER "Build embedded OPC UA server using open62541" ON)
set(PANEL_QTOPCUA_SOURCE_DIR "C:/Qt/6.10.3/Src/qtopcua" CACHE PATH "Path to Qt OPC UA sources")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    HttpServer
    SerialPort
    SerialBus
)

qt_standard_project_setup()

add_subdirectory(core)
add_subdirectory(hmi)
add_subdirectory(service)
add_subdirectory(web)
