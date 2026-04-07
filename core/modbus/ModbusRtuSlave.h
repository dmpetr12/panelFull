//9600 8E1
#pragma once

#include <QObject>
#include <QVector>
#include <QTimer>
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
    void recreateServer();
    void scheduleReconnect(const QString &reason);
    void tryReconnect();

signals:
    void errorOccurred(const QString &message);
    void serverOffline(const QString &reason);
    void serverOnline();

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

    QString m_portName;
    int m_baudRate = 9600;
    QSerialPort::Parity m_parity = QSerialPort::EvenParity;
    QSerialPort::DataBits m_dataBits = QSerialPort::Data8;
    QSerialPort::StopBits m_stopBits = QSerialPort::OneStop;
    int m_slaveId = 1;

    bool m_wantRunning = false;
    bool m_online = false;
    QTimer m_reconnect;
    QString m_lastError= "";
    bool m_portUnavailableLogged = false;
};
