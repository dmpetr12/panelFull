#include "ModbusRtuSlave.h"

#include <QTimer>
#include <QDebug>
#include <QtSerialBus/QModbusRtuSerialServer>
#include <QtSerialBus/QModbusDevice>
#include <qserialport.h>

#include "../core/BackendController.h"
#include "ModbusRegisterMap.h"
#include "logger.h"

ModbusRtuSlave::ModbusRtuSlave(BackendController *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_server(nullptr)
    , m_inputCache(ModbusMap::InputRegisterCount, 0)
{
    recreateServer();

    auto *timer = new QTimer(this);
    timer->setInterval(500);
    connect(timer, &QTimer::timeout, this, &ModbusRtuSlave::refreshInputRegisters);
    timer->start();

    m_reconnect.setInterval(2000);
    m_reconnect.setSingleShot(false);
    connect(&m_reconnect, &QTimer::timeout, this, &ModbusRtuSlave::tryReconnect);
}

ModbusRtuSlave::~ModbusRtuSlave()
{
    stop();
}

void ModbusRtuSlave::recreateServer()
{
    if (m_server) {
        m_server->disconnectDevice();
        m_server->deleteLater();
        m_server = nullptr;
    }

    m_server = new QModbusRtuSerialServer(this);

    connect(m_server, &QModbusServer::dataWritten,
            this, &ModbusRtuSlave::onDataWritten);

    connect(m_server, &QModbusDevice::stateChanged, this,
            [this](QModbusDevice::State state) {
                if (state == QModbusDevice::ConnectedState) {
                    m_reconnect.stop();

                    if (!m_online) {
                        m_online = true;
                        emit serverOnline();
                    }
                } else if (state == QModbusDevice::UnconnectedState) {
                    if (m_online) {
                        m_online = false;
                        emit serverOffline(m_server ? m_server->errorString() : QString());
                    }

                    if (m_wantRunning && !m_reconnect.isActive())
                        m_reconnect.start();
                }
            });

    connect(m_server, &QModbusDevice::errorOccurred, this,
            [this](QModbusDevice::Error error) {
                if (error == QModbusDevice::NoError)
                    return;

                const QString reason = m_server ? m_server->errorString()
                                                : QStringLiteral("unknown error");

                emit errorOccurred(QStringLiteral("Modbus RTU server error on %1: %2")
                                       .arg(m_portName, reason));

                switch (error) {
                case QModbusDevice::ConnectionError:
                case QModbusDevice::ReadError:
                case QModbusDevice::WriteError:
                case QModbusDevice::TimeoutError:
                case QModbusDevice::UnknownError:
                    scheduleReconnect(reason);
                    break;
                default:
                    break;
                }
            });
}

bool ModbusRtuSlave::start(const QString &portName,
                           int baudRate,
                           QSerialPort::Parity parity,
                           QSerialPort::DataBits dataBits,
                           QSerialPort::StopBits stopBits,
                           int slaveId)
{
    m_portName = portName;
    m_baudRate = baudRate;
    m_parity = parity;
    m_dataBits = dataBits;
    m_stopBits = stopBits;
    m_slaveId = slaveId;
    m_wantRunning = true;

    if (!m_server)
        recreateServer();

    if (m_server->state() != QModbusDevice::UnconnectedState)
        m_server->disconnectDevice();

    setupServerMap();

    m_server->setConnectionParameter(QModbusDevice::SerialPortNameParameter, m_portName);
    m_server->setConnectionParameter(QModbusDevice::SerialParityParameter, m_parity);
    m_server->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, m_baudRate);
    m_server->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, m_dataBits);
    m_server->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, m_stopBits);

    m_server->setServerAddress(m_slaveId);

    if (!m_server->connectDevice()) {
        const QString msg = QString("Modbus RTU slave start failed: %1").arg(m_server->errorString());
        log(msg);
        emit errorOccurred(msg);

        if (!m_reconnect.isActive())
            m_reconnect.start();

        return false;
    }

    refreshInputRegisters();
    log(QString("Modbus RTU slave started on %1 %2").arg(m_portName).arg(m_slaveId));
    return true;
}

void ModbusRtuSlave::stop()
{
    m_wantRunning = false;
    m_reconnect.stop();

    if (m_server)
        m_server->disconnectDevice();
}

bool ModbusRtuSlave::isRunning() const
{
    return m_server && m_server->state() == QModbusDevice::ConnectedState;
}

void ModbusRtuSlave::setupServerMap()
{
    QModbusDataUnitMap map;

    map.insert(QModbusDataUnit::Coils,
               QModbusDataUnit(QModbusDataUnit::Coils, 0, ModbusMap::CoilCount));

    map.insert(QModbusDataUnit::InputRegisters,
               QModbusDataUnit(QModbusDataUnit::InputRegisters, 0, ModbusMap::InputRegisterCount));

    map.insert(QModbusDataUnit::DiscreteInputs,
               QModbusDataUnit(QModbusDataUnit::DiscreteInputs, 0, 16));

    map.insert(QModbusDataUnit::HoldingRegisters,
               QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 0, 32));

    m_server->setMap(map);
}

void ModbusRtuSlave::updateInputRegister(int address, quint16 value)
{
    if (address < 0 || address >= m_inputCache.size())
        return;

    if (m_inputCache[address] == value)
        return;

    m_inputCache[address] = value;
    m_server->setData(QModbusDataUnit::InputRegisters, address, value);
}

void ModbusRtuSlave::refreshInputRegisters()
{
    if (!m_backend || !m_server)
        return;

    for (int addr = 0; addr < ModbusMap::InputRegisterCount; ++addr) {
        const quint16 v = ModbusMap::readInputRegister(m_backend, addr);
        updateInputRegister(addr, v);
    }
}

void ModbusRtuSlave::onDataWritten(QModbusDataUnit::RegisterType table, int address, int size)
{
    if (table != QModbusDataUnit::Coils || !m_server || !m_backend)
        return;

    for (int i = 0; i < size; ++i) {
        quint16 rawValue = 0;
        if (!m_server->data(QModbusDataUnit::Coils, address + i, &rawValue))
            continue;

        const bool value = (rawValue != 0);

        if (ModbusMap::writeCoil(m_backend, address + i, value)) {
            m_server->setData(QModbusDataUnit::Coils, address + i, quint16(0));
        }
    }

    refreshInputRegisters();
}

void ModbusRtuSlave::scheduleReconnect(const QString &reason)
{
    if (m_server && m_server->state() != QModbusDevice::UnconnectedState)
        m_server->disconnectDevice();

    if (m_online) {
        m_online = false;
        emit serverOffline(reason);
    }

    if (m_wantRunning && !m_reconnect.isActive())
        m_reconnect.start();
}

void ModbusRtuSlave::tryReconnect()
{
    if (!m_wantRunning)
        return;

    if (isRunning()) {
        m_reconnect.stop();
        return;
    }

    recreateServer();
    start(m_portName, m_baudRate, m_parity, m_dataBits, m_stopBits, m_slaveId);
}