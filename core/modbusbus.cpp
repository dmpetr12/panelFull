#include "modbusbus.h"
#include "AppConfig.h"

#include <QVariant>
#include <QSerialPort>
#include <QtSerialBus/QModbusReply>
#include <QDebug>
#include <QHash>
#include <QFile>

ModbusBus::ModbusBus(AppConfig *config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
    recreateClient();

    // NORMAL tick (1 sec)
    m_tickNormal.setInterval(1000);
    m_tickNormal.setSingleShot(false);
    connect(&m_tickNormal, &QTimer::timeout, this, &ModbusBus::normalTick);

    // TEST: fast meter tick (500 ms)
    m_tickTestFast.setInterval(500);
    m_tickTestFast.setSingleShot(false);
    connect(&m_tickTestFast, &QTimer::timeout, this, &ModbusBus::testFastTick);

    // TEST: fire inputs tick (500 ms)
    m_tickTestFire.setInterval(500);
    m_tickTestFire.setSingleShot(false);
    connect(&m_tickTestFire, &QTimer::timeout, this, &ModbusBus::testFireTick);

    m_reconnect.setInterval(m_reconnectMs);
    m_reconnect.setSingleShot(false);

    connect(&m_reconnect, &QTimer::timeout, this, [this](){
        if (!m_wantConnected)
            return;

        if (isConnected()) {
            m_reconnect.stop();
            return;
        }

        recreateClient();
        setupDevice();

        if (!QFile::exists(m_portName)) {
            emit errorOccurred(QStringLiteral("Port not found: %1").arg(m_portName));
            return;
        }

        if (!m_modbus->connectDevice()) {
            emit errorOccurred(QStringLiteral("Reconnect failed on %1: %2")
                                   .arg(m_portName, m_modbus->errorString()));
            return;
        }
    });
}

ModbusBus::~ModbusBus()
{
    stopModeTimers();
    if (m_modbus)
        m_modbus->disconnectDevice();
}

void ModbusBus::recreateClient()
{
    if (m_modbus) {
        m_modbus->disconnectDevice();
        m_modbus->deleteLater();
        m_modbus = nullptr;
    }

    m_modbus = new QModbusRtuSerialClient(this);

    connect(m_modbus, &QModbusClient::stateChanged,
            this, &ModbusBus::onStateChanged);
    connect(m_modbus, &QModbusClient::errorOccurred,
            this, &ModbusBus::onErrorOccurred);
}

void ModbusBus::setPortName(const QString &portName) { m_portName = portName; }
void ModbusBus::setBaudRate(int baud) { m_baudRate = baud; }
void ModbusBus::setRelayModuleCount(int count) { m_relayModuleCount = count; }
void ModbusBus::setRelayBaseAddress(int addr) { m_relayBaseAddr = addr; }
void ModbusBus::setTimeoutMs(int ms) { m_timeoutMs = ms; }
void ModbusBus::setRetries(int n) { m_retries = n; }

void ModbusBus::setInletMeterAddr(int addr) { m_addrInlet = addr; }
void ModbusBus::setTestMeterAddr(int addr) { m_addrTest = addr; }
void ModbusBus::setSht20Addr(int addr) { m_addrSht20 = addr; }

void ModbusBus::setSht20TempInputReg(int reg) { m_sht20TempReg = reg; }
void ModbusBus::setSht20TempScale(double scale) { m_sht20TempScale = scale; }

bool ModbusBus::isConnected() const
{
    return m_modbus && m_modbus->state() == QModbusDevice::ConnectedState;
}

void ModbusBus::setupDevice()
{
    if (!m_modbus)
        return;

    m_modbus->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QVariant(m_portName));
    m_modbus->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QVariant::fromValue(int(m_baudRate)));
    m_modbus->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QVariant::fromValue(int(QSerialPort::Data8)));
    m_modbus->setConnectionParameter(QModbusDevice::SerialParityParameter, QVariant::fromValue(int(QSerialPort::NoParity)));
    m_modbus->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QVariant::fromValue(int(QSerialPort::OneStop)));

    m_modbus->setTimeout(m_timeoutMs);
    m_modbus->setNumberOfRetries(m_retries);
}

void ModbusBus::connectDevice()
{
    if (!m_modbus) return;

    m_wantConnected = true;

    if (isConnected()) return;

    setupDevice();

    if (!m_modbus->connectDevice()) {
        emit errorOccurred(QStringLiteral("Modbus connect error: %1").arg(m_modbus->errorString()));
        m_reconnect.start();
    }
}

