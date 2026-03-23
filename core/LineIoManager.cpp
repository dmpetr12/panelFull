#include "LineIoManager.h"
#include "modbusbus.h"
#include "linesmodel.h"
#include "line.h"

namespace {

static inline void setBitLocal(quint8 &v, int b, bool on)
{
    if (on) v |=  (quint8(1u) << b);
    else    v &= ~(quint8(1u) << b);
}

// MEAS (NO): on=true => coil=1
static inline void setMeasOn(quint8 &mask, int bitMeas, bool on)
{
    setBitLocal(mask, bitMeas, on);
}

// WORK (NC): lineOn=true => coil=0
static inline void setWorkLineOn(quint8 &mask, int bitWork, bool lineOn)
{
    setBitLocal(mask, bitWork, !lineOn);
}

} // namespace

LineIoManager::LineIoManager(QObject *parent)
    : QObject(parent)
    , m_StepSwich(new QTimer(this))
{
    for (int i = 0; i < MAX_MODULES; ++i) {
        m_lastInputs[i]     = 0x00;
        m_desiredRelays[i]  = 0x00;
        m_lastSentRelays[i] = 0xFF;
    }

    m_StepSwich->setSingleShot(true);

    connect(m_StepSwich, &QTimer::timeout, this, [this]() {
        if (m_twoStepKind == TwoStepKind::Step1) {
            m_twoStepKind = TwoStepKind::Step2;
            recomputeDesiredAll();
            applyAllModules(true);
            m_StepSwich->start(m_twoStepDelayMs);
            return;
        }

        if (m_twoStepKind == TwoStepKind::Step2) {
            m_twoStepKind = TwoStepKind::Step3;
            recomputeDesiredAll();
            applyAllModules(true);
            return;
        }
    });
}

