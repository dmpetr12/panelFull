#include "LineIoManager.h"
#include "modbusbus.h"
#include "linesmodel.h"
#include "line.h"

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

namespace {
    // обычный битовый сеттер
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
    static inline bool getBit(quint8 v, int b)
    {
        return ((v >> b) & 0x01) != 0;
    }
} // namespace

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

    // module0: FIRE/STOP имеют приоритет всегда
    if (moduleIndex == MODULE0) {
        updateFireStopFromModule0(bits);
        recomputeDesiredAll();
        applyAllModules(true);   // форс, чтобы авария точно дошла
        return;
    }

    // остальные модули: во время теста игнорируем входы линий
    if (m_fireTestActive || m_forceFire)
        return;

    recomputeDesiredAll();
    applyAllModules(true);
}


void LineIoManager::updateFireStopFromModule0(quint8 bits0)
{
    const bool newFire = bit(bits0, IN_FIRE);
    const bool newStop = bit(bits0, IN_STOP);

    const bool fireCh = (newFire != m_fireInput);
    const bool stopCh = (newStop != m_stopInput);

    m_fireInput = newFire;
    m_stopInput = newStop;

    if (fireCh) emit fireChanged(m_fireInput);
    if (stopCh) emit stopChanged(m_stopInput);

    if (m_fireInput || m_stopInput) {
        if (m_fireTestActive) {
            m_fireTestActive = false;
            m_fireTestLine = -1;
            stopStepSwichTimers();
            emit fireTestActiveChanged(false);
            emit fireTestLineChanged(-1);
        }
        if (m_singleTestActive) {
            m_singleTestActive = false;
            m_singleTestLine = -1;
            emit singleLineTestActiveChanged(false);
            emit singleLineTestLineChanged(-1);
        }

        m_forceFire = false;
        if (m_bus) m_bus->setModeNormal();

        emit emergencyStop();
    }

}

void LineIoManager::setForcedFire(bool on)
{
    if (m_forceFire == on)
        return;
    stopStepSwichTimers();

    const bool prevEff = effectiveFire();
    m_forceFire = on;
    const bool newEff  = effectiveFire();
    if (prevEff != newEff)
        emit fireChanged(newEff);

    if (!on) {
        bool wasAnyTest = false;

        if (m_fireTestActive) {
            m_fireTestActive = false;
            m_fireTestLine = -1;
            emit fireTestActiveChanged(false);
            emit fireTestLineChanged(-1);
            wasAnyTest = true;
        }
        if (m_singleTestActive) {
            m_singleTestActive = false;
            m_singleTestLine = -1;
            emit singleLineTestActiveChanged(false);
            emit singleLineTestLineChanged(-1);
            wasAnyTest = true;
        }

        if (wasAnyTest) {
            if (m_bus) m_bus->setModeNormal();
        }
    }

    recomputeDesiredAll();
    applyAllModules(true);
}


bool LineIoManager::requestSingleLineTestStart(int lineIndex)
{
    if (!m_bus || !m_lines) return false;
    if (lineIndex < 0 || lineIndex >= m_numLines) return false;

    // Если реальный FIRE/STOP — тест запрещаем
    if (m_fireInput || m_stopInput) return false;

    // Останавливаем режим all-lines теста, если он был (чтобы не смешивать)
    if (m_fireTestActive) {
        m_fireTestActive = false;
        m_fireTestLine = -1;
        stopStepSwichTimers();
        emit fireTestActiveChanged(false);
        emit fireTestLineChanged(-1);
    }

    m_forceFire = true;                 // чтобы игнорировать входы линий во время теста
    m_singleTestActive = true;
    if (m_bus) m_bus->setModeTest();
    m_singleTestLine = lineIndex;

    emit singleLineTestActiveChanged(true);
    emit singleLineTestLineChanged(lineIndex);

    recomputeDesiredAll();
    applyAllModules(true);
    return true;
}

