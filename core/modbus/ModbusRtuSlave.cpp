#include "ModbusRtuSlave.h"

#include <QTimer>
#include <QDebug>
#include <QtSerialBus/QModbusRtuSerialServer>
#include <QtSerialBus/QModbusDevice>
#include <qserialport.h>

#include "../core/BackendController.h"
#include "ModbusRegisterMap.h"

ModbusRtuSlave::ModbusRtuSlave(BackendController *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_server(new QModbusRtuSerialServer(this))
    , m_inputCache(ModbusMap::InputRegisterCount, 0)
{
    connect(m_server, &QModbusServer::dataWritten,
            this, &ModbusRtuSlave::onDataWritten);

    auto *timer = new QTimer(this);
    timer->setInterval(500);
    connect(timer, &QTimer::timeout, this, &ModbusRtuSlave::refreshInputRegisters);
    timer->start();
}

ModbusRtuSlave::~ModbusRtuSlave()
{
    stop();
}

bool ModbusRtuSlave::start(const QString &portName,
                           int baudRate,
                           QSerialPort::Parity parity,
                           QSerialPort::DataBits dataBits,
                           QSerialPort::StopBits stopBits,
                           int slaveId)
{
    if (m_server->state() != QModbusDevice::UnconnectedState)
        m_server->disconnectDevice();

    setupServerMap();

    m_server->setConnectionParameter(QModbusDevice::SerialPortNameParameter, portName);
    m_server->setConnectionParameter(QModbusDevice::SerialParityParameter, parity);
    m_server->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudRate);
    m_server->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, dataBits);
    m_server->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, stopBits);

    m_server->setServerAddress(slaveId);

    if (!m_server->connectDevice()) {
        qWarning() << "Modbus RTU slave start failed:" << m_server->errorString();
        return false;
    }

    refreshInputRegisters();

    qDebug() << "Modbus RTU slave started on" << portName << "addr" << slaveId;
    return true;
}

void ModbusRtuSlave::stop()
{
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