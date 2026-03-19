//9600 8E1
#pragma once

#include <QObject>
#include <QVector>
#include <QSerialPort>
#include <QtSerialBus/QModbusDataUnit>

class QModbusRtuSerialServer;
class BackendController;

class ModbusRtuSlave : public QObject
{
    Q_OBJECT

public:
    explicit ModbusRtuSlave(BackendController *backend, QObject *parent = nullptr);
    ~ModbusRtuSlave() override;

    bool start(const QString &portName,
               int baudRate,
               QSerialPort::Parity parity,
               QSerialPort::DataBits dataBits,
               QSerialPort::StopBits stopBits,
               int slaveId);
    void stop();

    bool isRunning() const;

private slots:
    void onDataWritten(QModbusDataUnit::RegisterType table, int address, int size);
    void refreshInputRegisters();

private:
    void setupServerMap();
    void updateInputRegister(int address, quint16 value);

private:
    BackendController *m_backend = nullptr;
    QModbusRtuSerialServer *m_server = nullptr;
    QVector<quint16> m_inputCache;
};
