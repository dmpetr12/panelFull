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
    LineOutputState = 2,
    LineLastTestLow = 3,
    LineLastTestHigh = 4,
    LineReserved1 = 5
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

int lineBase(int lineIndex);

quint16 readInputRegister(BackendController *backend, int address);
bool writeCoil(BackendController *backend, int address, bool value);

quint16 encodeEmergencyState(BackendController *backend);
quint16 encodeCabinetState(BackendController *backend);
quint16 encodeBatteryState(BackendController *backend);
quint16 encodeLineType(Line *line);
quint16 encodeLineState(Line *line);
quint16 encodeLineOutputState(Line *line);

quint16 toScaled10(double v);
quint16 toScaled100(double v);
}

/*
 * Таблица регистров Modbus RTU

Система управления аварийным освещением

1. Команды управления
Адрес	Тип	Название	Формат	Описание
00001	Coil	Fire ON	Bool	Включить программный режим FIRE
00002	Coil	Fire OFF	Bool	Выключить программный режим FIRE
00003	Coil	резерв
00004	Coil	резерв
00005	Coil	Stop Test	Bool	Остановить текущий тест
00006	Coil	Start Functional Test	Bool	Запуск функционального теста
00007	Coil	Start Duration Test	Bool	Запуск теста на длительность
00008–00016	Coil	Reserved	Bool	Резерв

Команды являются импульсными. Шкаф обрабатывает только запись 1.
После выполнения команда автоматически сбрасывается в 0.

2. Общий статус шкафа
Адрес	Тип	Название	Формат	Описание
30001	Input Register	Cabinet Enabled	uint16	0 = выключен, 1 = включен
30002	Input Register	Cabinet State	uint16	Общее состояние шкафа
30003	Input Register	Lines Count	uint16	Количество линий
30004	Input Register	Battery State	uint16	Состояние батареи
30005	Input Register	Emergency State	uint16	Состояние аварийных команд FIRE/STOP
30006	Input Register	Reserved	uint16	Резерв
30007	Input Register	Reserved	uint16	Резерв
30008	Input Register	Reserved	uint16	Резерв
Состояние шкафа (30002)
Код	Значение
0	OK
1	Emergency
2	Функциональный тест
3	Тест на длительность
4	Неисправность батареи
5	Нет заряда батареи
6	Авария по линиям
Аварийное состояние (30005)
Код	Значение
0	Нет аварийной команды
1	FIRE
2	STOP
3	FIRE и STOP одновременно
3. Измерения
Адрес	Тип	Название	Формат	Описание
30009	Input Register	Input Voltage	uint16	Входное напряжение ×10 В
30010	Input Register	Input Power	uint16	Входная мощность ×10 Вт
30011	Input Register	Input Current	uint16	Входной ток ×100 А
30012	Input Register	Input Frequency	uint16	Частота ×100 Гц
30013	Input Register	Temperature	uint16	Температура ×10 °C
30014	Input Register	Reserved	uint16	Резерв
30015	Input Register	Reserved	uint16	Резерв
30016	Input Register	Reserved	uint16	Резерв
4. Время
Адрес	Тип	Название	Формат	Описание
30017	Input Register	System Time Low	uint16	Системное время Unix, младшее слово
30018	Input Register	System Time High	uint16	Системное время Unix, старшее слово
30019	Input Register	Last Duration Test Low	uint16	Время последнего длительного теста, младшее слово
30020	Input Register	Last Duration Test High	uint16	Время последнего длительного теста, старшее слово
30021–30024	Input Register	Reserved	uint16	Резерв

Время передается в формате Unix Time в секундах, как 32-битное значение, разбитое на два регистра.

5. Состояние батареи
Код	Значение
0	Норма
1	Неисправность
2	Нет заряда
6. Данные по линиям

Каждая линия занимает 6 регистров.
Начальный адрес блока линий: 30101

Формула адреса линии:

Адрес = 30101 + (номер линии - 1) × 6

7. Структура блока линии
Смещение	Название	Формат	Описание
+0	Line Type	uint16	Тип линии
+1	Line Status	uint16	Состояние линии
+2	Line State	uint16	Состояние линии
+3	Last Test Time Low	uint16	Время последнего теста, младшее слово
+4	Last Test Time High	uint16	Время последнего теста, старшее слово
+5	Reserved	uint16	Резерв
Тип линии
Код	Значение
0	Линия не используется
1	Постоянного действия
2	Непостоянного действия
Статус линии
Код	Значение
0	OK
1	Авария
2	Тест
3	Не используется
Состояние линии
0	Выкл
1	Вкл
2	Не известно

8. Пример адресов первых линий
Линия 1
Адрес	Название
30101	Тип линии
30102	Состояние линии
30103	Дата последнего теста low
30104	Дата последнего теста high
30105	Резерв
30106	Резерв
Линия 2
Адрес	Название
30107	Тип линии
30108	Состояние линии
30109	Дата последнего теста low
30110	Дата последнего теста high
30111	Резерв
30112	Резерв
Линия 3
Адрес	Название
30113	Тип линии
30114	Состояние линии
30115	Дата последнего теста low
30116	Дата последнего теста high
30117	Резерв
30118	Резерв
9. Резерв

Рекомендуется оставить свободное пространство:

Диапазон	Назначение
00008–00016	Резерв команд
30006–30008	Резерв статуса
30014–30016	Резерв измерений
30021–30024	Резерв времени
30500–30600	Общий резерв системы
Соответствие Modbus регистров и BackendController
1. Команды управления
Адрес	Название	Действие BackendController
00001	Fire ON	setForcedFire(true)
00002	Fire OFF	setForcedFire(false)
00003	Stop ON	setForcedStop(true)
00004	Stop OFF	setForcedStop(false)
00005	Stop Test	stopCurrentTest()
00006	Start Functional Test	startFunctionalTest()
00007	Start Duration Test	startDurationTest()

После выполнения команды бит должен автоматически сбрасываться в 0.

2. Общий статус шкафа
Адрес	Название	Источник данных
30001	Cabinet Enabled	в текущей реализации всегда 1
30002	Cabinet State	encodeCabinetState(backend)
30003	Lines Count	LinesModel::rowCount()
30004	Battery State	encodeBatteryState(backend) / BatteryController
30005	Emergency State	LineIoManager::fireActive() и LineIoManager::stopActive()
30006	Reserved	—
30007	Reserved	—
30008	Reserved	—
3. Измерения

Все измерения берутся из ValueProvider.

Адрес	Название	Источник
30009	Input Voltage	ValueProvider* inletU()
30010	Input Power	ValueProvider* inletP()
30011	Input Current	ValueProvider* inletI()
30012	Input Frequency	ValueProvider* inletF()
30013	Temperature	ValueProvider* temperature()
30014	Reserved	—
30015	Reserved	—
30016	Reserved	—

Значения считываются через:

valid()
value()

и масштабируются перед выдачей в регистр.

4. Время
Адрес	Название	Источник
30017	System Time Low	QDateTime::currentSecsSinceEpoch()
30018	System Time High	QDateTime::currentSecsSinceEpoch()
30019	Last Duration Test Low	TestController::lastLongSystemTest()
30020	Last Duration Test High	TestController::lastLongSystemTest()
30021–30024	Reserved	—
5. Данные по линиям

Данные берутся из LinesModel.

Получение линии:

Line *line = backend->lines()->line(index);
Структура блока линии
Смещение	Название	Источник
+0	Line Type	Line::mode()
+1	Line State	Line::status()
+2	Last Test Time Low	Line::lastMeasuredTest()
+3	Last Test Time High	Line::lastMeasuredTest()
+4	Reserved	—
+5	Reserved	—
Соответствие свойств Line
Свойство Line	Использование
mode()	Тип линии
status()	Состояние линии
lastMeasuredTest()	Дата последнего теста
power()	резерв на будущее
current()	резерв на будущее
voltage()	резерв на будущее
6. Формирование состояния шкафа (30002)

Состояние шкафа формируется функцией:

encodeCabinetState(BackendController *backend)

Приоритет формирования:

Emergency
Неисправность батареи
Нет заряда батареи
Функциональный тест
Тест на длительность
Авария по линиям
OK
7. Состояние батареи

Источник:
BatteryController

Код	Значение
0	Норма
1	Неисправность
2	Нет заряда
*/