#include "AppConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

AppConfig::AppConfig(QObject *parent)
    : QObject(parent)
{
}

int AppConfig::timeAuto() const { return m_timeAuto; }
int AppConfig::timeTest() const { return m_timeTest; }
int AppConfig::numLines() const { return m_numLines; }
int AppConfig::maxRelayModules() const { return m_maxRelayModules; }
QString AppConfig::serialPort() const { return m_serialPort; }

bool AppConfig::modbusRtuEnabled() const { return m_modbusRtu.enabled; }
QString AppConfig::modbusRtuDevice() const { return m_modbusRtu.device; }
int AppConfig::modbusRtuBaudRate() const { return m_modbusRtu.baudRate; }
QString AppConfig::modbusRtuParity() const { return m_modbusRtu.parity; }
int AppConfig::modbusRtuDataBits() const { return m_modbusRtu.dataBits; }
int AppConfig::modbusRtuStopBits() const { return m_modbusRtu.stopBits; }
int AppConfig::modbusRtuSlaveId() const { return m_modbusRtu.slaveId; }

bool AppConfig::modbusTcpEnabled() const { return m_modbusTcp.enabled; }
QString AppConfig::modbusTcpBind() const { return m_modbusTcp.bind; }
int AppConfig::modbusTcpPort() const { return m_modbusTcp.port; }

bool AppConfig::opcUaEnabled() const { return m_opcUa.enabled; }
QString AppConfig::opcUaEndpoint() const { return m_opcUa.endpoint; }
QString AppConfig::opcUaApplicationUri() const { return m_opcUa.applicationUri; }
QString AppConfig::opcUaServerName() const { return m_opcUa.serverName; }
QString AppConfig::opcUaSecurityPolicy() const { return m_opcUa.securityPolicy; }

const AppConfig::ModbusRtuSettings& AppConfig::modbusRtu() const { return m_modbusRtu; }
const AppConfig::ModbusTcpSettings& AppConfig::modbusTcp() const { return m_modbusTcp; }
const AppConfig::OpcUaSettings& AppConfig::opcUa() const { return m_opcUa; }

bool AppConfig::webEnabled() const { return m_web.enabled; }
QString AppConfig::webBind() const { return m_web.bind; }
int AppConfig::webPort() const { return m_web.port; }
bool AppConfig::webAllowRemote() const { return m_web.allowRemote; }

const AppConfig::WebServerSettings& AppConfig::webServer() const
{
    return m_web;
}

bool AppConfig::load(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "AppConfig: cannot open" << filePath << "for read";
        return false;
    }

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "AppConfig: JSON parse error in" << filePath
                   << ":" << err.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "AppConfig: root is not object in" << filePath;
        return false;
    }

    const QJsonObject obj = doc.object();

    if (obj.contains("timeAuto") && obj["timeAuto"].isDouble())
        m_timeAuto = obj["timeAuto"].toInt(m_timeAuto);

    if (obj.contains("timeTest") && obj["timeTest"].isDouble())
        m_timeTest = obj["timeTest"].toInt(m_timeTest);

    if (obj.contains("numLines") && obj["numLines"].isDouble())
        m_numLines = obj["numLines"].toInt(m_numLines);

    if (obj.contains("maxRelayModules") && obj["maxRelayModules"].isDouble())
        m_maxRelayModules = obj["maxRelayModules"].toInt(m_maxRelayModules);

    if (obj.contains("serialPort") && obj["serialPort"].isString())
        m_serialPort = obj["serialPort"].toString(m_serialPort);

    if (obj.contains("modbusRtu") && obj["modbusRtu"].isObject()) {
        const QJsonObject rtu = obj["modbusRtu"].toObject();

        if (rtu.contains("enabled") && rtu["enabled"].isBool())
            m_modbusRtu.enabled = rtu["enabled"].toBool(m_modbusRtu.enabled);

        if (rtu.contains("device") && rtu["device"].isString())
            m_modbusRtu.device = rtu["device"].toString(m_modbusRtu.device);

        if (rtu.contains("baudRate") && rtu["baudRate"].isDouble())
            m_modbusRtu.baudRate = rtu["baudRate"].toInt(m_modbusRtu.baudRate);

        if (rtu.contains("parity") && rtu["parity"].isString())
            m_modbusRtu.parity = rtu["parity"].toString(m_modbusRtu.parity);

        if (rtu.contains("dataBits") && rtu["dataBits"].isDouble())
            m_modbusRtu.dataBits = rtu["dataBits"].toInt(m_modbusRtu.dataBits);

        if (rtu.contains("stopBits") && rtu["stopBits"].isDouble())
            m_modbusRtu.stopBits = rtu["stopBits"].toInt(m_modbusRtu.stopBits);

        if (rtu.contains("slaveId") && rtu["slaveId"].isDouble())
            m_modbusRtu.slaveId = rtu["slaveId"].toInt(m_modbusRtu.slaveId);
    }

    if (obj.contains("modbusTcp") && obj["modbusTcp"].isObject()) {
        const QJsonObject tcp = obj["modbusTcp"].toObject();

        if (tcp.contains("enabled") && tcp["enabled"].isBool())
            m_modbusTcp.enabled = tcp["enabled"].toBool(m_modbusTcp.enabled);

        if (tcp.contains("bind") && tcp["bind"].isString())
            m_modbusTcp.bind = tcp["bind"].toString(m_modbusTcp.bind);

        if (tcp.contains("port") && tcp["port"].isDouble())
            m_modbusTcp.port = tcp["port"].toInt(m_modbusTcp.port);
    }

    if (obj.contains("opcUa") && obj["opcUa"].isObject()) {
        const QJsonObject opc = obj["opcUa"].toObject();

        if (opc.contains("enabled") && opc["enabled"].isBool())
            m_opcUa.enabled = opc["enabled"].toBool(m_opcUa.enabled);

        if (opc.contains("endpoint") && opc["endpoint"].isString())
            m_opcUa.endpoint = opc["endpoint"].toString(m_opcUa.endpoint);

        if (opc.contains("applicationUri") && opc["applicationUri"].isString())
            m_opcUa.applicationUri = opc["applicationUri"].toString(m_opcUa.applicationUri);

        if (opc.contains("serverName") && opc["serverName"].isString())
            m_opcUa.serverName = opc["serverName"].toString(m_opcUa.serverName);

        if (opc.contains("securityPolicy") && opc["securityPolicy"].isString())
            m_opcUa.securityPolicy = opc["securityPolicy"].toString(m_opcUa.securityPolicy);
    }

    if (obj.contains("webServer") && obj["webServer"].isObject()) {

        QJsonObject web = obj["webServer"].toObject();

        if (web.contains("enabled") && web["enabled"].isBool())
            m_web.enabled = web["enabled"].toBool(m_web.enabled);

        if (web.contains("bind") && web["bind"].isString())
            m_web.bind = web["bind"].toString(m_web.bind);

        if (web.contains("port") && web["port"].isDouble())
            m_web.port = web["port"].toInt(m_web.port);

        if (web.contains("allowRemote") && web["allowRemote"].isBool())
            m_web.allowRemote = web["allowRemote"].toBool(m_web.allowRemote);

        if (web.contains("apiPrefix") && web["apiPrefix"].isString())
            m_web.apiPrefix = web["apiPrefix"].toString(m_web.apiPrefix);

        if (web.contains("maxConnections") && web["maxConnections"].isDouble())
            m_web.maxConnections = web["maxConnections"].toInt(m_web.maxConnections);

        if (web.contains("requestTimeoutMs") && web["requestTimeoutMs"].isDouble())
            m_web.requestTimeoutMs = web["requestTimeoutMs"].toInt(m_web.requestTimeoutMs);
    }

    if (obj.contains("logging") && obj["logging"].isObject()) {
        QJsonObject log = obj["logging"].toObject();

        if (log.contains("level") && log["level"].isString())
            m_logging.level = log["level"].toString(m_logging.level);
    }

    if (obj.contains("modbusPolling") && obj["modbusPolling"].isObject()) {
        QJsonObject poll = obj["modbusPolling"].toObject();

        if (poll.contains("requestTimeoutMs") && poll["requestTimeoutMs"].isDouble())
            m_modbusPolling.requestTimeoutMs = poll["requestTimeoutMs"].toInt(m_modbusPolling.requestTimeoutMs);

        if (poll.contains("pollIntervalMs") && poll["pollIntervalMs"].isDouble())
            m_modbusPolling.pollIntervalMs = poll["pollIntervalMs"].toInt(m_modbusPolling.pollIntervalMs);
    }

    emit configChanged();
    return true;
}