void ModbusBus::disconnectDevice()
{
    m_wantConnected = false;
    m_reconnect.stop();

    stopModeTimers();
    clearQueues();
    resetAllDeviceStates();

    if (m_modbus)
        m_modbus->disconnectDevice();
}

void ModbusBus::scheduleReconnect(const QString &reason)
{
    stopModeTimers();
    clearQueues();
    m_busy = false;

    if (m_busOnline) {
        m_busOnline = false;
        emit busOffline(reason);
    }

    if (m_modbus && m_modbus->state() != QModbusDevice::UnconnectedState)
        m_modbus->disconnectDevice();

    if (m_wantConnected && !m_reconnect.isActive())
        m_reconnect.start();
}

void ModbusBus::handleTransportFailure(const QString &reason)
{
    emit errorOccurred(QStringLiteral("Modbus transport failure on %1: %2")
                           .arg(m_portName, reason));

    if (m_reconnect.isActive())
        return;

    recreateClient();
    setupDevice();
    scheduleReconnect(reason);
}

void ModbusBus::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState) {
        m_reconnect.stop();
        m_sht20PollDivider = 0;
        resetAllDeviceStates();

        m_consecutiveTransportErrors = 0;
        m_hadAnySuccessSinceConnect = false;

        emit connectedChanged(true);
        startModeTimers();

        if (!m_busOnline) {
            m_busOnline = true;
            emit busOnline();
        }

        if (m_mode == Mode::Normal)
            normalTick();
        else {
            testFireTick();
            testFastTick();
        }
    }
    else if (state == QModbusDevice::UnconnectedState) {
        emit connectedChanged(false);
        stopModeTimers();
        clearQueues();

        if (m_busOnline) {
            m_busOnline = false;
            emit busOffline(m_modbus ? m_modbus->errorString() : QString());
        }

        if (m_wantConnected && !m_reconnect.isActive())
            m_reconnect.start();
    }
}

void ModbusBus::onErrorOccurred(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError)
        return;

    QString reason = m_modbus ? m_modbus->errorString() : QStringLiteral("unknown error");

    switch (error) {
    case QModbusDevice::ConnectionError:
    case QModbusDevice::WriteError:
    case QModbusDevice::UnknownError:
        ++m_consecutiveTransportErrors;

        emit errorOccurred(QStringLiteral(
                               "Modbus hard transport warning on %1: %2 failSeq=%3")
                               .arg(m_portName)
                               .arg(reason)
                               .arg(m_consecutiveTransportErrors));

        if (m_consecutiveTransportErrors >= m_transportFailThreshold ||
            !m_hadAnySuccessSinceConnect) {
            handleTransportFailure(reason);
            return;
        }
        return;

    case QModbusDevice::ReadError:
    case QModbusDevice::TimeoutError:
        emit errorOccurred(QStringLiteral(
                               "Modbus soft error on %1: %2")
                               .arg(m_portName)
                               .arg(reason));
        return;

    default:
        break;
    }

    if (m_wantConnected && !m_reconnect.isActive())
        m_reconnect.start();
}

// =======================
// РЕЖИМЫ ОПРОСА
// =======================

void ModbusBus::startModeTimers()
{
    if (!isConnected())
        return;

    stopModeTimers();

    if (m_mode == Mode::Normal) {
        m_tickNormal.start();
    } else {
        m_tickTestFire.start();
        m_tickTestFast.start();
    }
}

void ModbusBus::stopModeTimers()
{
    m_tickNormal.stop();
    m_tickTestFast.stop();
    m_tickTestFire.stop();
}

void ModbusBus::setModeNormal()
{
    m_mode = Mode::Normal;

    // CHANGED: возвращена старая логика
    clearPendingWrites();
    m_qNorm.clear();
    m_qLow.clear();

    if (isConnected())
        startModeTimers();
}

void ModbusBus::setModeTest()
{
    m_mode = Mode::Test;

    // CHANGED: возвращена старая логика
    clearPendingWrites();
    m_qNorm.clear();
    m_qLow.clear();

    if (isConnected())
        startModeTimers();
}

