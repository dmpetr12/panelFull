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

    systemObjects.h

    modbus/ModbusRegisterMap.cpp
    modbus/ModbusRegisterMap.h
    modbus/ModbusRtuSlave.cpp
    modbus/ModbusRtuSlave.h
    modbus/ModbusTcpServer.h
    modbus/ModbusTcpServer.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../modbus
)

target_link_libraries(core PUBLIC
    Qt6::Core
    Qt6::SerialPort
    Qt6::SerialBus
    Qt6::Quick
    Qt6::Gui
)