bool AppConfig::save(const QString &filePath) const
{
    QJsonObject obj;
    obj["timeAuto"] = m_timeAuto;
    obj["timeTest"] = m_timeTest;
    obj["numLines"] = m_numLines;
    obj["maxRelayModules"] = m_maxRelayModules;
    obj["serialPort"] = m_serialPort;

    QJsonObject rtu;
    rtu["enabled"] = m_modbusRtu.enabled;
    rtu["device"] = m_modbusRtu.device;
    rtu["baudRate"] = m_modbusRtu.baudRate;
    rtu["parity"] = m_modbusRtu.parity;
    rtu["dataBits"] = m_modbusRtu.dataBits;
    rtu["stopBits"] = m_modbusRtu.stopBits;
    rtu["slaveId"] = m_modbusRtu.slaveId;
    obj["modbusRtu"] = rtu;

    QJsonObject tcp;
    tcp["enabled"] = m_modbusTcp.enabled;
    tcp["bind"] = m_modbusTcp.bind;
    tcp["port"] = m_modbusTcp.port;
    obj["modbusTcp"] = tcp;

    QJsonObject opc;
    opc["enabled"] = m_opcUa.enabled;
    opc["endpoint"] = m_opcUa.endpoint;
    opc["applicationUri"] = m_opcUa.applicationUri;
    opc["serverName"] = m_opcUa.serverName;
    opc["securityPolicy"] = m_opcUa.securityPolicy;
    obj["opcUa"] = opc;

    QJsonObject web;

    web["enabled"] = m_web.enabled;
    web["bind"] = m_web.bind;
    web["port"] = m_web.port;
    web["allowRemote"] = m_web.allowRemote;
    web["apiPrefix"] = m_web.apiPrefix;
    web["maxConnections"] = m_web.maxConnections;
    web["requestTimeoutMs"] = m_web.requestTimeoutMs;
    obj["webServer"] = web;


    QJsonObject log;
    log["level"] = m_logging.level;
    obj["logging"] = log;

    QJsonObject poll;
    poll["requestTimeoutMs"] = m_modbusPolling.requestTimeoutMs;
    poll["pollIntervalMs"] = m_modbusPolling.pollIntervalMs;
    obj["modbusPolling"] = poll;

    QJsonDocument doc(obj);

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "AppConfig: cannot open" << filePath << "for write";
        return false;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

QString AppConfig::logLevel() const { return m_logging.level; }

int AppConfig::modbusRequestTimeout() const { return m_modbusPolling.requestTimeoutMs; }
int AppConfig::modbusPollInterval() const { return m_modbusPolling.pollIntervalMs; }

const AppConfig::LoggingSettings& AppConfig::logging() const { return m_logging; }
const AppConfig::ModbusPollingSettings& AppConfig::modbusPolling() const { return m_modbusPolling; }
