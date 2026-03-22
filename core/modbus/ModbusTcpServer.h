#pragma once

#include <QObject>
#include <QTimer>
#include <QtSerialBus/QModbusTcpServer>

class BackendController;

class ModbusTcpServer : public QObject
{
    Q_OBJECT

public:
    explicit ModbusTcpServer(QObject *parent = nullptr);
    ~ModbusTcpServer() override;

    void setBackend(BackendController *backend);

    bool start(const QString &listenAddress, int port);
    void stop();

    bool isRunning() const;

signals:
    void logMessage(const QString &msg);

private slots:
    void onStateChanged(int state);
    void onErrorOccurred(QModbusDevice::Error error);
    void onDataWritten(QModbusDataUnit::RegisterType table, int address, int size);
    void refreshInputRegisters();

private:
    bool setupServerMap();
    void processWrittenCoil(int address, bool value);
    void resetCoil(int address);

private:
    BackendController *m_backend = nullptr;
    QModbusTcpServer *m_server = nullptr;
    QTimer m_refreshTimer;
};