void ModbusBus::normalTick()
{
    if (!isConnected() || m_relayModuleCount <= 0)
        return;
    if (m_qHigh.size() > 4 || m_qNorm.size() > 8 || m_qLow.size() > 8)
        return;

    for (int moduleIndex = 0; moduleIndex < m_relayModuleCount; ++moduleIndex) {
        Request r;
        r.type = ReqType::ReadInputs;
        r.moduleIndex = moduleIndex;
        r.slaveAddr = m_relayBaseAddr + moduleIndex;
        r.start = 0;
        r.count = 8;
        enqueueNorm(r);
    }

    {
        Request r;
        r.type = ReqType::ReadHolding;
        r.slaveAddr = m_addrInlet;
        r.start = 0x000B;
        r.count = 7;
        r.meterKind = MeterKind::ADL200_Inlet;
        enqueueNorm(r);
    }

    {
        ++m_sht20PollDivider;
        if (m_sht20PollDivider >= m_sht20PollEveryTicks) {
            m_sht20PollDivider = 0;

            Request r;
            r.type = ReqType::ReadHolding;
            r.slaveAddr = m_addrSht20;
            r.start = m_sht20TempReg;
            r.count = 1;
            r.meterKind = MeterKind::SHT20_Temperature;
            enqueueNorm(r);
        }
    }
}

void ModbusBus::testFireTick()
{
    if (!isConnected() || m_relayModuleCount <= 0)
        return;
    if (m_qHigh.size() > 4 || m_qNorm.size() > 8)
        return;

    Request r;
    r.type = ReqType::ReadInputs;
    r.moduleIndex = 0;
    r.slaveAddr = m_relayBaseAddr + 0;
    r.start = 0;
    r.count = 8;
    enqueueNorm(r);
}

void ModbusBus::testFastTick()
{
    if (!isConnected())
        return;

    Request r;
    r.type = ReqType::ReadHolding;
    r.slaveAddr = m_addrTest;
    r.start = 0;
    r.count = 4;
    r.meterKind = MeterKind::DJSF_Test;

    enqueueLow(r);
}

// =======================
// (Опционально) round-robin опросы
// =======================

void ModbusBus::pollAllInputs()
{
    if (m_relayModuleCount <= 0) return;

    int moduleIndex = m_pollModuleInputs++ % m_relayModuleCount;

    Request r;
    r.type = ReqType::ReadInputs;
    r.moduleIndex = moduleIndex;
    r.slaveAddr = m_relayBaseAddr + moduleIndex;
    r.start = 0;
    r.count = 8;

    enqueueNorm(r);
}

void ModbusBus::pollAllRelays()
{
    if (m_relayModuleCount <= 0) return;

    int moduleIndex = m_pollModuleRelays++ % m_relayModuleCount;

    Request r;
    r.type = ReqType::ReadCoils;
    r.moduleIndex = moduleIndex;
    r.slaveAddr = m_relayBaseAddr + moduleIndex;
    r.start = 0;
    r.count = 8;

    enqueueLow(r);
}

// =======================
// РЕЛЕ
// =======================

void ModbusBus::setAllRelaysOff()
{
    if (m_relayModuleCount <= 0)
        return;

    for (int m = 0; m < m_relayModuleCount; ++m)
        setModuleRelaysBits(m, 0x00);
}

void ModbusBus::setModuleRelaysBits(int moduleIndex, quint8 bits)
{
    if (moduleIndex < 0 || moduleIndex >= m_relayModuleCount)
        return;

    Request r;
    r.type = ReqType::WriteCoils8;
    r.moduleIndex = moduleIndex;
    r.slaveAddr = m_relayBaseAddr + moduleIndex;
    r.start = 0;
    r.count = 8;
    r.coilsBits = bits; // CHANGED

    enqueueHigh(r);
}

// =======================
// Очереди + coalescing
// =======================

bool ModbusBus::samePeriodic(const Request& a, const Request& b)
{
    const auto isRead = [](ReqType t){
        return t == ReqType::ReadInputs || t == ReqType::ReadCoils
               || t == ReqType::ReadHolding || t == ReqType::ReadInputRegs;
    };
    if (!isRead(a.type) || !isRead(b.type))
        return false;

    return a.type == b.type
           && a.slaveAddr == b.slaveAddr
           && a.start == b.start
           && a.count == b.count
           && a.moduleIndex == b.moduleIndex
           && a.meterKind == b.meterKind;
}

// CHANGED
void ModbusBus::clearPendingWrites()
{
    for (auto it = m_qHigh.begin(); it != m_qHigh.end(); ) {
        if (it->type == ReqType::WriteCoils8)
            it = m_qHigh.erase(it);
        else
            ++it;
    }
}

