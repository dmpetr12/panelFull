add_library(core STATIC
    BackendController.cpp
    BackendController.h
    BatteryController.cpp
    BatteryController.h
    AppConfig.cpp AppConfig.h line.h LineIoManager.cpp LineIoManager.h linesmodel.h MaintenanceChecker.h modbusbus.cpp modbusbus.h schedulemanager.cpp schedulemanager.h TestController.cpp TestController.h  ValueProvider.h
    PasswordManager.h
    DeviceSnapshot.h
    scheduletypes.h
    schedulestorage.h
    schedulestorage.cpp
    schedulecalculator.h
    schedulecalculator.cpp
    logger.h
    logger.cpp
    devicesnapshot_json.h
    LocalIpcServer.cpp
    LocalIpcServer.h
    OpcUaServer.cpp
    OpcUaServer.h

    systemObjects.h

    modbus/ModbusRegisterMap.cpp
    modbus/ModbusRegisterMap.h
    modbus/ModbusRtuSlave.cpp
    modbus/ModbusRtuSlave.h
    modbus/ModbusTcpServer.h
    modbus/ModbusTcpServer.cpp
    web/WebApiServer.h
    web/WebApiServer.cpp
)

if (PANEL_ENABLE_OPCUA_SERVER)
    set(PANEL_OPEN62541_DIR "${PANEL_QTOPCUA_SOURCE_DIR}/src/3rdparty/open62541")
    if (EXISTS "${PANEL_OPEN62541_DIR}/open62541.c" AND EXISTS "${PANEL_OPEN62541_DIR}/open62541.h")
        target_sources(core PRIVATE
            "${PANEL_OPEN62541_DIR}/open62541.c"
            "${PANEL_OPEN62541_DIR}/open62541.h"
        )
        target_include_directories(core PRIVATE
            "${PANEL_OPEN62541_DIR}"
        )
        target_compile_definitions(core PUBLIC PANEL_HAS_OPEN62541_SERVER=1)

        if (WIN32)
            target_link_libraries(core PUBLIC iphlpapi)
        endif()
    else()
        message(WARNING "PANEL_ENABLE_OPCUA_SERVER=ON, but open62541 sources were not found in ${PANEL_OPEN62541_DIR}. OPC UA server will be built as stub.")
    endif()
endif()

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../modbus
    ${CMAKE_CURRENT_SOURCE_DIR}/../web
)

target_link_libraries(core PUBLIC
    Qt6::Core
    Qt6::SerialPort
    Qt6::SerialBus
    Qt6::Quick
    Qt6::Gui
    Qt6::HttpServer
    Qt6::Network
)
