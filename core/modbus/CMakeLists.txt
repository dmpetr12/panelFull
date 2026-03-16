add_library(core STATIC
    BackendController.cpp
    BackendController.h

    BatteryController.cpp
    BatteryController.h

    AppConfig.cpp
    AppConfig.h

    LineIoManager.cpp
    LineIoManager.h

    modbusbus.cpp
    modbusbus.h

    schedulemanager.cpp
    schedulemanager.h

    schedulestorage.cpp
    schedulestorage.h

    schedulecalculator.cpp
    schedulecalculator.h

    TestController.cpp
    TestController.h

    LocalIpcServer.cpp
    LocalIpcServer.h

    logger.cpp
    logger.h

    line.h
    linesmodel.h
    MaintenanceChecker.h
    UsbSerialWatcher.h
    ValueProvider.h
    PasswordManager.h
    DeviceSnapshot.h
    devicesnapshot_json.h
    scheduletypes.h
    TimeAjst.h
    systemObjects.h

    ../modbus/ModbusRegisterMap.cpp
    ../modbus/ModbusRegisterMap.h
    ../modbus/ModbusRtuSlave.cpp
    ../modbus/ModbusRtuSlave.h
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../modbus
)

target_link_libraries(core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::SerialPort
    Qt6::SerialBus
)