void ModbusBus::enqueueHigh(const Request &r)
{
    // CHANGED: старая логика — оставляем только последнюю запись
    for (auto it = m_qHigh.begin(); it != m_qHigh.end(); ) {
        if (sameHighTarget(*it, r))
            it = m_qHigh.erase(it);
        else
            ++it;
    }

    m_qHigh.push_back(r);
    pump();
}

void ModbusBus::enqueueNorm(const Request &r)
{
    for (auto it = m_qNorm.begin(); it != m_qNorm.end(); ++it) {
        if (samePeriodic(*it, r))
            return;
    }
    m_qNorm.push_back(r);
    pump();
}

void ModbusBus::enqueueLow(const Request &r)
{
    for (auto it = m_qLow.begin(); it != m_qLow.end(); ++it) {
        if (samePeriodic(*it, r))
            return;
    }
    m_qLow.push_back(r);
    pump();
}

void ModbusBus::pump()
{
    if (!isConnected()) return;
    if (m_busy) return;

    Request r;
    bool has = false;

    if (!m_qHigh.empty()) { r = m_qHigh.front(); m_qHigh.pop_front(); has = true; }
    else if (!m_qNorm.empty()) { r = m_qNorm.front(); m_qNorm.pop_front(); has = true; }
    else if (!m_qLow.empty()) { r = m_qLow.front();  m_qLow.pop_front();  has = true; }

    if (!has) return;

    m_busy = true;
    sendRequest(r);
}

void ModbusBus::sendRequest(const Request &r)
{
    if (!m_modbus) { m_busy = false; return; }

    QModbusReply *reply = nullptr;

    switch (r.type) {
    case ReqType::WriteCoils8: {
        QModbusDataUnit unit(QModbusDataUnit::Coils, r.start, r.count);
        for (int i = 0; i < 8; ++i) {
            const bool on = (r.coilsBits >> i) & 0x01;
            unit.setValue(i, on ? 1 : 0);
        }
        reply = m_modbus->sendWriteRequest(unit, r.slaveAddr);
        break;
    }
    case ReqType::ReadInputs: {
        QModbusDataUnit unit(QModbusDataUnit::DiscreteInputs, r.start, r.count);
        reply = m_modbus->sendReadRequest(unit, r.slaveAddr);
        break;
    }
    case ReqType::ReadCoils: {
        QModbusDataUnit unit(QModbusDataUnit::Coils, r.start, r.count);
        reply = m_modbus->sendReadRequest(unit, r.slaveAddr);
        break;
    }
    case ReqType::ReadHolding: {
        QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, r.start, r.count);
        reply = m_modbus->sendReadRequest(unit, r.slaveAddr);
        break;
    }
    case ReqType::ReadInputRegs: {
        QModbusDataUnit unit(QModbusDataUnit::InputRegisters, r.start, r.count);
        reply = m_modbus->sendReadRequest(unit, r.slaveAddr);
        break;
    }
    }

    if (!reply) {
        const QString reason = m_modbus
                                   ? m_modbus->errorString()
                                   : QStringLiteral("modbus client is null");

        m_busy = false;
        handleTransportFailure(QStringLiteral("sendRequest failed: %1").arg(reason));
        return;
    }

    connect(reply, &QModbusReply::finished, this, [this, reply, r]() {
        const auto err = reply->error();
        const QString errStr = reply->errorString();
        const auto res = reply->result();
        reply->deleteLater();

        if (err != QModbusDevice::NoError) {
            markRequestFailure(r);

            const bool transportBroken =
                err == QModbusDevice::ConnectionError ||
                err == QModbusDevice::ReadError ||
                err == QModbusDevice::WriteError ||
                err == QModbusDevice::TimeoutError ||
                err == QModbusDevice::UnknownError;

            m_busy = false;

            if (transportBroken) {
                ++m_consecutiveTransportErrors;

                emit errorOccurred(QStringLiteral(
                                       "Modbus transport warning on %1: addr=%2 type=%3 err=%4 failSeq=%5")
                                       .arg(m_portName)
                                       .arg(r.slaveAddr)
                                       .arg(reqTypeName(r.type))
                                       .arg(errStr)
                                       .arg(m_consecutiveTransportErrors));

                if (m_hadAnySuccessSinceConnect &&
                    m_consecutiveTransportErrors < m_transportFailThreshold) {
                    pump();
                    return;
                }

                if (!m_hadAnySuccessSinceConnect &&
                    m_consecutiveTransportErrors < 3) {
                    pump();
                    return;
                }

                handleTransportFailure(errStr);
                return;
            }

            pump();
            return;
        }

        markRequestSuccess(r);

        if (r.type == ReqType::ReadInputs) {
            quint8 bits = 0;
            for (int i = 0; i < res.valueCount() && i < 8; ++i)
                if (res.value(i)) bits |= (1u << i);
            emit inputsUpdated(r.moduleIndex, bits);
        }
        else if (r.type == ReqType::ReadCoils) {
            quint8 bits = 0;
            for (int i = 0; i < res.valueCount() && i < 8; ++i)
                if (res.value(i)) bits |= (1u << i);
            emit relaysUpdated(r.moduleIndex, bits);
        }
        else if (r.type == ReqType::ReadHolding && r.meterKind == MeterKind::ADL200_Inlet) {
            if (res.valueCount() >= 7) {
                const double U = res.value(0) * 0.1;
                const double I = res.value(1) * 0.01;
                const double P = qint16(res.value(2)) * 1.0;
                const double F = res.value(6) * 0.01;

                emit inletMeterUpdated(U, I, P, F);
            }
        }
        else if (r.type == ReqType::ReadHolding && r.meterKind == MeterKind::DJSF_Test) {
            if (res.valueCount() >= 4) {
                auto s16 = [&](int idx)->qint16 { return qint16(res.value(idx)); };

                const qint16 rawU = s16(0);
                const qint16 dptU = s16(1);
                const qint16 rawI = s16(2);
                const qint16 dptI = s16(3);

                const double U = scaleDjsf(rawU, dptU);
                const double I = scaleDjsf(rawI, dptI);
                const double P = U * I;

                emit testMeterUpdated(U, I, P);
            }
        }
        else if (r.type == ReqType::ReadInputRegs && r.meterKind == MeterKind::SHT20_Temperature) {
            if (res.valueCount() >= 1) {
                const qint16 rawT = qint16(res.value(0));
                const double T = rawT * m_sht20TempScale;
                emit temperatureUpdated(T);
            }
        }

        m_busy = false;
        pump();
    });
}