void LineIoManager::requestSingleLineTestStop()
{
    if (!m_singleTestActive) return;

    m_singleTestActive = false;
    m_singleTestLine = -1;

    emit singleLineTestActiveChanged(false);
    emit singleLineTestLineChanged(-1);

    // Важно: если all-lines тест не активен — отпускаем forceFire
    // (иначе останешься в "пожаре" без причины)
    m_forceFire = false;
    if (m_bus) m_bus->setModeNormal();
    stopStepSwichTimers();
    recomputeDesiredAll();
    applyAllModules(true);
}

bool LineIoManager::requestFireTestStart(int lineIndex)
{
    if (!m_bus || !m_lines) return false;
    if (lineIndex < 0 || lineIndex >= m_numLines) return false;

    // Если уже есть реальный FIRE/STOP — тест запрещаем
    if (m_fireInput || m_stopInput) return false;

    if (m_singleTestActive) {
        m_singleTestActive = false;
        m_singleTestLine = -1;
        emit singleLineTestActiveChanged(false);
        emit singleLineTestLineChanged(-1);
    }

    // 1) Включаем тестовый режим
    m_forceFire = true;
    m_fireTestActive = true;
    stopStepSwichTimers();
    if (m_bus) m_bus->setModeTest();
    m_fireTestLine   = lineIndex;
    m_twoStepKind = TwoStepKind::Step1;
    emit fireTestActiveChanged(true);
    emit fireTestLineChanged(lineIndex);
    recomputeDesiredAll();
    applyAllModules(true);
    m_StepSwich->start(m_twoStepDelayMs);
    return true;
}


void LineIoManager::requestFireTestStop()
{
    if (!m_fireTestActive) return;
    stopStepSwichTimers();
    m_fireTestActive = false;
    m_fireTestLine   = -1;
    m_forceFire = false;
    if (m_bus) m_bus->setModeNormal();
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
    // сброс желаемого
    for (int m = 0; m < MAX_MODULES; ++m)
        m_desiredRelays[m] = 0x00;

    if (m_fireInput) { fillFireMode(); return;}
    if (m_stopInput) { fillStopMode(); return;}
    if (m_singleTestActive && m_singleTestLine >= 0) {
        fillSingleLineTestMode(m_singleTestLine);
        return;
    }
    if (m_forceFire) {
        if (m_fireTestActive && m_fireTestLine >= 0)
            fillFireTestMode(m_fireTestLine);
        else
            fillFireMode();
        return;
    }

    fillNormalMode();
}

void LineIoManager::fillFireMode()
{
    // REL_SERV_0: твоя служебная коммутация
    setBit(m_desiredRelays[0], REL_SERV_0, true);

    // В пожаре: ВСЕ линии должны быть включены (эвакуация)
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
    // служебная коммутация как в пожаре (если нужна)
    setBit(m_desiredRelays[0], REL_SERV_0, true);

    // 1) Выключить ВСЕ линии: WORK=OFF, MEAS=OFF
    for (int i = 0; i < m_numLines; ++i) {
        int mod, bMeas, bWork;
        if (!mapLineToRelayBits(i, mod, bMeas, bWork))
            continue;

        setMeasOn(m_desiredRelays[mod], bMeas, false);
        setWorkLineOn(m_desiredRelays[mod], bWork, false); // lineOn=false => coil=1 => OFF
    }

    // 2) Включить ТОЛЬКО тестируемую линию через MEAS
    int mod, bMeas, bWork;
    if (!mapLineToRelayBits(lineIndex, mod, bMeas, bWork))
        return;

    setWorkLineOn(m_desiredRelays[mod], bWork, false); // WORK держим OFF
    setMeasOn(m_desiredRelays[mod], bMeas, true);      // MEAS ON => линия включена
}