void LineIoManager::bind(ModbusBus *bus, LinesModel *linesModel, int numLines)
{
    m_bus = bus;
    m_lines = linesModel;
    m_numLines = numLines;

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::onInputsUpdated(int moduleIndex, quint8 bits)
{
    if (moduleIndex < 0 || moduleIndex >= MAX_MODULES)
        return;

    if (m_lastInputs[moduleIndex] == bits)
        return;

    m_lastInputs[moduleIndex] = bits;

    if (moduleIndex == MODULE0) {
        updateFireStopFromModule0(bits);
        recomputeDesiredAll();
        applyAllModules(true);
        return;
    }

    // во время тестов входы линий игнорируем
    if (m_fireTestActive || m_singleTestActive || m_testForceFire)
        return;

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::updateFireStopFromModule0(quint8 bits0)
{
    const bool prevFire = fireActive();
    const bool prevStop = stopActive();
    const bool prevEmergency = emergencyActive();

    const bool newFire = bit(bits0, IN_FIRE);
    const bool newStop = bit(bits0, IN_STOP);

    m_fireInput = newFire;
    m_stopInput = newStop;

    const bool newFireActive = fireActive();
    const bool newStopActive = stopActive();
    const bool newEmergency = emergencyActive();

    if (!prevEmergency && newEmergency) {
        cancelTestsDueToEmergency();
        if (m_bus)
            m_bus->setModeNormal();
        emit emergencyStop();
    }

    if (prevFire != newFireActive)
        emit fireChanged(newFireActive);

    if (prevStop != newStopActive)
        emit stopChanged(newStopActive);
}

void LineIoManager::setForcedFire(bool on)
{
    if (m_forcedFireCommand == on)
        return;

    const bool prevFire = fireActive();
    const bool prevStop = stopActive();
    const bool prevEmergency = emergencyActive();

    m_forcedFireCommand = on;
    emit forcedFireChanged(m_forcedFireCommand);

    const bool newFire = fireActive();
    const bool newStop = stopActive();
    const bool newEmergency = emergencyActive();

    if (!prevEmergency && newEmergency) {
        cancelTestsDueToEmergency();
        if (m_bus)
            m_bus->setModeNormal();
        emit emergencyStop();
    }

    if (prevFire != newFire)
        emit fireChanged(newFire);

    if (prevStop != newStop)
        emit stopChanged(newStop);

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::setForcedStop(bool on)
{
    if (m_forcedStopCommand == on)
        return;

    const bool prevFire = fireActive();
    const bool prevStop = stopActive();
    const bool prevEmergency = emergencyActive();

    m_forcedStopCommand = on;

    const bool newFire = fireActive();
    const bool newStop = stopActive();
    const bool newEmergency = emergencyActive();

    if (!prevEmergency && newEmergency) {
        cancelTestsDueToEmergency();
        if (m_bus)
            m_bus->setModeNormal();
        emit emergencyStop();
    }

    if (prevFire != newFire)
        emit fireChanged(newFire);

    if (prevStop != newStop)
        emit stopChanged(newStop);

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::cancelTestsDueToEmergency()
{
    bool anyChanged = false;

    if (m_fireTestActive) {
        m_fireTestActive = false;
        m_fireTestLine = -1;
        emit fireTestActiveChanged(false);
        emit fireTestLineChanged(-1);
        anyChanged = true;
    }

    if (m_singleTestActive) {
        m_singleTestActive = false;
        m_singleTestLine = -1;
        emit singleLineTestActiveChanged(false);
        emit singleLineTestLineChanged(-1);
        anyChanged = true;
    }

    if (m_testForceFire) {
        m_testForceFire = false;
        anyChanged = true;
    }

    if (anyChanged)
        stopStepSwichTimers();
}

bool LineIoManager::requestSingleLineTestStart(int lineIndex)
{
    if (!m_bus || !m_lines) return false;
    if (lineIndex < 0 || lineIndex >= m_numLines) return false;

    if (emergencyActive()) return false;

    if (m_fireTestActive) {
        m_fireTestActive = false;
        m_fireTestLine = -1;
        stopStepSwichTimers();
        emit fireTestActiveChanged(false);
        emit fireTestLineChanged(-1);
    }

    m_testForceFire = true;
    m_singleTestActive = true;
    m_singleTestLine = lineIndex;

    if (m_bus)
        m_bus->setModeTest();

    emit singleLineTestActiveChanged(true);
    emit singleLineTestLineChanged(lineIndex);

    recomputeDesiredAll();
    applyAllModules(true);
    return true;
}

void LineIoManager::requestSingleLineTestStop()
{
    if (!m_singleTestActive)
        return;

    m_singleTestActive = false;
    m_singleTestLine = -1;
    m_testForceFire = false;

    emit singleLineTestActiveChanged(false);
    emit singleLineTestLineChanged(-1);

    if (m_bus)
        m_bus->setModeNormal();

    stopStepSwichTimers();
    recomputeDesiredAll();
    applyAllModules(true);
}

bool LineIoManager::requestFireTestStart(int lineIndex)
{
    if (!m_bus || !m_lines) return false;
    if (lineIndex < 0 || lineIndex >= m_numLines) return false;

    if (emergencyActive()) return false;

    if (m_singleTestActive) {
        m_singleTestActive = false;
        m_singleTestLine = -1;
        emit singleLineTestActiveChanged(false);
        emit singleLineTestLineChanged(-1);
    }

    stopStepSwichTimers();

    m_testForceFire = true;
    m_fireTestActive = true;
    m_fireTestLine = lineIndex;
    m_twoStepKind = TwoStepKind::Step1;

    if (m_bus)
        m_bus->setModeTest();

    emit fireTestActiveChanged(true);
    emit fireTestLineChanged(lineIndex);

    recomputeDesiredAll();
    applyAllModules(true);
    m_StepSwich->start(m_twoStepDelayMs);
    return true;
}

void LineIoManager::requestFireTestStop()
{
    if (!m_fireTestActive)
        return;

    stopStepSwichTimers();

    m_fireTestActive = false;
    m_fireTestLine = -1;
    m_testForceFire = false;

    if (m_bus)
        m_bus->setModeNormal();

    emit fireTestActiveChanged(false);
    emit fireTestLineChanged(-1);

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::forceApplyAll()
{
    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::recomputeDesiredAll()
{
    for (int m = 0; m < MAX_MODULES; ++m)
        m_desiredRelays[m] = 0x00;

    // FIRE выше STOP
    if (fireActive()) {
        fillFireMode();
        return;
    }

    if (stopActive()) {
        fillStopMode();
        return;
    }

    if (m_testForceFire) {
        if (m_fireTestActive && m_fireTestLine >= 0)
            fillFireTestMode(m_fireTestLine);
        else
            fillFireMode();
        return;
    }

    if (m_singleTestActive && m_singleTestLine >= 0) {
        fillSingleLineTestMode(m_singleTestLine);
        return;
    }

    if (m_fireTestActive && m_fireTestLine >= 0) {
        fillFireTestMode(m_fireTestLine);
        return;
    }

    fillNormalMode();
}

void LineIoManager::fillFireMode()
{
    setBit(m_desiredRelays[0], REL_SERV_0, true);

    for (int i = 0; i < m_numLines; ++i) {
        int mod, bMeas, bWork;
        if (!mapLineToRelayBits(i, mod, bMeas, bWork))
            continue;

        setMeasOn(m_desiredRelays[mod], bMeas, false);
        setWorkLineOn(m_desiredRelays[mod], bWork, true);
    }
}

void LineIoManager::fillSingleLineTestMode(int lineIndex)
{
    setBit(m_desiredRelays[0], REL_SERV_0, true);

    for (int i = 0; i < m_numLines; ++i) {
        int mod, bMeas, bWork;
        if (!mapLineToRelayBits(i, mod, bMeas, bWork))
            continue;

        setMeasOn(m_desiredRelays[mod], bMeas, false);
        setWorkLineOn(m_desiredRelays[mod], bWork, false);
    }

    int mod, bMeas, bWork;
    if (!mapLineToRelayBits(lineIndex, mod, bMeas, bWork))
        return;

    setWorkLineOn(m_desiredRelays[mod], bWork, false);
    setMeasOn(m_desiredRelays[mod], bMeas, true);
}

void LineIoManager::fillFireTestMode(int lineIndex)
{
    int mod, bMeas, bWork;
    if (!mapLineToRelayBits(lineIndex, mod, bMeas, bWork))
        return;

    switch (m_twoStepKind) {
    case TwoStepKind::Step1:
        fillFireMode();
        break;

    case TwoStepKind::Step2:
        fillFireMode();
        setMeasOn(m_desiredRelays[mod], bMeas, true);
        break;

    case TwoStepKind::Step3:
        fillFireMode();
        setWorkLineOn(m_desiredRelays[mod], bWork, false);
        setMeasOn(m_desiredRelays[mod], bMeas, true);
        m_twoStepKind = TwoStepKind::None;
        m_lastMes = lineIndex;
        break;

    default:
        break;
    }
}

void LineIoManager::fillStopMode()
{
    setBit(m_desiredRelays[0], REL_SERV_0, false);

    for (int i = 0; i < m_numLines; ++i) {
        int mod, bMeas, bWork;
        if (!mapLineToRelayBits(i, mod, bMeas, bWork))
            continue;

        const bool lineOn = wantLineOn(i);

        setMeasOn(m_desiredRelays[mod], bMeas, false);
        setWorkLineOn(m_desiredRelays[mod], bWork, lineOn);
    }
}

void LineIoManager::fillNormalMode()
{
    fillStopMode();
}

bool LineIoManager::wantLineOn(int lineIndex) const
{
    if (!m_lines) return true;

    Line *ln = m_lines->line(lineIndex);
    if (!ln) return true;

    if (ln->mode() == Line::Constant) return true;
    if (ln->mode() == Line::NotUsed)  return true;

    int mod, inBit;
    if (!mapLineToInputBit(lineIndex, mod, inBit))
        return true;

    return bit(m_lastInputs[mod], inBit);
}

void LineIoManager::applyAllModules(bool force)
{
    for (int m = 0; m < MAX_MODULES; ++m)
        applyModuleIfChanged(m, force);

    syncLineStatesFromDesired();
}

void LineIoManager::applyModuleIfChanged(int moduleIndex, bool force)
{
    if (!m_bus) return;
    if (!m_bus->isConnected()) return;
    if (moduleIndex < 0 || moduleIndex >= MAX_MODULES) return;

    quint8 desired = m_desiredRelays[moduleIndex];

    if (moduleIndex == MODULE0)
        setBit(desired, REL_SERV_1, m_alarmLampOn);

    if (!force && desired == m_lastSentRelays[moduleIndex])
        return;

    m_bus->setModuleRelaysBits(moduleIndex, desired);
    m_lastSentRelays[moduleIndex] = desired;
}

// сначала идет рабочее реле, потом измерительное
bool LineIoManager::mapLineToRelayBits(int lineIndex, int &moduleIndex, int &bitMeas, int &bitWork) const
{
    if (lineIndex < 0)
        return false;

    if (lineIndex < 3) {
        moduleIndex = 0;
        bitMeas = 3 + lineIndex * 2;
        bitWork = 2 + lineIndex * 2;
        return true;
    }

    const int idx = lineIndex - 3;
    const int mod = 1 + (idx / 4);
    const int pos = (idx % 4);

    if (mod < 1 || mod >= MAX_MODULES)
        return false;

    moduleIndex = mod;
    bitMeas = 1 + pos * 2;
    bitWork = 0 + pos * 2;
    return true;
}

bool LineIoManager::mapLineToInputBit(int lineIndex, int &moduleIndex, int &inputBit) const
{
    if (lineIndex < 0)
        return false;

    if (lineIndex == 0) { moduleIndex = 0; inputBit = IN_M0_LINE0; return true; }
    if (lineIndex == 1) { moduleIndex = 0; inputBit = IN_M0_LINE1; return true; }
    if (lineIndex == 2) { moduleIndex = 0; inputBit = IN_M0_LINE2; return true; }

    const int idx = lineIndex - 3;
    const int mod = 1 + (idx / 4);
    const int pos = (idx % 4);

    if (mod < 1 || mod >= MAX_MODULES)
        return false;

    moduleIndex = mod;
    inputBit = pos;
    return true;
}

void LineIoManager::syncLineStatesFromDesired()
{
    if (!m_lines)
        return;

    auto setLine = [&](int lineIndex, bool on)
    {
        if (lineIndex < 0 || lineIndex >= m_lines->rowCount())
            return;

        Line *ln = m_lines->line(lineIndex);
        if (!ln)
            return;

        if (ln->mode() == Line::NotUsed)
            return;

        ln->setLineState(on ? Line::On : Line::Off);
    };

    auto lineOnFromDesiredWorkBit = [&](int moduleIndex, int measBit, int workBit) -> bool
    {
        const quint8 desired = m_desiredRelays[moduleIndex];
        const bool measOn = ((desired >> measBit) & 0x01) != 0;
        if (measOn)
            return true;

        const bool coilOn = ((desired >> workBit) & 0x01) != 0;
        return !coilOn;
    };

    for (int i = 0; i < m_numLines; ++i) {
        int mod, bMeas, bWork;
        if (!mapLineToRelayBits(i, mod, bMeas, bWork))
            continue;

        setLine(i, lineOnFromDesiredWorkBit(mod, bMeas, bWork));
    }
}

void LineIoManager::setAlarmLamp(bool on)
{
    if (m_alarmLampOn == on)
        return;

    m_alarmLampOn = on;
    recomputeDesiredAll();
    applyAllModules(true);
}