void ModbusBus::clearQueues()
{
    m_qHigh.clear();
    m_qNorm.clear();
    m_qLow.clear();
    m_busy = false;
}

double ModbusBus::scaleDjsf(qint16 value, qint16 dp)
{
    double out = double(value);
    int exp = int(dp) - 3;
    while (exp > 0) { out *= 10.0; --exp; }
    while (exp < 0) { out /= 10.0; ++exp; }
    return out;
}

QString ModbusBus::deviceNameForRequest(const Request &r) const
{
    if (r.slaveAddr == m_addrInlet)
        return QStringLiteral("Счетчик входа");
    if (r.slaveAddr == m_addrTest)
        return QStringLiteral("Тестовый счетчик");
    if (r.slaveAddr == m_addrSht20)
        return QStringLiteral("Датчик температуры");

    if (r.slaveAddr >= m_relayBaseAddr &&
        r.slaveAddr < m_relayBaseAddr + m_relayModuleCount) {
        const int moduleIndex = r.slaveAddr - m_relayBaseAddr;
        return QStringLiteral("Релейный модуль %1").arg(moduleIndex + 1);
    }

    return QStringLiteral("Modbus устройство");
}

void ModbusBus::markRequestSuccess(const Request &r)
{
    DeviceState &st = m_deviceStates[r.slaveAddr];
    if (st.name.isEmpty())
        st.name = deviceNameForRequest(r);

    const bool wasOffline = !st.online;

    st.online = true;
    st.failCount = 0;

    if (wasOffline) {
        emit deviceOnline(st.name, r.slaveAddr);
    }

    m_consecutiveTransportErrors = 0;
    m_hadAnySuccessSinceConnect = true;
}

void ModbusBus::markRequestFailure(const Request &r)
{
    DeviceState &st = m_deviceStates[r.slaveAddr];
    if (st.name.isEmpty())
        st.name = deviceNameForRequest(r);

    st.failCount++;

    if (st.online && st.failCount >= m_failThreshold) {
        st.online = false;
        emit deviceOffline(st.name, r.slaveAddr);
    }
}

void ModbusBus::resetAllDeviceStates()
{
    for (auto it = m_deviceStates.begin(); it != m_deviceStates.end(); ++it) {
        it->online = true;
        it->failCount = 0;
    }
}