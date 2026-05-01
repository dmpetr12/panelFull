#pragma once

#include <QtGlobal>

class BackendController;
class Line;

namespace ModbusMap
{
enum CoilAddress : int
{
    CoilFireOn = 0,
    CoilFireOff = 1,
    CoilStopOn = 2,
    CoilStopOff = 3,
    CoilStopTest = 4,
    CoilStartFunctionalTest = 5,
    CoilStartDurationTest = 6,

    CoilCount = 16
};

enum InputRegAddress : int
{
    // 30001...
    InRegCabinetEnabled = 0,
    InRegCabinetState = 1,
    InRegLinesCount = 2,
    InRegBatteryState = 3,
    InRegEmergencyState = 4,     // 30005
    InRegDoorState = 5,
    InRegSystemState = 6,        // 30007
    InRegReserved08 = 7,

    // измерения 30009...
    InRegInputVoltage = 8,
    InRegInputPower = 9,
    InRegInputCurrent = 10,
    InRegInputFrequency = 11,
    InRegTemperature = 12,
    InRegReserved14 = 13,
    InRegReserved15 = 14,
    InRegReserved16 = 15,

    // время 30017...
    InRegSystemTimeLow = 16,
    InRegSystemTimeHigh = 17,
    InRegLastDurationTestLow = 18,
    InRegLastDurationTestHigh = 19,
    InRegReserved21 = 20,
    InRegReserved22 = 21,
    InRegReserved23 = 22,
    InRegReserved24 = 23,

    // линии 30101...
    InRegLinesBase = 100,
    LineBlockSize = 6,

    InputRegisterCount = 400
};

enum LineBlockOffset : int
{
    LineType = 0,
    LineState = 1,
    LineOutputState = 2,
    LineLastTestLow = 3,
    LineLastTestHigh = 4,
    LineReserved1 = 5
};

enum CabinetStateCode : quint16
{
    CabinetNormal = 0,
    CabinetFire = 1,
    CabinetTest = 2,
    CabinetAlarm = 3, // legacy / reserved
    CabinetBatteryFault = 4,
    CabinetBatteryNoCharge = 5
};

enum SystemStateCode : quint16
{
    SystemOk = 0,
    SystemAlarm = 1
};

enum BatteryStateCode : quint16
{
    BatteryNormal = 0,
    BatteryFault = 1,
    BatteryNoCharge = 2
};

enum LineTypeCode : quint16
{
    LineNotUsed = 0,
    LineConstant = 1,
    LineNonConstant = 2
};

enum LineOutputStateCode : quint16
{
    LineOutputOff = 0,
    LineOutputOn = 1,
    LineOutputUnknown = 2
};

enum LineStateCode : quint16
{
    LineOk = 0,
    LineAlarm = 1,
    LineTest = 2,
    LineUnused = 3
};

enum EmergencyStateCode : quint16
{
    EmergencyNone = 0,
    EmergencyFire = 1,
    EmergencyStop = 2,
    EmergencyFireAndStop = 3
};

enum DoorStateCode : quint16
{
    DoorClosed = 0,
    DoorOpen = 1
};

int lineBase(int lineIndex);

quint16 readInputRegister(BackendController *backend, int address);
bool writeCoil(BackendController *backend, int address, bool value);

quint16 encodeEmergencyState(BackendController *backend);
quint16 encodeCabinetState(BackendController *backend);
quint16 encodeSystemState(BackendController *backend);
quint16 encodeBatteryState(BackendController *backend);
quint16 encodeLineType(Line *line);
quint16 encodeLineState(Line *line);
quint16 encodeLineOutputState(Line *line);

quint16 toScaled10(double v);
quint16 toScaled100(double v);
}
