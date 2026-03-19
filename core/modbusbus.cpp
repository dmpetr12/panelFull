// ModbusBus.cpp
#include "modbusbus.h"

#include <QVariant>
#include <QSerialPort>
#include <QtSerialBus/QModbusReply>
#include <QDebug>
#include <QHash>

ModbusBus::ModbusBus(QObject *parent)
    : QObject(parent)
{
    m_modbus = new QModbusRtuSerialClient(this);

    connect(m_modbus, &QModbusClient::stateChanged,
            this, &ModbusBus::onStateChanged);
    connect(m_modbus, &QModbusClient::errorOccurred,
            this, &ModbusBus::onErrorOccurred);

    // NORMAL tick (1 sec)
    m_tickNormal.setInterval(1000);
    m_tickNormal.setSingleShot(false);
    connect(&m_tickNormal, &QTimer::timeout, this, &ModbusBus::normalTick);

    // TEST: fast meter tick (200 ms)
    m_tickTestFast.setInterval(200);
    m_tickTestFast.setSingleShot(false);
    connect(&m_tickTestFast, &QTimer::timeout, this, &ModbusBus::testFastTick);

    // TEST: fire inputs tick (500 ms)
    m_tickTestFire.setInterval(500);
    m_tickTestFire.setSingleShot(false);
    connect(&m_tickTestFire, &QTimer::timeout, this, &ModbusBus::testFireTick);

    m_reconnect.setInterval(m_reconnectMs);
    m_reconnect.setSingleShot(false);

    connect(&m_reconnect, &QTimer::timeout, this, [this](){
        if (!m_wantConnected) return;
        if (isConnected()) { m_reconnect.stop(); return; }

        // на всякий случай сбросить состояние
        if (m_modbus && m_modbus->state() != QModbusDevice::UnconnectedState)
            m_modbus->disconnectDevice();

        setupDevice();

        if (m_modbus && !m_modbus->connectDevice()) {
            emit errorOccurred(QStringLiteral("Reconnect failed: %1").arg(m_modbus->errorString()));
        }
    });
}

ModbusBus::~ModbusBus()
{
    stopModeTimers();
    if (m_modbus)
        m_modbus->disconnectDevice();
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
        m_reconnect.start();   // <-- ВАЖНО
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

void ModbusBus::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState) {
        m_reconnect.stop();
        m_sht20PollDivider = 0;
        resetAllDeviceStates();
        emit connectedChanged(true);
        startModeTimers();

        if (m_mode == Mode::Normal)
            normalTick();
        else {
            testFireTick();
            testFastTick();
        }
    } else if (state == QModbusDevice::UnconnectedState) {
        emit connectedChanged(false);
        stopModeTimers();
        clearQueues();

        if (m_wantConnected)
            m_reconnect.start();
    }
}

void ModbusBus::onErrorOccurred(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError) return;

    emit errorOccurred(QStringLiteral("Modbus error: %1").arg(m_modbus->errorString()));

    if (m_wantConnected && !isConnected())
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

    // срежем хвост тестовых запросов, но High (команды реле) не трогаем
    m_qNorm.clear();
    m_qLow.clear();

    if (isConnected())
        startModeTimers();
}

void ModbusBus::setModeTest()
{
    m_mode = Mode::Test;

    // срежем хвост обычных опросов
    m_qNorm.clear();
    m_qLow.clear();

    if (isConnected())
        startModeTimers();
}