void LineIoManager::fillFireTestMode(int lineIndex)
{
    int mod, bMeas, bWork;
    if (!mapLineToRelayBits(lineIndex, mod, bMeas, bWork))
        return;
    switch (m_twoStepKind ) {
    case TwoStepKind::Step1:
        fillFireMode();
        if(mapLineToRelayBits(m_lastMes, mod, bMeas, bWork))
            setMeasOn(m_desiredRelays[mod], bMeas, true);
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

    // Constant и NotUsed — всегда включены (по твоему требованию)
    if (ln->mode() == Line::Constant) return true;
    if (ln->mode() == Line::NotUsed)  return true;

    int mod, inBit;
    if (!mapLineToInputBit(lineIndex, mod, inBit)) {
        return true; // безопасно включаем
    }
    return bit(m_lastInputs[mod], inBit);
}

void LineIoManager::applyAllModules(bool force)
{
    for (int m = 0; m < MAX_MODULES; ++m)
        applyModuleIfChanged(m, force);

    // UI обновляем по нашему desired (сразу, без опроса)
    syncLineStatesFromDesired();
}

void LineIoManager::applyModuleIfChanged(int moduleIndex, bool force)
{
    if (!m_bus) return;
    if (!m_bus->isConnected()) return;
    if (moduleIndex < 0 || moduleIndex >= MAX_MODULES) return;

    quint8 desired = m_desiredRelays[moduleIndex];

    if (moduleIndex == MODULE0) {
        setBit(desired, REL_SERV_1, m_alarmLampOn);
    }

    if (!force && desired == m_lastSentRelays[moduleIndex])
        return;

    m_bus->setModuleRelaysBits(moduleIndex, desired);
    m_lastSentRelays[moduleIndex] = desired;
}

// сначала идет рабочее  реле, потом измерительное
bool LineIoManager::mapLineToRelayBits(int lineIndex, int &moduleIndex, int &bitMeas, int &bitWork) const
{
    if (lineIndex < 0) return false;

    if (lineIndex < 3) {
        moduleIndex = 0;
        bitMeas = 3 + lineIndex * 2;
        bitWork = 2 + lineIndex * 2;
        return true;
    }

    const int idx = lineIndex - 3;
    const int mod = 1 + (idx / 4);
    const int pos = (idx % 4);

    if (mod < 1 || mod >= MAX_MODULES) return false;

    moduleIndex = mod;
    bitMeas = 1 + pos * 2;
    bitWork = 0 + pos * 2;
    return true;
}

bool LineIoManager::mapLineToInputBit(int lineIndex, int &moduleIndex, int &inputBit) const
{
    if (lineIndex < 0) return false;

    if (lineIndex == 0) { moduleIndex = 0; inputBit = IN_M0_LINE0; return true; }
    if (lineIndex == 1) { moduleIndex = 0; inputBit = IN_M0_LINE1; return true; }
    if (lineIndex == 2) { moduleIndex = 0; inputBit = IN_M0_LINE2; return true; }

    const int idx = lineIndex - 3;
    const int mod = 1 + (idx / 4);
    const int pos = (idx % 4);

    if (mod < 1 || mod >= MAX_MODULES) return false;

    moduleIndex = mod;
    inputBit = pos;
    return true;
}

void LineIoManager::syncLineStatesFromDesired()
{
    if (!m_lines) return;

    auto setLine = [&](int lineIndex, bool on)
    {
        if (lineIndex < 0 || lineIndex >= m_lines->rowCount()) return;
        Line *ln = m_lines->line(lineIndex);
        if (!ln) return;
        if ( ln->mode() == Line::NotUsed) return;
        ln->setLineState(on ? Line::On : Line::Off);
    };

    auto lineOnFromDesiredWorkBit = [&](int moduleIndex, int measBit, int workBit) -> bool
    {
        const quint8 desired = m_desiredRelays[moduleIndex];
        const bool measOn = (desired >> measBit) & 0x01;   // NO: 1 => замкнуло
        if (measOn) return true;                           // во время теста "фактически включена"
        const bool coilOn = ((desired >> workBit) & 0x01); // 1 = катушка включена
        return !coilOn; // NC: coil=0 => линия ON
    };

    for( int i=0 ; i<m_numLines;i++ ){
        int mod, bMeas, bWork;
        if (!mapLineToRelayBits(i, mod, bMeas, bWork)) continue;
        setLine(i, lineOnFromDesiredWorkBit(mod, bMeas, bWork));
    }
}

void LineIoManager::setAlarmLamp(bool on)
{
    if (m_alarmLampOn == on) return;
    m_alarmLampOn = on;
    recomputeDesiredAll();
    applyAllModules(true);
}

