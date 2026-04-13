#pragma once

#include <QObject>
#include <QString>

class AppConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int timeAuto READ timeAuto NOTIFY configChanged)
    Q_PROPERTY(int timeTest READ timeTest NOTIFY configChanged)
    Q_PROPERTY(int numLines READ numLines NOTIFY configChanged)
    Q_PROPERTY(int maxRelayModules READ maxRelayModules NOTIFY configChanged)
    Q_PROPERTY(QString serialPort READ serialPort NOTIFY configChanged)
    Q_PROPERTY(int tempReg READ tempReg NOTIFY configChanged)

    Q_PROPERTY(bool modbusRtuEnabled READ modbusRtuEnabled NOTIFY configChanged)
    Q_PROPERTY(QString modbusRtuDevice READ modbusRtuDevice NOTIFY configChanged)
    Q_PROPERTY(int modbusRtuBaudRate READ modbusRtuBaudRate NOTIFY configChanged)
    Q_PROPERTY(QString modbusRtuParity READ modbusRtuParity NOTIFY configChanged)
    Q_PROPERTY(int modbusRtuDataBits READ modbusRtuDataBits NOTIFY configChanged)
    Q_PROPERTY(int modbusRtuStopBits READ modbusRtuStopBits NOTIFY configChanged)
    Q_PROPERTY(int modbusRtuSlaveId READ modbusRtuSlaveId NOTIFY configChanged)

    Q_PROPERTY(bool modbusTcpEnabled READ modbusTcpEnabled NOTIFY configChanged)
    Q_PROPERTY(QString modbusTcpBind READ modbusTcpBind NOTIFY configChanged)
    Q_PROPERTY(int modbusTcpPort READ modbusTcpPort NOTIFY configChanged)

    Q_PROPERTY(bool opcUaEnabled READ opcUaEnabled NOTIFY configChanged)
    Q_PROPERTY(QString opcUaEndpoint READ opcUaEndpoint NOTIFY configChanged)
    Q_PROPERTY(QString opcUaApplicationUri READ opcUaApplicationUri NOTIFY configChanged)
    Q_PROPERTY(QString opcUaServerName READ opcUaServerName NOTIFY configChanged)
    Q_PROPERTY(QString opcUaSecurityPolicy READ opcUaSecurityPolicy NOTIFY configChanged)

    Q_PROPERTY(bool webEnabled READ webEnabled NOTIFY configChanged)
    Q_PROPERTY(QString webBind READ webBind NOTIFY configChanged)
    Q_PROPERTY(int webPort READ webPort NOTIFY configChanged)
    Q_PROPERTY(bool webAllowRemote READ webAllowRemote NOTIFY configChanged)

    Q_PROPERTY(QString logLevel READ logLevel NOTIFY configChanged)

    Q_PROPERTY(int modbusRequestTimeout READ modbusRequestTimeout NOTIFY configChanged)
    Q_PROPERTY(int modbusPollInterval READ modbusPollInterval NOTIFY configChanged)

public:
    struct ModbusRtuSettings
    {
        bool enabled = false;
        QString device = "/dev/rs485_server";
        int baudRate = 9600;
        QString parity = "E";
        int dataBits = 8;
        int stopBits = 1;
        int slaveId = 10;
    };

    struct ModbusTcpSettings
    {
        bool enabled = false;
        QString bind = "0.0.0.0";
        int port = 502;
    };

    struct OpcUaSettings
    {
        bool enabled = false;
        QString endpoint = "opc.tcp://0.0.0.0:4840";
        QString applicationUri = "urn:emergency-lighting:controller";
        QString serverName = "Emergency Lighting Controller";
        QString securityPolicy = "None";
    };

    struct WebServerSettings
    {
        bool enabled = false;

        QString bind = "0.0.0.0";
        int port = 8080;

        bool allowRemote = true;

        QString apiPrefix = "/api";

        int maxConnections = 20;
        int requestTimeoutMs = 5000;
    };

    struct LoggingSettings
    {
        QString level = "INFO";
    };

    struct ModbusPollingSettings
    {
        int requestTimeoutMs = 350;
        int pollIntervalMs = 1000;
    };

public:
    explicit AppConfig(QObject *parent = nullptr);

    int timeAuto() const;
    int timeTest() const;
    int numLines() const;
    int maxRelayModules() const;
    QString serialPort() const;
    int tempReg() const;

    bool modbusRtuEnabled() const;
    QString modbusRtuDevice() const;
    int modbusRtuBaudRate() const;
    QString modbusRtuParity() const;
    int modbusRtuDataBits() const;
    int modbusRtuStopBits() const;
    int modbusRtuSlaveId() const;

    bool modbusTcpEnabled() const;
    QString modbusTcpBind() const;
    int modbusTcpPort() const;

    bool opcUaEnabled() const;
    QString opcUaEndpoint() const;
    QString opcUaApplicationUri() const;
    QString opcUaServerName() const;
    QString opcUaSecurityPolicy() const;

    const ModbusRtuSettings& modbusRtu() const;
    const ModbusTcpSettings& modbusTcp() const;
    const OpcUaSettings& opcUa() const;

    bool load(const QString &filePath);
    bool save(const QString &filePath) const;

    bool webEnabled() const;
    QString webBind() const;
    int webPort() const;
    bool webAllowRemote() const;

    const WebServerSettings& webServer() const;

    QString logLevel() const;

    int modbusRequestTimeout() const;
    int modbusPollInterval() const;

    const LoggingSettings& logging() const;
    const ModbusPollingSettings& modbusPolling() const;

signals:
    void configChanged();

private:
    int m_timeAuto = 1;
    int m_timeTest = 2;
    int m_numLines = 25;
    int m_maxRelayModules = 1;
    QString m_serialPort = "/dev/rs485_internal";
    int m_tempReg=0x301;

    ModbusRtuSettings m_modbusRtu;
    ModbusTcpSettings m_modbusTcp;
    OpcUaSettings m_opcUa;
    WebServerSettings m_web;

    LoggingSettings m_logging;
    ModbusPollingSettings m_modbusPolling;
};