void ModbusBus::normalTick()
{
    if (!isConnected() || m_relayModuleCount <= 0)
        return;

    // 1) ВХОДА всех модулей (1..8) — раз в 1 сек
    for (int moduleIndex = 0; moduleIndex < m_relayModuleCount; ++moduleIndex) {
        Request r;
        r.type = ReqType::ReadInputs;
        r.moduleIndex = moduleIndex;
        r.slaveAddr = m_relayBaseAddr + moduleIndex; // 1..8
        r.start = 0;
        r.count = 8;
        enqueueNorm(r);
    }

    // 2) ADL200 (slave 10): holding 0x000B..0x000D (3 regs)
    {
        Request r;
        r.type = ReqType::ReadHolding;
        r.slaveAddr = m_addrInlet;
        r.start = 0x000B;
        r.count = 7;
        r.meterKind = MeterKind::ADL200_Inlet;
        enqueueNorm(r);
    }

    // 3) SHT20 (slave 12): Input Registers (0x04), temp reg (default 0x0001), 1 reg
    {
        ++m_sht20PollDivider;
        if (m_sht20PollDivider >= m_sht20PollEveryTicks) {
            m_sht20PollDivider = 0;

            Request r;
            r.type = ReqType::ReadInputRegs;
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

    // Только модуль 0 (ПОЖАР) => slave = base + 0
    Request r;
    r.type = ReqType::ReadInputs;
    r.moduleIndex = 0;
    r.slaveAddr = m_relayBaseAddr + 0; // 1
    r.start = 0;
    r.count = 8;
    enqueueNorm(r);
}

void ModbusBus::testFastTick()
{
    if (!isConnected())
        return;

    // DJSF1352 (slave 11): holding 0..9 (10 regs)
    Request r;
    r.type = ReqType::ReadHolding;
    r.slaveAddr = m_addrTest;
    r.start = 0;
    r.count = 4;
    r.meterKind = MeterKind::DJSF_Test;

    // Low, чтобы команды реле (High) и пожарный опрос (Norm) были важнее
    enqueueLow(r);
}

// =======================
// (Опционально) старые round-robin опросы
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

void ModbusBus::setRelayGlobal(int index, bool on)
{
    if (!isConnected()) return;
    if (index < 0 || m_relayModuleCount <= 0) return;

    int moduleIndex = index / 8;
    int coilIndex   = index % 8;

    if (moduleIndex >= m_relayModuleCount) return;

    Request r;
    r.type = ReqType::WriteCoil;
    r.slaveAddr = m_relayBaseAddr + moduleIndex;
    r.coilIndex = coilIndex;
    r.coilOn = on;

    enqueueHigh(r);
}

void ModbusBus::setAllRelaysOff()
{
    if (m_relayModuleCount <= 0) return;

    for (int m = 0; m < m_relayModuleCount; ++m) {
        int slaveAddr = m_relayBaseAddr + m;
        for (int coil = 0; coil < 8; ++coil) {
            Request r;
            r.type = ReqType::WriteCoil;
            r.slaveAddr = slaveAddr;
            r.coilIndex = coil;
            r.coilOn = false;
            enqueueHigh(r);
        }
    }
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
    r.coilsBits = bits;

    enqueueHigh(r);
}

void ModbusBus::setAllRelaysOffFast()
{
    if (m_relayModuleCount <= 0) return;

    for (int m = 0; m < m_relayModuleCount; ++m) {
        setModuleRelaysBits(m, 0x00);
    }
}

// =======================
// Очереди + coalescing
// =======================

bool ModbusBus::samePeriodic(const Request& a, const Request& b)
{
    // периодическими считаем: любые Read* запросы
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

void ModbusBus::enqueueHigh(const Request &r)
{
    m_qHigh.push_back(r);
    pump();
}

void ModbusBus::enqueueNorm(const Request &r)
{
    // coalesce: оставляем только последний одинаковый периодический запрос
    for (auto it = m_qNorm.begin(); it != m_qNorm.end(); ++it) {
        if (samePeriodic(*it, r)) return;
    }
    m_qNorm.push_back(r);
    pump();
}

void ModbusBus::enqueueLow(const Request &r)
{
    for (auto it = m_qLow.begin(); it != m_qLow.end(); ++it) {
        if (samePeriodic(*it, r)) return;
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
    // qInfo().noquote() << QString("TX -> addr=%1  %2  start=%3 count=%4 module=%5 meter=%6")
    //     .arg(r.slaveAddr).arg(reqTypeName(r.type)).arg(r.start).arg(r.count).arg(r.moduleIndex).arg(int(r.meterKind));

    switch (r.type) {
    case ReqType::WriteCoil: {
        QModbusDataUnit unit(QModbusDataUnit::Coils, r.coilIndex, 1);
        unit.setValue(0, r.coilOn ? 1 : 0);
        reply = m_modbus->sendWriteRequest(unit, r.slaveAddr);
        break;
    }
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
        emit errorOccurred(QStringLiteral("sendRequest failed: %1").arg(m_modbus->errorString()));
        m_busy = false;
        QTimer::singleShot(0, this, &ModbusBus::pump);
        return;
    }

    connect(reply, &QModbusReply::finished, this, [this, reply, r]() {
        const auto err = reply->error();
        const QString errStr = reply->errorString();
        const auto res = reply->result();
        //qDebug() << "Get temp  "<< res.valueCount();
        reply->deleteLater();

        if (err != QModbusDevice::NoError) {
            markRequestFailure(r);

            emit errorOccurred(QStringLiteral("Modbus reply error: %1 (%2) addr=%3")
                                   .arg(errStr).arg(int(err)).arg(r.slaveAddr));

            m_busy = false;
            pump();
            return;
        }

        markRequestSuccess(r);

        // ===== обработка чтения =====
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
            // ADL200 holding registers: // 0x000B Voltage   (0.1V)
            // 0x000C Current   (0.01A)// 0x000D Active P  (0.001kW = 1W)
            // ...// 0x0011 Frequency (0.01Hz)
            // Поэтому запрос делай start=0x000B, count=7

            if (res.valueCount() >= 7) {
                const double U = res.value(0) * 0.1;
                const double I = res.value(1) * 0.01;

                // Active power лучше трактовать как signed (может быть отрицательная)
                const double P = qint16(res.value(2)) * 1.0;  // 0.001kW => 1W

                const double F = res.value(6) * 0.01;         // 0x0011, 0.01Hz

                emit inletMeterUpdated(U, I, P, F);
            }
        }
        else if (r.type == ReqType::ReadHolding && r.meterKind == MeterKind::DJSF_Test) {
            // DJSF1352: start=0 count=10 => U/DPT/I/DCT/.../P/DP
            if (res.valueCount() >= 4) {
                auto s16 = [&](int idx)->qint16 { return qint16(res.value(idx)); };

                const qint16 rawU = s16(0);
                const qint16 dptU = s16(1);
                const qint16 rawI = s16(2);
                const qint16 dptI = s16(3);
                // const qint16 rawP = s16(8);
                // const qint16 dptP = s16(9);

                const double U = scaleDjsf(rawU, dptU);
                const double I = scaleDjsf(rawI, dptI);
                const double P = U*I;

                emit testMeterUpdated(U, I, P);
            }
        }
        else if (r.type == ReqType::ReadInputRegs && r.meterKind == MeterKind::SHT20_Temperature) {
            // SHT20: 1 input register
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
    // value * 10^(dp-3)
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