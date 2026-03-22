/*Coils — команды
    Input Registers — всё, что диспетчер читает
    Holding Registers — если потом понадобятся настройки
    Discrete Inputs пока можно вообще не использовать

Команды — Coils
   Блок команд
00001 — Пожар ВКЛ
00002 — Пожар ВЫКЛ
00003 — Стоп тест
00004 — Старт функционального теста
00005 — Старт теста на длительность
00006..00016 — резерв
     Команды делаем импульсным диспетчер записал 1 → шкаф выполнил → сам сбросил в 0.
Общий статус шкафа — Input Registers
        Блок 30001...
30001 — Шкаф включен
    0 = нет
    1 = да
30002 — Состояние шкафа
    0 = OK
    1 = авария
    2 = функциональный тест
    3 = тест на длительность
    4 = неисправность батареи
    5 = нет заряда
30003 — Количество линий
30004 — Состояние батареи
    0 = норма
    1 = неисправность
    2 = нет заряда
30005 30008— Резерв

 5. Измерения — Input Registers
     передавать не float, а целые числа с масштабом.
   напряжение ×10
     ток ×100
    мощность ×10
    частота ×100
    температура ×10
        Блок измерений

30009 — Входное напряжение ×10
30010 — Входная мощность ×10
30011 — Входной ток ×100
30012 — Частота ×100
30013 — Температура ×10
30014 — Резерв
30015 — Резерв
30016 — Резерв

 6. Время

 Системное время и даты храним как Unix time 32-bit, то есть по 2 регистра на одно время.
    Системное время
30017 — System time low word
30018 — System time high word
            Дата последнего длительного теста шкафа
30019 — Last duration test low word
30020 — Last duration test high word
30021..30024 — резерв

    7. Линии
  даём каждой линии одинаковый блок.
    по 6 регистров на линию.
               База:
                      линия 1 начинается с 30101
                      линия 2 с 30107
                      линия 3 с 30113
                      и так далее
     Формула:
base = 30101 + (номер_линии - 1) * 6

      Что хранить по линии
base + 0 — тип линии
    0 = не используется
    1 = постоянного действия
    2 = непостоянного действия

base + 1 — состояние линии
      0 = OK
    1 = авария
    2 = тест
    3 = отключена / не используется
base + 2 — дата последнего теста, low word
base + 3 — дата последнего теста, high word
base + 4 — резерв
base + 5 — резерв
*/

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
    InRegReserved06 = 5,
    InRegReserved07 = 6,
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
    LineLastTestLow = 2,
    LineLastTestHigh = 3,
    LineReserved1 = 4,
    LineReserved2 = 5
};

enum CabinetStateCode : quint16
{
    CabinetOk = 0,
    CabinetEmergency = 1,       // FIRE или STOP
    CabinetFunctionalTest = 2,
    CabinetDurationTest = 3,
    CabinetBatteryFault = 4,
    CabinetBatteryNoCharge = 5,
    CabinetLineAlarm = 6
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

int lineBase(int lineIndex);

quint16 readInputRegister(BackendController *backend, int address);
bool writeCoil(BackendController *backend, int address, bool value);

quint16 encodeEmergencyState(BackendController *backend);
quint16 encodeCabinetState(BackendController *backend);
quint16 encodeBatteryState(BackendController *backend);
quint16 encodeLineType(Line *line);
quint16 encodeLineState(Line *line);

quint16 toScaled10(double v);
quint16 toScaled100(double v);
}