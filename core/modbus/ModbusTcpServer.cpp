#include "ModbusTcpServer.h"

#include <QHostAddress>
#include <QVariant>

#include "BackendController.h"
#include "../modbus/ModbusRegisterMap.h"

ModbusTcpServer::ModbusTcpServer(QObject *parent)
    : QObject(parent)
    , m_server(new QModbusTcpServer(this))
{
    connect(m_server, &QModbusServer::stateChanged,
            this, &ModbusTcpServer::onStateChanged);

    connect(m_server, &QModbusServer::errorOccurred,
            this, &ModbusTcpServer::onErrorOccurred);

    connect(m_server, &QModbusServer::dataWritten,
            this, &ModbusTcpServer::onDataWritten);

    m_refreshTimer.setInterval(500);
    m_refreshTimer.setSingleShot(false);

    connect(&m_refreshTimer, &QTimer::timeout,
            this, &ModbusTcpServer::refreshInputRegisters);
}

ModbusTcpServer::~ModbusTcpServer()
{
    stop();
}

void ModbusTcpServer::setBackend(BackendController *backend)
{
    m_backend = backend;
}

bool ModbusTcpServer::setupServerMap()
{
    if (!m_server)
        return false;

    QModbusDataUnitMap regMap;

    regMap.insert(QModbusDataUnit::Coils,
                  QModbusDataUnit(QModbusDataUnit::Coils, 0, ModbusMap::CoilCount));

    regMap.insert(QModbusDataUnit::DiscreteInputs,
                  QModbusDataUnit(QModbusDataUnit::DiscreteInputs, 0, 16));

    regMap.insert(QModbusDataUnit::InputRegisters,
                  QModbusDataUnit(QModbusDataUnit::InputRegisters, 0, ModbusMap::InputRegisterCount));

    regMap.insert(QModbusDataUnit::HoldingRegisters,
                  QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 0, 32));

    return m_server->setMap(regMap);
}

bool ModbusTcpServer::start(const QString &listenAddress, int port)
{
    if (!m_server)
        return false;

    if (m_server->state() != QModbusDevice::UnconnectedState)
        stop();

    if (!setupServerMap()) {
        emit logMessage(QStringLiteral("Modbus TCP: не удалось создать карту регистров"));
        return false;
    }

    m_server->setConnectionParameter(QModbusDevice::NetworkAddressParameter, listenAddress);
    m_server->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);

    // для TCP slave id обычно не критичен, но можно оставить 1
    m_server->setServerAddress(1);

    if (!m_server->connectDevice()) {
        emit logMessage(QStringLiteral("Modbus TCP start error: %1").arg(m_server->errorString()));
        return false;
    }

    refreshInputRegisters();
    m_refreshTimer.start();

    emit logMessage(QStringLiteral("Modbus TCP сервер запущен: %1:%2")
                        .arg(listenAddress)
                        .arg(port));
    return true;
}

void ModbusTcpServer::stop()
{
    m_refreshTimer.stop();

    if (m_server)
        m_server->disconnectDevice();
}

bool ModbusTcpServer::isRunning() const
{
    return m_server && m_server->state() == QModbusDevice::ConnectedState;
}

void ModbusTcpServer::onStateChanged(int state)
{
    QString text;
    switch (state) {
    case QModbusDevice::UnconnectedState:
        text = QStringLiteral("Unconnected");
        break;
    case QModbusDevice::ConnectingState:
        text = QStringLiteral("Connecting");
        break;
    case QModbusDevice::ConnectedState:
        text = QStringLiteral("Connected");
        break;
    case QModbusDevice::ClosingState:
        text = QStringLiteral("Closing");
        break;
    default:
        text = QStringLiteral("Unknown");
        break;
    }

    emit logMessage(QStringLiteral("Modbus TCP state: %1").arg(text));
}

void ModbusTcpServer::onErrorOccurred(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError)
        return;

    emit logMessage(QStringLiteral("Modbus TCP error: %1").arg(m_server->errorString()));
}

void ModbusTcpServer::onDataWritten(QModbusDataUnit::RegisterType table, int address, int size)
{
    if (!m_server || !m_backend)
        return;

    if (table != QModbusDataUnit::Coils)
        return;

    for (int i = 0; i < size; ++i) {
        quint16 value = 0;
        if (!m_server->data(QModbusDataUnit::Coils, address + i, &value))
            continue;

        processWrittenCoil(address + i, value != 0);
    }
}

void ModbusTcpServer::processWrittenCoil(int address, bool value)
{
    if (!m_backend)
        return;

    const bool ok = ModbusMap::writeCoil(m_backend, address, value);

    if (value) {
        emit logMessage(QStringLiteral("Modbus TCP coil %1 = 1, result=%2")
                            .arg(address + 1)
                            .arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));

        // импульсная команда: всегда сбрасываем обратно в 0
        resetCoil(address);
    }
}

void ModbusTcpServer::resetCoil(int address)
{
    if (!m_server)
        return;

    m_server->setData(QModbusDataUnit::Coils, address, 0);
}

void ModbusTcpServer::refreshInputRegisters()
{
    if (!m_server || !m_backend)
        return;

    for (int addr = 0; addr < ModbusMap::InputRegisterCount; ++addr) {
        const quint16 v = ModbusMap::readInputRegister(m_backend, addr);
        m_server->setData(QModbusDataUnit::InputRegisters, addr, v);
    }
}