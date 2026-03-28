#include "ModbusRegisterMap.h"

#include <QDateTime>
#include <QtMath>

#include "../core/BackendController.h"
#include "../core/LineIoManager.h"
#include "../core/BatteryController.h"
#include "../core/TestController.h"
#include "../core/ValueProvider.h"
#include "../core/linesmodel.h"
#include "../core/line.h"

namespace ModbusMap
{
static constexpr quint16 InvalidRegValue = 0xFFFF;

static quint16 lowWord(quint32 v)
{
    return static_cast<quint16>(v & 0xFFFF);
}

static quint16 highWord(quint32 v)
{
    return static_cast<quint16>((v >> 16) & 0xFFFF);
}

static quint32 safeDateTimeToU32(const QDateTime &dt)
{
    if (!dt.isValid())
        return 0;

    const qint64 secs = dt.toSecsSinceEpoch();
    if (secs <= 0)
        return 0;

    return static_cast<quint32>(secs);
}

static double readProvider(ValueProvider *p)
{
    if (!p || !p->valid())
        return 0.0;
    return p->value();
}

int lineBase(int lineIndex)
{
    return InRegLinesBase + lineIndex * LineBlockSize;
}

quint16 toScaled10(double v)
{
    if (v < 0.0)
        v = 0.0;
    return static_cast<quint16>(qRound(v * 10.0));
}

quint16 toScaled100(double v)
{
    if (v < 0.0)
        v = 0.0;
    return static_cast<quint16>(qRound(v * 100.0));
}

static quint16 readScaled10(ValueProvider *p)
{
    if (!p || !p->valid())
        return InvalidRegValue;
    return toScaled10(p->value());
}

static quint16 readScaled100(ValueProvider *p)
{
    if (!p || !p->valid())
        return InvalidRegValue;
    return toScaled100(p->value());
}

quint16 encodeBatteryState(BackendController *backend)
{
    if (!backend)
        return BatteryNormal;

    BatteryController *bat = backend->batteryController();
    if (!bat)
        return BatteryNormal;

    if (bat->batteryFault())
        return BatteryFault;

    if (bat->batteryLow()) // || (!bat->charging() && bat->chargePercent() <= 0.1))
        return BatteryNoCharge;

    return BatteryNormal;
}

quint16 encodeEmergencyState(BackendController *backend)
{
    if (!backend || !backend->lineIoManager())
        return EmergencyNone;

    const bool fire = backend->lineIoManager()->fireActive();

    return fire ? EmergencyFire : EmergencyNone;
}

quint16 encodeCabinetState(BackendController *backend)
{
    if (!backend)
        return CabinetOk;

    if (backend->lineIoManager() &&
        backend->lineIoManager()->fireActive()) {
        return CabinetEmergency;
    }

    const quint16 batState = encodeBatteryState(backend);
    if (batState == BatteryFault)
        return CabinetBatteryFault;
    if (batState == BatteryNoCharge)
        return CabinetBatteryNoCharge;

    TestController *tc = backend->testController();
    if (tc && tc->testActive()) {
        switch (tc->currentTestKind()) {
        case TestController::TestKind::Functional:
            return CabinetFunctionalTest;
        case TestController::TestKind::Duration:
            return CabinetDurationTest;
        case TestController::TestKind::None:
        default:
            break;
        }
    }

    LinesModel *lm = backend->lines();
    if (lm && lm->systemState() == 1)
        return CabinetLineAlarm;

    return CabinetOk;
}

quint16 encodeLineType(Line *line)
{
    if (!line)
        return LineNotUsed;

    switch (line->mode()) {
    case Line::Constant:
        return LineConstant;
    case Line::NonConstant:
        return LineNonConstant;
    case Line::NotUsed:
    default:
        return LineNotUsed;
    }
}

quint16 encodeLineOutputState(Line *line)
{
    if (!line)
        return LineOutputUnknown;

    if (line->mode() == Line::NotUsed)
        return LineOutputUnknown;

    switch (line->lineState()) {
    case Line::Off:
        return LineOutputOff;
    case Line::On:
        return LineOutputOn;
    default:
        return LineOutputUnknown;
    }
}

quint16 encodeLineState(Line *line)
{
    if (!line)
        return LineUnused;

    if (line->mode() == Line::NotUsed)
        return LineUnused;

    switch (line->status()) {
    case Line::OK:
        return LineOk;
    case Line::Failure:
        return LineAlarm;
    case Line::Test:
        return LineTest;
    case Line::Undefined:
    default:
        return LineAlarm;
    }
}

quint16 readInputRegister(BackendController *backend, int address)
{
    if (!backend)
        return 0;

    if (address == InRegCabinetEnabled)
        return  1 ;

    if (address == InRegCabinetState)
        return encodeCabinetState(backend);

    if (address == InRegEmergencyState)
        return encodeEmergencyState(backend);

    if (address == InRegLinesCount)
        return backend->lines() ? static_cast<quint16>(backend->lines()->rowCount()) : 0;

    if (address == InRegBatteryState)
        return encodeBatteryState(backend);

    if (address == InRegInputVoltage)
        return readScaled10(backend->inletU());

    if (address == InRegInputPower)
        return readScaled10(backend->inletP());

    if (address == InRegInputCurrent)
        return readScaled100(backend->inletI());

    if (address == InRegInputFrequency)
        return readScaled100(backend->inletF());

    if (address == InRegTemperature)
        return readScaled10(backend->temperature());

    if (address == InRegSystemTimeLow || address == InRegSystemTimeHigh) {
        const quint32 ts = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());
        return (address == InRegSystemTimeLow) ? lowWord(ts) : highWord(ts);
    }

    if (address == InRegLastDurationTestLow || address == InRegLastDurationTestHigh) {
        quint32 ts = 0;

        if (backend->testController())
            ts = safeDateTimeToU32(backend->testController()->lastLongSystemTest());

        return (address == InRegLastDurationTestLow) ? lowWord(ts) : highWord(ts);
    }

    if (address >= InRegLinesBase) {
        const int rel = address - InRegLinesBase;
        const int lineIndex = rel / LineBlockSize;
        const int offset = rel % LineBlockSize;

        if (!backend->lines())
            return 0;

        Line *line = backend->lines()->line(lineIndex);
        if (!line)
            return 0;

        switch (offset) {
        case LineType:
            return encodeLineType(line);

        case LineState:
            return encodeLineState(line);

        case LineOutputState:
            return encodeLineOutputState(line);

        case LineLastTestLow:
        case LineLastTestHigh: {
            const quint32 ts = safeDateTimeToU32(line->lastMeasuredTest());
            return (offset == LineLastTestLow) ? lowWord(ts) : highWord(ts);
        }

        case LineReserved1:
        default:
            return 0;
        }
    }

    return 0;
}

bool writeCoil(BackendController *backend, int address, bool value)
{
    if (!backend)
        return false;

    if (!value)
        return true;   // игнорируем отпускание импульса

    switch (address) {
    case CoilFireOn:
        return backend->setForcedFire(true);

    case CoilFireOff:
        return backend->setForcedFire(false);

    // case CoilStopOn:
    //     return backend->setForcedStop(true);

    // case CoilStopOff:
    //     return backend->setForcedStop(false);

    case CoilStopTest:
        return backend->stopCurrentTest();

    case CoilStartFunctionalTest:
        return backend->startFunctionalTest();

    case CoilStartDurationTest:
        return backend->startDurationTest();

    default:
        return false;
    }
}
}