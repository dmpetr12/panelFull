#include "LineIoManager.h"
#include "modbusbus.h"
#include "linesmodel.h"
#include "line.h"
#include "logger.h"

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
    , m_normalWatchdog(new QTimer(this))
{
    for (int i = 0; i < MAX_MODULES; ++i) {
        m_lastInputs[i]     = 0x00;
        m_desiredRelays[i]  = 0x00;
        m_lastSentRelays[i] = 0xFF;
        m_actualRelays[i] = 0x00;
        m_actualRelaysKnown[i] = false;
        m_normalMismatchCount[i] = 0;
        m_normalRepairAttempts[i] = 0;
    }

    m_normalWatchdog->setInterval(10000);
    m_normalWatchdog->setSingleShot(false);
    connect(m_normalWatchdog, &QTimer::timeout, this, [this]() {
        checkNormalRelayConsistency();
    });
    m_normalWatchdog->start();

    m_StepSwich->setSingleShot(true);

    connect(m_StepSwich, &QTimer::timeout, this, [this]() {
        if (!m_stepTestActive || m_stepTestLine < 0) {
            m_twoStepKind = TwoStepKind::None;
            return;
        }

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
    m_relayModuleCount = m_bus ? m_bus->relayModuleCount() : 0;

    // CHANGED: подключаем входы
    connect(m_bus, &ModbusBus::inputsUpdated,
            this, &LineIoManager::onInputsUpdated,
            Qt::UniqueConnection);

    // CHANGED: подключаем подтверждение состояния реле
    connect(m_bus, &ModbusBus::relaysUpdated,
            this, &LineIoManager::onRelaysUpdated,
            Qt::UniqueConnection);

    // CHANGED: после восстановления шины заново продавливаем нужное состояние
    connect(m_bus, &ModbusBus::busOnline,
            this, &LineIoManager::onBusOnline,
            Qt::UniqueConnection);
    // CHANGED: при потере шины подтверждённое состояние считаем неизвестным
    connect(m_bus, &ModbusBus::busOffline,
            this, &LineIoManager::onBusOffline,
            Qt::UniqueConnection);

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::onInputsUpdated(int moduleIndex, quint8 bits)
{
    if (moduleIndex < 0 || moduleIndex >= m_relayModuleCount)
        return;

    if (m_lastInputs[moduleIndex] == bits)
        return;
    const quint8 oldBits = m_lastInputs[moduleIndex];
    m_lastInputs[moduleIndex] = bits;

    if (moduleIndex == MODULE0) {
        updateFireFromModule0(bits);
        updateDoorFromModule0(bits);
        processProgramFireButtons(oldBits, bits);
        recomputeDesiredAll();
        applyAllModules(true);
        return;
    }

    // во время тестов входы линий игнорируем
    if (m_stepTestActive || m_singleTestActive || m_noMeasTestActive)
        return;

    recomputeDesiredAll();
    applyAllModules(true);
}

// CHANGED

void LineIoManager::onRelaysUpdated(int moduleIndex, quint8 bits)
{
    if (moduleIndex < 0 || moduleIndex >= m_relayModuleCount)
        return;

    const bool prevConfirmedFire = confirmedFireActive();
    const bool prevConfirmedStepTestActive = confirmedStepTestActive();
    const int prevConfirmedStepTestLine = confirmedStepTestLine();
    const bool prevConfirmedSingleTestActive = confirmedSingleLineTestActive();
    const int prevConfirmedSingleTestLine = confirmedSingleLineTestLine();

    m_actualRelays[moduleIndex] = bits;
    m_actualRelaysKnown[moduleIndex] = true;

    syncLineStatesFromActual();

    emitConfirmedStateChanges(prevConfirmedFire,
                              prevConfirmedStepTestActive,
                              prevConfirmedStepTestLine,
                              prevConfirmedSingleTestActive,
                              prevConfirmedSingleTestLine);
}

void LineIoManager::onBusOnline()
{
    for (int i = 0; i < m_relayModuleCount; ++i){
        m_lastSentRelays[i] = 0xFF;
        m_normalMismatchCount[i] = 0;
        m_normalRepairAttempts[i] = 0;
    }

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::onBusOffline(const QString &reason)
{
    Q_UNUSED(reason);

    const bool prevConfirmedFire = confirmedFireActive();
    const bool prevConfirmedStepTestActive = confirmedStepTestActive();
    const int prevConfirmedStepTestLine = confirmedStepTestLine();
    const bool prevConfirmedSingleTestActive = confirmedSingleLineTestActive();
    const int prevConfirmedSingleTestLine = confirmedSingleLineTestLine();

    for (int i = 0; i < m_relayModuleCount; ++i){
        m_lastSentRelays[i] = 0xFF;
        m_actualRelaysKnown[i] = false;
        m_normalMismatchCount[i] = 0;
        m_normalRepairAttempts[i] = 0;
    }

    emitConfirmedStateChanges(prevConfirmedFire,
                              prevConfirmedStepTestActive,
                              prevConfirmedStepTestLine,
                              prevConfirmedSingleTestActive,
                              prevConfirmedSingleTestLine);
}

void LineIoManager::emitConfirmedStateChanges(bool prevConfirmedFire,
                                              bool prevConfirmedStepTestActive,
                                              int prevConfirmedStepTestLine,
                                              bool prevConfirmedSingleTestActive,
                                              int prevConfirmedSingleTestLine)
{
    const bool newConfirmedFire = confirmedFireActive();
    const bool newConfirmedStepTestActive = confirmedStepTestActive();
    const int newConfirmedStepTestLine = confirmedStepTestLine();
    const bool newConfirmedSingleTestActive = confirmedSingleLineTestActive();
    const int newConfirmedSingleTestLine = confirmedSingleLineTestLine();

    if (prevConfirmedFire != newConfirmedFire)
        emit fireChanged(newConfirmedFire);

    if (prevConfirmedStepTestActive != newConfirmedStepTestActive)
        emit stepTestActiveChanged(newConfirmedStepTestActive);

    if (prevConfirmedStepTestLine != newConfirmedStepTestLine)
        emit stepTestLineChanged(newConfirmedStepTestLine);

    if (prevConfirmedSingleTestActive != newConfirmedSingleTestActive)
        emit singleLineTestActiveChanged(newConfirmedSingleTestActive);

    if (prevConfirmedSingleTestLine != newConfirmedSingleTestLine)
        emit singleLineTestLineChanged(newConfirmedSingleTestLine);
}

void LineIoManager::updateFireFromModule0(quint8 bits0)
{
    const bool prevFire = fireActive();

    const bool newFireInput = bit(bits0, IN_FIRE);
    m_fireInput = newFireInput;

    const bool newFire = fireActive();

    if (!prevFire && newFire) {
        cancelTestsDueToEmergency();
        if (m_bus)
            m_bus->setModeNormal();
        emit emergencyStop();
    }

    if (prevFire != newFire)
        emit fireChanged(newFire);
}

void LineIoManager::processProgramFireButtons(quint8 oldBits, quint8 newBits)
{
    const bool oldProgFireOn  = bit(oldBits, IN_PROG_FIRE_ON);
    const bool newProgFireOn  = bit(newBits, IN_PROG_FIRE_ON);

    const bool oldProgFireOff = bit(oldBits, IN_PROG_FIRE_OFF);
    const bool newProgFireOff = bit(newBits, IN_PROG_FIRE_OFF);

    if (!oldProgFireOn && newProgFireOn)
        emit programFireOnRequested();

    if (!oldProgFireOff && newProgFireOff)
        emit programFireOffRequested();
}

void LineIoManager::updateDoorFromModule0(quint8 bits0)
{
    const bool newDoorOpen = bit(bits0, IN_DOOR);  // замкнули = дверь открыта

    if (m_doorOpen == newDoorOpen)
        return;

    m_doorOpen = newDoorOpen;
    emit doorOpenChanged(m_doorOpen);
}

void LineIoManager::setProgramFire(bool on)
{
    if (m_programFire == on)
        return;

    const bool prevFire = fireActive();

    m_programFire = on;
    emit programFireChanged(m_programFire);

    const bool newFire = fireActive();

    if (!prevFire && newFire) {
        cancelTestsDueToEmergency();
        if (m_bus)
            m_bus->setModeNormal();
        emit emergencyStop();
    }

    if (prevFire != newFire)
        emit fireChanged(newFire);

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::stopProgramFire()
{
    setProgramFire(false);
}

bool LineIoManager::requestNoMeasTestStart()
{
    if (!m_bus || !m_lines)
        return false;

    if (fireActive())
        return false;

    cancelTestsDueToEmergency();

    m_noMeasTestActive = true;

    if (m_bus)
        m_bus->setModeTest();

    recomputeDesiredAll();
    applyAllModules(true);
    return true;
}

void LineIoManager::requestNoMeasTestStop()
{
    if (!m_noMeasTestActive)
        return;

    m_noMeasTestActive = false;

    if (m_bus)
        m_bus->setModeNormal();

    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::cancelTestsDueToEmergency()
{
    bool anyChanged = false;

    if (m_stepTestActive) {
        m_stepTestActive = false;
        m_stepTestLine = -1;
        m_twoStepKind = TwoStepKind::None;

        anyChanged = true;
    }

    if (m_singleTestActive) {
        m_singleTestActive = false;
        m_singleTestLine = -1;
        anyChanged = true;
    }

    if (m_noMeasTestActive) {
        m_noMeasTestActive = false;
        anyChanged = true;
    }

    if (anyChanged)
        stopStepSwichTimers();
}

bool LineIoManager::requestSingleLineTestStart(int lineIndex)
{
    if (!m_bus || !m_lines) return false;
    if (lineIndex < 0 || lineIndex >= m_numLines) return false;

    if (fireActive()) return false;

    if (m_stepTestActive) {
        m_stepTestActive = false;
        m_stepTestLine = -1;
        stopStepSwichTimers();
    }

    m_noMeasTestActive = false;
    m_singleTestActive = true;
    m_singleTestLine = lineIndex;

    if (m_bus)
        m_bus->setModeTest();

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


    if (m_bus)
        m_bus->setModeNormal();

    stopStepSwichTimers();
    recomputeDesiredAll();
    applyAllModules(true);
}

bool LineIoManager::requestStepTestStart(int lineIndex)
{
    if (!m_bus || !m_lines) return false;
    if (lineIndex < 0 || lineIndex >= m_numLines) return false;

    if (fireActive()) return false;

    if (m_singleTestActive) {
        m_singleTestActive = false;
        m_singleTestLine = -1;
    }

    m_noMeasTestActive = false;
    stopStepSwichTimers();

    m_stepTestActive = true;
    m_stepTestLine = lineIndex;
    m_twoStepKind = TwoStepKind::Step1;

    if (m_bus)
        m_bus->setModeTest();

    recomputeDesiredAll();
    applyAllModules(true);
    m_StepSwich->start(m_twoStepDelayMs);
    return true;
}

void LineIoManager::requestStepTestStop()
{
    stopStepSwichTimers();
    m_twoStepKind = TwoStepKind::None;

    m_stepTestActive = false;
    m_stepTestLine = -1;

    if (m_bus)
        m_bus->setModeNormal();

    recomputeDesiredAll();
    // for (int m = 0; m < m_relayModuleCount; ++m) {log(QString("STEP STOP desired: module=%1 desired=0x%2").arg(m).arg(QString::number(m_desiredRelays[m], 16).rightJustified(2, '0')));}
    applyAllModules(true);
    //log("STEP STOP end");
}

void LineIoManager::forceApplyAll()
{
    recomputeDesiredAll();
    applyAllModules(true);
}

void LineIoManager::checkNormalRelayConsistency()
{
    if (!m_bus || !m_bus->isConnected())
        return;

    if (currentMode() != Mode::Normal)
        return;

    constexpr int ConfirmMismatchCount = 2;
    constexpr int MaxRepairAttempts = 2;

    for (int m = 0; m < m_relayModuleCount; ++m) {
        if (!m_actualRelaysKnown[m])
            continue;

        const quint8 desired = m_desiredRelays[m];
        const quint8 actual  = m_actualRelays[m];

        if (actual == desired) {
            m_normalMismatchCount[m] = 0;
            m_normalRepairAttempts[m] = 0;
            continue;
        }

        ++m_normalMismatchCount[m];

        if (m_normalMismatchCount[m] < ConfirmMismatchCount)
            continue;
        if (m_normalRepairAttempts[m] == MaxRepairAttempts) {
            log(QString("❌ Реле module=%1 не удалось восстановить после %2 попыток")
                    .arg(m)
                    .arg(MaxRepairAttempts));
        }

        if (m_normalRepairAttempts[m] >= MaxRepairAttempts)
            continue;

        ++m_normalRepairAttempts[m];

        log(QString("⚠️ Несовпадение реле в NORMAL: module=%1 desired=0x%2 actual=0x%3, попытка восстановления %4/%5")
                .arg(m)
                .arg(QString::number(desired, 16).rightJustified(2, '0'))
                .arg(QString::number(actual, 16).rightJustified(2, '0'))
                .arg(m_normalRepairAttempts[m])
                .arg(MaxRepairAttempts));

        m_bus->setModuleRelaysBits(m, desired);
        m_lastSentRelays[m] = desired;
    }
}

LineIoManager::Mode LineIoManager::currentMode() const
{
    if (fireActive())
        return Mode::Fire;

    if (m_noMeasTestActive)
        return Mode::NoMeasTest;

    if (m_singleTestActive && m_singleTestLine >= 0)
        return Mode::SingleLineTest;

    if (m_stepTestActive && m_stepTestLine >= 0)
        return Mode::StepTest;

    return Mode::Normal;
}

void LineIoManager::recomputeDesiredAll()
{
    for (int m = 0; m < m_relayModuleCount; ++m)
        m_desiredRelays[m] = 0x00;

    switch (currentMode()) {
    case Mode::Fire:
        fillFireMode();
        break;

    case Mode::NoMeasTest:
        fillNoMeasTestMode();
        break;

    case Mode::SingleLineTest:
        fillSingleLineTestMode(m_singleTestLine);
        break;

    case Mode::StepTest:
        fillStepTestMode(m_stepTestLine);
        break;

    case Mode::Normal:
    default:
        fillNormalMode();
        break;
    }

    setBit(m_desiredRelays[MODULE0], REL_SERV_1, m_alarmLampOn);
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

void LineIoManager::fillNoMeasTestMode()
{
    fillFireMode();
}

void LineIoManager::fillSingleLineTestMode(int lineIndex)
{
    fillFireMode();

    int mod, bMeas, bWork;
    if (!mapLineToRelayBits(lineIndex, mod, bMeas, bWork))
        return;
    setWorkLineOn(m_desiredRelays[mod], bWork, false);
    setMeasOn(m_desiredRelays[mod], bMeas, true);
}

void LineIoManager::fillStepTestMode(int lineIndex)
{
    int mod, bMeas, bWork;
    if (!mapLineToRelayBits(lineIndex, mod, bMeas, bWork))
        return;

    switch (m_twoStepKind) {
    case TwoStepKind::Step1:
        fillNoMeasTestMode();
        break;

    case TwoStepKind::Step2:
        fillNoMeasTestMode();
        setMeasOn(m_desiredRelays[mod], bMeas, true);
        break;

    case TwoStepKind::Step3:
        fillNoMeasTestMode();
        setWorkLineOn(m_desiredRelays[mod], bWork, false);
        setMeasOn(m_desiredRelays[mod], bMeas, true);
        m_twoStepKind = TwoStepKind::None;
        m_lastMes = lineIndex;
        break;

    default:
        break;
    }
}

void LineIoManager::fillNormalMode()
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
    for (int m = 0; m < m_relayModuleCount; ++m)
        applyModuleIfChanged(m, force);
}

void LineIoManager::applyModuleIfChanged(int moduleIndex, bool force)
{
    if (!m_bus) return;
    if (!m_bus->isConnected()) return;
    if (moduleIndex < 0 || moduleIndex >= m_relayModuleCount) return;

    const quint8 desired = m_desiredRelays[moduleIndex];

    if (!force && desired == m_lastSentRelays[moduleIndex])
        return;
    //log(QString("APPLY send: module=%1 force=%2 desired=0x%3 lastSent=0x%4").arg(moduleIndex).arg(force).arg(QString::number(desired, 16).rightJustified(2, '0')).arg(QString::number(m_lastSentRelays[moduleIndex], 16).rightJustified(2, '0')));

    m_bus->setModuleRelaysBits(moduleIndex, desired);
    m_lastSentRelays[moduleIndex] = desired;
    m_lastRelayApplyMsec = QDateTime::currentMSecsSinceEpoch();
}

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

    if (mod < 1 || mod >= m_relayModuleCount)
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

    if (mod < 1 || mod >= m_relayModuleCount)
        return false;

    moduleIndex = mod;
    inputBit = pos;
    return true;
}

void LineIoManager::syncLineStatesFromActual()
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

    auto lineOnFromActualRelayBit = [&](int moduleIndex, int measBit, int workBit, bool &ok) -> bool
    {
        ok = false;

        if (moduleIndex < 0 || moduleIndex >= m_relayModuleCount)
            return false;

        if (!m_actualRelaysKnown[moduleIndex])
            return false;

        const quint8 actual = m_actualRelays[moduleIndex];

        const bool measOn = ((actual >> measBit) & 0x01) != 0;
        if (measOn) {
            ok = true;
            return true;
        }

        const bool coilOn = ((actual >> workBit) & 0x01) != 0;
        ok = true;
        return !coilOn;
    };

    for (int i = 0; i < m_numLines; ++i) {
        int mod, bMeas, bWork;
        if (!mapLineToRelayBits(i, mod, bMeas, bWork))
            continue;

        bool ok = false;
        const bool lineOn = lineOnFromActualRelayBit(mod, bMeas, bWork, ok);
        if (!ok)
            continue;

        setLine(i, lineOn);
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

bool LineIoManager::moduleMatchesDesired(int moduleIndex) const
{
    if (moduleIndex < 0 || moduleIndex >= m_relayModuleCount)
        return false;

    if (!m_actualRelaysKnown[moduleIndex])
        return false;

    return m_actualRelays[moduleIndex] == m_desiredRelays[moduleIndex];
}

bool LineIoManager::allRelevantModulesKnown() const
{
    for (int m = 0; m < m_relayModuleCount; ++m) {
        if (m_desiredRelays[m] != 0x00 || m_actualRelaysKnown[m]) {
            if (!m_actualRelaysKnown[m])
                return false;
        }
    }
    return true;
}

bool LineIoManager::anyRelevantMismatch() const
{
    for (int m = 0; m < m_relayModuleCount; ++m) {

        if (!m_actualRelaysKnown[m])
            return true;

        if (m_actualRelays[m] != m_desiredRelays[m])
            return true;
    }
    return false;
}

bool LineIoManager::relayStateKnown() const
{
    return allRelevantModulesKnown();
}

bool LineIoManager::relayMismatch() const
{
    return anyRelevantMismatch();
}

bool LineIoManager::isConfirmedFireByRelays() const
{
    if (!relayStateKnown())
        return false;

    if (relayMismatch())
        return false;

    // Подтверждаем пожар только если текущая логическая цель = Fire
    return currentMode() == Mode::Fire;
}

bool LineIoManager::confirmedFireActive() const
{
    return isConfirmedFireByRelays();
}

bool LineIoManager::isConfirmedTestByRelays() const
{
    if (!relayStateKnown())
        return false;

    if (relayMismatch())
        return false;

    const Mode mode = currentMode();
    return mode == Mode::NoMeasTest ||
           mode == Mode::SingleLineTest ||
           mode == Mode::StepTest;
}

bool LineIoManager::confirmedTestActive() const
{
    return isConfirmedTestByRelays();
}

bool LineIoManager::confirmedNoMeasTestActive() const
{
    return confirmedTestActive() && currentMode() == Mode::NoMeasTest;
}

bool LineIoManager::confirmedSingleLineTestActive() const
{
    return confirmedTestActive() && currentMode() == Mode::SingleLineTest;
}

int LineIoManager::confirmedSingleLineTestLine() const
{
    return confirmedSingleLineTestActive() ? m_singleTestLine : -1;
}

bool LineIoManager::confirmedStepTestActive() const
{
    return confirmedTestActive() && currentMode() == Mode::StepTest;
}

int LineIoManager::confirmedStepTestLine() const
{
    return confirmedStepTestActive() ? m_stepTestLine : -1;
}

bool LineIoManager::relayTransitionInProgress() const
{
    if (m_lastRelayApplyMsec <= 0)
        return false;

    return (QDateTime::currentMSecsSinceEpoch() - m_lastRelayApplyMsec) < m_transitionGuardMs;
}

LineIoManager::ConfirmedMode LineIoManager::confirmedMode() const
{
    if (!relayStateKnown())
        return ConfirmedMode::Alarm;

    if (relayMismatch())
        return ConfirmedMode::Alarm;

    if (isConfirmedFireByRelays())
        return ConfirmedMode::Fire;

    if (isConfirmedTestByRelays())
        return ConfirmedMode::Test;

    return ConfirmedMode::Normal;
}
