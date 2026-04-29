#include "OpcUaServer.h"

#include "AppConfig.h"
#include "BackendController.h"
#include "logger.h"

#include <QByteArray>

namespace {

#ifdef PANEL_HAS_OPEN62541_SERVER
static UA_NodeId makeStringNodeId(quint16 nsIndex, const QString &id)
{
    return UA_NODEID_STRING_ALLOC(nsIndex, const_cast<char *>(id.toUtf8().constData()));
}

static UA_QualifiedName makeQualifiedName(quint16 nsIndex, const QString &name)
{
    return UA_QUALIFIEDNAME_ALLOC(nsIndex, const_cast<char *>(name.toUtf8().constData()));
}

static UA_LocalizedText makeText(const QString &text)
{
    return UA_LOCALIZEDTEXT_ALLOC(const_cast<char *>("ru-RU"),
                                  const_cast<char *>(text.toUtf8().constData()));
}
#endif

}

OpcUaServer::OpcUaServer(BackendController *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
{
#ifdef PANEL_HAS_OPEN62541_SERVER
    m_iterateTimer = new QTimer(this);
    m_iterateTimer->setInterval(20);
    connect(m_iterateTimer, &QTimer::timeout, this, &OpcUaServer::processServerEvents);
#endif
}

bool OpcUaServer::start(const AppConfig *config)
{
    if (!config) {
        log(QStringLiteral("OPC UA: config недоступен"));
        return false;
    }

    m_enabled = config->opcUaEnabled();
    m_bind = config->opcUaBind();
    m_port = config->opcUaPort();
    m_endpoint = config->opcUaEndpoint();

    if (!m_enabled) {
        m_running = false;
        log(QStringLiteral("OPC UA: отключен в config"));
        return true;
    }

    if (m_endpoint.isEmpty()) {
        m_endpoint = QStringLiteral("opc.tcp://%1:%2")
                         .arg(m_bind.isEmpty() ? QStringLiteral("0.0.0.0") : m_bind)
                         .arg(m_port > 0 ? m_port : 4840);
    }

#ifdef PANEL_HAS_OPEN62541_SERVER
    if (!initServer())
        return false;

    createAddressSpace();
    refreshModel();

    UA_StatusCode status = UA_Server_run_startup(m_server);
    if (status != UA_STATUSCODE_GOOD) {
        log(QStringLiteral("OPC UA: startup failed, status=0x%1")
                .arg(QString::number(status, 16)));
        shutdownServer();
        return false;
    }

    m_running = true;
    if (m_iterateTimer)
        m_iterateTimer->start();

    log(QStringLiteral("OPC UA server started, endpoint=%1").arg(m_endpoint));
#else
    m_running = true;
    log(QStringLiteral("OPC UA: включен, но сборка выполнена без open62541. Включи PANEL_ENABLE_OPCUA_SERVER=ON"));
#endif

    emit started();
    emit modelChanged();
    return true;
}

void OpcUaServer::stop()
{
    if (!m_running && !m_enabled)
        return;

#ifdef PANEL_HAS_OPEN62541_SERVER
    shutdownServer();
#endif

    const bool wasRunning = m_running;
    m_running = false;
    m_enabled = false;
    m_endpoint.clear();
    m_bind.clear();
    m_port = 0;

    if (wasRunning) {
        log(QStringLiteral("OPC UA server stopped"));
        emit stopped();
    }
}

bool OpcUaServer::enabled() const
{
    return m_enabled;
}

bool OpcUaServer::running() const
{
    return m_running;
}

QString OpcUaServer::endpoint() const
{
    return m_endpoint;
}

QString OpcUaServer::bind() const
{
    return m_bind;
}

int OpcUaServer::port() const
{
    return m_port;
}

QVariantMap OpcUaServer::namespaceSnapshot() const
{
    QVariantMap root;
    root[QStringLiteral("serverName")] = QStringLiteral("EmergencyLightingOpcUa");
    root[QStringLiteral("enabled")] = m_enabled;
    root[QStringLiteral("running")] = m_running;
    root[QStringLiteral("endpoint")] = m_endpoint;
    root[QStringLiteral("cabinet")] = buildCabinetNode();
    root[QStringLiteral("lines")] = buildLineNodes();
    root[QStringLiteral("commands")] = buildCommandNodes();
    return root;
}

QStringList OpcUaServer::commandNames() const
{
    return {
        QStringLiteral("FireOn"),
        QStringLiteral("FireOff"),
        QStringLiteral("StartFunctionalTest"),
        QStringLiteral("StartDurationTest"),
        QStringLiteral("StopTest"),
        QStringLiteral("ResetAlarm")
    };
}

bool OpcUaServer::invokeCommand(const QString &commandName)
{
    if (!m_backend)
        return false;

    if (commandName == QStringLiteral("FireOn"))
        return m_backend->setForcedFire(true);
    if (commandName == QStringLiteral("FireOff"))
        return m_backend->setForcedFire(false);
    if (commandName == QStringLiteral("StartFunctionalTest"))
        return m_backend->startFunctionalTest();
    if (commandName == QStringLiteral("StartDurationTest"))
        return m_backend->startDurationTest();
    if (commandName == QStringLiteral("StopTest"))
        return m_backend->stopCurrentTest();
    if (commandName == QStringLiteral("ResetAlarm"))
        return m_backend->resetAlarm();

    return false;
}

void OpcUaServer::refreshModel()
{
    emit modelChanged();

#ifdef PANEL_HAS_OPEN62541_SERVER
    if (!m_running || !m_backend || !m_server)
        return;

    const DeviceSnapshot snapshot = m_backend->snapshot();

    writeScalarString(m_cabinetVariableNodes.value(QStringLiteral("StateText")).toString(),
                      cabinetStateText(snapshot.cabinetMode));
    writeScalarInt32(m_cabinetVariableNodes.value(QStringLiteral("StateCode")).toString(),
                     snapshot.cabinetMode);
    writeScalarInt32(m_cabinetVariableNodes.value(QStringLiteral("SystemState")).toString(),
                     snapshot.systemState);
    writeScalarBool(m_cabinetVariableNodes.value(QStringLiteral("BusConnected")).toString(),
                    snapshot.busConnected);
    writeScalarBool(m_cabinetVariableNodes.value(QStringLiteral("DoorOpen")).toString(),
                    snapshot.doorOpen);
    writeScalarBool(m_cabinetVariableNodes.value(QStringLiteral("FireActive")).toString(),
                    snapshot.fireActive);
    writeScalarBool(m_cabinetVariableNodes.value(QStringLiteral("ProgramFireActive")).toString(),
                    snapshot.programFireActive);
    writeScalarBool(m_cabinetVariableNodes.value(QStringLiteral("FireInput")).toString(),
                    snapshot.fireInput);
    writeScalarBool(m_cabinetVariableNodes.value(QStringLiteral("TestRunning")).toString(),
                    snapshot.testRunning);
    writeScalarDouble(m_cabinetVariableNodes.value(QStringLiteral("BatteryChargePercent")).toString(),
                      snapshot.battery.chargePercent);

    for (int i = 0; i < static_cast<int>(snapshot.lines.size()) && i < m_lineVariableNodes.size(); ++i) {
        const auto vars = m_lineVariableNodes.at(i).toMap();
        const LineSnapshot &line = snapshot.lines.at(i);
        writeScalarString(vars.value(QStringLiteral("Name")).toString(), line.description);
        writeScalarString(vars.value(QStringLiteral("ModeText")).toString(), lineModeText(line.mode));
        writeScalarInt32(vars.value(QStringLiteral("Mode")).toString(), line.mode);
        writeScalarString(vars.value(QStringLiteral("StatusText")).toString(), lineStatusText(line.status));
        writeScalarInt32(vars.value(QStringLiteral("Status")).toString(), line.status);
        writeScalarDouble(vars.value(QStringLiteral("ControlledPower")).toString(), line.mpower);
    }
#endif
}

#ifdef PANEL_HAS_OPEN62541_SERVER
bool OpcUaServer::initServer()
{
    m_server = UA_Server_new();
    if (!m_server) {
        log(QStringLiteral("OPC UA: UA_Server_new failed"));
        return false;
    }

    UA_ServerConfig *config = UA_Server_getConfig(m_server);
    const UA_StatusCode status = UA_ServerConfig_setMinimal(config,
                                                            static_cast<quint16>(m_port > 0 ? m_port : 4840),
                                                            nullptr);
    if (status != UA_STATUSCODE_GOOD) {
        log(QStringLiteral("OPC UA: UA_ServerConfig_setMinimal failed, status=0x%1")
                .arg(QString::number(status, 16)));
        UA_Server_delete(m_server);
        m_server = nullptr;
        return false;
    }

    config->tcpReuseAddr = true;
    m_namespaceIndex = UA_Server_addNamespace(
        m_server,
        QByteArrayLiteral("urn:panelfull:emergency-lighting").constData());
    log(QStringLiteral("OPC UA: namespace index=%1").arg(m_namespaceIndex));
    return true;
}

void OpcUaServer::shutdownServer()
{
    if (m_iterateTimer)
        m_iterateTimer->stop();

    if (m_server) {
        if (m_running)
            UA_Server_run_shutdown(m_server);
        UA_Server_delete(m_server);
        m_server = nullptr;
    }

    UA_NodeId_clear(&m_rootNodeId);
    UA_NodeId_clear(&m_cabinetNodeId);
    UA_NodeId_clear(&m_linesNodeId);
    UA_NodeId_clear(&m_commandsNodeId);
    m_rootNodeId = UA_NODEID_NULL;
    m_cabinetNodeId = UA_NODEID_NULL;
    m_linesNodeId = UA_NODEID_NULL;
    m_commandsNodeId = UA_NODEID_NULL;
    m_namespaceIndex = 1;
    m_cabinetVariableNodes.clear();
    m_lineVariableNodes.clear();
}

void OpcUaServer::processServerEvents()
{
    if (m_server && m_running)
        UA_Server_run_iterate(m_server, false);
}

void OpcUaServer::createAddressSpace()
{
    if (!m_server || !m_backend)
        return;

    m_rootNodeId = addObjectNode(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                 QStringLiteral("CabinetPanel"),
                                 QStringLiteral("CabinetPanel"),
                                 QStringLiteral("Cabinet Panel"));

    m_cabinetNodeId = addObjectNode(m_rootNodeId,
                                    QStringLiteral("CabinetPanel.Cabinet"),
                                    QStringLiteral("Cabinet"),
                                    QStringLiteral("Cabinet"));
    m_linesNodeId = addObjectNode(m_rootNodeId,
                                  QStringLiteral("CabinetPanel.Lines"),
                                  QStringLiteral("Lines"),
                                  QStringLiteral("Lines"));
    m_commandsNodeId = addObjectNode(m_rootNodeId,
                                     QStringLiteral("CabinetPanel.Commands"),
                                     QStringLiteral("Commands"),
                                     QStringLiteral("Commands"));

    m_cabinetVariableNodes.insert(QStringLiteral("StateText"),
                                  QStringLiteral("CabinetPanel.Cabinet.StateText"));
    m_cabinetVariableNodes.insert(QStringLiteral("StateCode"),
                                  QStringLiteral("CabinetPanel.Cabinet.StateCode"));
    m_cabinetVariableNodes.insert(QStringLiteral("SystemState"),
                                  QStringLiteral("CabinetPanel.Cabinet.SystemState"));
    m_cabinetVariableNodes.insert(QStringLiteral("BusConnected"),
                                  QStringLiteral("CabinetPanel.Cabinet.BusConnected"));
    m_cabinetVariableNodes.insert(QStringLiteral("DoorOpen"),
                                  QStringLiteral("CabinetPanel.Cabinet.DoorOpen"));
    m_cabinetVariableNodes.insert(QStringLiteral("FireActive"),
                                  QStringLiteral("CabinetPanel.Cabinet.FireActive"));
    m_cabinetVariableNodes.insert(QStringLiteral("ProgramFireActive"),
                                  QStringLiteral("CabinetPanel.Cabinet.ProgramFireActive"));
    m_cabinetVariableNodes.insert(QStringLiteral("FireInput"),
                                  QStringLiteral("CabinetPanel.Cabinet.FireInput"));
    m_cabinetVariableNodes.insert(QStringLiteral("TestRunning"),
                                  QStringLiteral("CabinetPanel.Cabinet.TestRunning"));
    m_cabinetVariableNodes.insert(QStringLiteral("BatteryChargePercent"),
                                  QStringLiteral("CabinetPanel.Cabinet.BatteryChargePercent"));

    addStringVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.StateText"),
                          QStringLiteral("StateText"), QStringLiteral("State"), QStringLiteral("Работа"));
    addInt32VariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.StateCode"),
                         QStringLiteral("StateCode"), QStringLiteral("State Code"), 0);
    addInt32VariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.SystemState"),
                         QStringLiteral("SystemState"), QStringLiteral("System State"), 0);
    addBoolVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.BusConnected"),
                        QStringLiteral("BusConnected"), QStringLiteral("Bus Connected"), false);
    addBoolVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.DoorOpen"),
                        QStringLiteral("DoorOpen"), QStringLiteral("Door Open"), false);
    addBoolVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.FireActive"),
                        QStringLiteral("FireActive"), QStringLiteral("Fire Active"), false);
    addBoolVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.ProgramFireActive"),
                        QStringLiteral("ProgramFireActive"), QStringLiteral("Program Fire Active"), false);
    addBoolVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.FireInput"),
                        QStringLiteral("FireInput"), QStringLiteral("Fire Input"), false);
    addBoolVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.TestRunning"),
                        QStringLiteral("TestRunning"), QStringLiteral("Test Running"), false);
    addDoubleVariableNode(m_cabinetNodeId, QStringLiteral("CabinetPanel.Cabinet.BatteryChargePercent"),
                          QStringLiteral("BatteryChargePercent"), QStringLiteral("Battery Charge Percent"), 0.0);

    const DeviceSnapshot snapshot = m_backend->snapshot();
    for (int i = 0; i < static_cast<int>(snapshot.lines.size()); ++i) {
        const int lineNo = i + 1;
        const QString prefix = QStringLiteral("CabinetPanel.Lines.Line%1").arg(lineNo);
        const UA_NodeId lineObject = addObjectNode(m_linesNodeId,
                                                   prefix,
                                                   QStringLiteral("Line%1").arg(lineNo),
                                                   QStringLiteral("Line %1").arg(lineNo));
        QVariantMap vars;
        vars.insert(QStringLiteral("Name"), prefix + QStringLiteral(".Name"));
        vars.insert(QStringLiteral("Mode"), prefix + QStringLiteral(".Mode"));
        vars.insert(QStringLiteral("ModeText"), prefix + QStringLiteral(".ModeText"));
        vars.insert(QStringLiteral("Status"), prefix + QStringLiteral(".Status"));
        vars.insert(QStringLiteral("StatusText"), prefix + QStringLiteral(".StatusText"));
        vars.insert(QStringLiteral("ControlledPower"), prefix + QStringLiteral(".ControlledPower"));
        addStringVariableNode(lineObject, prefix + QStringLiteral(".Name"),
                              QStringLiteral("Name"), QStringLiteral("Name"),
                              snapshot.lines[i].description);
        addInt32VariableNode(lineObject, prefix + QStringLiteral(".Mode"),
                             QStringLiteral("Mode"), QStringLiteral("Mode"),
                             snapshot.lines[i].mode);
        addStringVariableNode(lineObject, prefix + QStringLiteral(".ModeText"),
                              QStringLiteral("ModeText"), QStringLiteral("Mode Text"),
                              lineModeText(snapshot.lines[i].mode));
        addInt32VariableNode(lineObject, prefix + QStringLiteral(".Status"),
                             QStringLiteral("Status"), QStringLiteral("Status"),
                             snapshot.lines[i].status);
        addStringVariableNode(lineObject, prefix + QStringLiteral(".StatusText"),
                              QStringLiteral("StatusText"), QStringLiteral("Status Text"),
                              lineStatusText(snapshot.lines[i].status));
        addDoubleVariableNode(lineObject, prefix + QStringLiteral(".ControlledPower"),
                              QStringLiteral("ControlledPower"), QStringLiteral("Controlled Power"),
                              snapshot.lines[i].mpower);
        m_lineVariableNodes.push_back(vars);
    }

    addMethodNode(m_commandsNodeId, QStringLiteral("CabinetPanel.Commands.FireOn"),
                  QStringLiteral("FireOn"), QStringLiteral("Fire On"),
                  QStringLiteral("FireOn"));
    addMethodNode(m_commandsNodeId, QStringLiteral("CabinetPanel.Commands.FireOff"),
                  QStringLiteral("FireOff"), QStringLiteral("Fire Off"),
                  QStringLiteral("FireOff"));
    addMethodNode(m_commandsNodeId, QStringLiteral("CabinetPanel.Commands.StartFunctionalTest"),
                  QStringLiteral("StartFunctionalTest"), QStringLiteral("Start Functional Test"),
                  QStringLiteral("StartFunctionalTest"));
    addMethodNode(m_commandsNodeId, QStringLiteral("CabinetPanel.Commands.StartDurationTest"),
                  QStringLiteral("StartDurationTest"), QStringLiteral("Start Duration Test"),
                  QStringLiteral("StartDurationTest"));
    addMethodNode(m_commandsNodeId, QStringLiteral("CabinetPanel.Commands.StopTest"),
                  QStringLiteral("StopTest"), QStringLiteral("Stop Test"),
                  QStringLiteral("StopTest"));
    addMethodNode(m_commandsNodeId, QStringLiteral("CabinetPanel.Commands.ResetAlarm"),
                  QStringLiteral("ResetAlarm"), QStringLiteral("Reset Alarm"),
                  QStringLiteral("ResetAlarm"));
}

void OpcUaServer::writeScalarString(const QString &nodeId, const QString &value)
{
    if (!m_server || nodeId.isEmpty())
        return;

    UA_NodeId uaNodeId = makeStringNodeId(m_namespaceIndex, nodeId);
    const QByteArray utf8 = value.toUtf8();
    UA_String uaValue = UA_STRING_ALLOC(utf8.constData());
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Variant_setScalarCopy(&variant, &uaValue, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(m_server, uaNodeId, variant);
    UA_NodeId_clear(&uaNodeId);
    UA_String_clear(&uaValue);
    UA_Variant_clear(&variant);
}

void OpcUaServer::writeScalarBool(const QString &nodeId, bool value)
{
    if (!m_server || nodeId.isEmpty())
        return;

    UA_NodeId uaNodeId = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_Boolean uaValue = value;
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Variant_setScalarCopy(&variant, &uaValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(m_server, uaNodeId, variant);
    UA_NodeId_clear(&uaNodeId);
    UA_Variant_clear(&variant);
}

void OpcUaServer::writeScalarInt32(const QString &nodeId, qint32 value)
{
    if (!m_server || nodeId.isEmpty())
        return;

    UA_NodeId uaNodeId = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_Int32 uaValue = value;
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Variant_setScalarCopy(&variant, &uaValue, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(m_server, uaNodeId, variant);
    UA_NodeId_clear(&uaNodeId);
    UA_Variant_clear(&variant);
}

void OpcUaServer::writeScalarDouble(const QString &nodeId, double value)
{
    if (!m_server || nodeId.isEmpty())
        return;

    UA_NodeId uaNodeId = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_Double uaValue = value;
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Variant_setScalarCopy(&variant, &uaValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(m_server, uaNodeId, variant);
    UA_NodeId_clear(&uaNodeId);
    UA_Variant_clear(&variant);
}

UA_NodeId OpcUaServer::addObjectNode(const UA_NodeId &parentNode, const QString &nodeId,
                                     const QString &browseName, const QString &displayName)
{
    UA_NodeId requested = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_QualifiedName qn = makeQualifiedName(m_namespaceIndex, browseName);
    UA_ObjectAttributes attrs = UA_ObjectAttributes_default;
    attrs.displayName = makeText(displayName);

    UA_NodeId result = UA_NODEID_NULL;
    const UA_StatusCode status = UA_Server_addObjectNode(m_server,
                                                         requested,
                                                         parentNode,
                                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                         qn,
                                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                         attrs,
                                                         nullptr,
                                                         &result);

    UA_NodeId_clear(&requested);
    UA_QualifiedName_clear(&qn);
    UA_ObjectAttributes_clear(&attrs);

    if (status != UA_STATUSCODE_GOOD)
        log(QStringLiteral("OPC UA: addObjectNode failed for %1 status=0x%2")
                .arg(nodeId, QString::number(status, 16)));

    return result;
}

UA_NodeId OpcUaServer::addStringVariableNode(const UA_NodeId &parentNode, const QString &nodeId,
                                             const QString &browseName, const QString &displayName,
                                             const QString &value)
{
    UA_NodeId requested = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_QualifiedName qn = makeQualifiedName(m_namespaceIndex, browseName);
    UA_VariableAttributes attrs = UA_VariableAttributes_default;
    attrs.displayName = makeText(displayName);
    attrs.accessLevel = UA_ACCESSLEVELMASK_READ;
    attrs.userAccessLevel = UA_ACCESSLEVELMASK_READ;

    const QByteArray utf8 = value.toUtf8();
    UA_String uaValue = UA_STRING_ALLOC(utf8.constData());
    UA_Variant_setScalarCopy(&attrs.value, &uaValue, &UA_TYPES[UA_TYPES_STRING]);
    attrs.dataType = UA_TYPES[UA_TYPES_STRING].typeId;

    UA_NodeId result = UA_NODEID_NULL;
    const UA_StatusCode status = UA_Server_addVariableNode(m_server, requested, parentNode,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                           qn,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                           attrs, nullptr, &result);

    UA_String_clear(&uaValue);
    UA_NodeId_clear(&requested);
    UA_QualifiedName_clear(&qn);
    UA_VariableAttributes_clear(&attrs);

    if (status != UA_STATUSCODE_GOOD)
        log(QStringLiteral("OPC UA: addStringVariableNode failed for %1 status=0x%2")
                .arg(nodeId, QString::number(status, 16)));

    return result;
}

UA_NodeId OpcUaServer::addBoolVariableNode(const UA_NodeId &parentNode, const QString &nodeId,
                                           const QString &browseName, const QString &displayName,
                                           bool value)
{
    UA_NodeId requested = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_QualifiedName qn = makeQualifiedName(m_namespaceIndex, browseName);
    UA_VariableAttributes attrs = UA_VariableAttributes_default;
    attrs.displayName = makeText(displayName);
    attrs.accessLevel = UA_ACCESSLEVELMASK_READ;
    attrs.userAccessLevel = UA_ACCESSLEVELMASK_READ;

    UA_Boolean uaValue = value;
    UA_Variant_setScalarCopy(&attrs.value, &uaValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attrs.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;

    UA_NodeId result = UA_NODEID_NULL;
    const UA_StatusCode status = UA_Server_addVariableNode(m_server, requested, parentNode,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                           qn,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                           attrs, nullptr, &result);

    UA_NodeId_clear(&requested);
    UA_QualifiedName_clear(&qn);
    UA_VariableAttributes_clear(&attrs);

    if (status != UA_STATUSCODE_GOOD)
        log(QStringLiteral("OPC UA: addBoolVariableNode failed for %1 status=0x%2")
                .arg(nodeId, QString::number(status, 16)));

    return result;
}

UA_NodeId OpcUaServer::addInt32VariableNode(const UA_NodeId &parentNode, const QString &nodeId,
                                            const QString &browseName, const QString &displayName,
                                            qint32 value)
{
    UA_NodeId requested = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_QualifiedName qn = makeQualifiedName(m_namespaceIndex, browseName);
    UA_VariableAttributes attrs = UA_VariableAttributes_default;
    attrs.displayName = makeText(displayName);
    attrs.accessLevel = UA_ACCESSLEVELMASK_READ;
    attrs.userAccessLevel = UA_ACCESSLEVELMASK_READ;

    UA_Int32 uaValue = value;
    UA_Variant_setScalarCopy(&attrs.value, &uaValue, &UA_TYPES[UA_TYPES_INT32]);
    attrs.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    UA_NodeId result = UA_NODEID_NULL;
    const UA_StatusCode status = UA_Server_addVariableNode(m_server, requested, parentNode,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                           qn,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                           attrs, nullptr, &result);

    UA_NodeId_clear(&requested);
    UA_QualifiedName_clear(&qn);
    UA_VariableAttributes_clear(&attrs);

    if (status != UA_STATUSCODE_GOOD)
        log(QStringLiteral("OPC UA: addInt32VariableNode failed for %1 status=0x%2")
                .arg(nodeId, QString::number(status, 16)));

    return result;
}

UA_NodeId OpcUaServer::addDoubleVariableNode(const UA_NodeId &parentNode, const QString &nodeId,
                                             const QString &browseName, const QString &displayName,
                                             double value)
{
    UA_NodeId requested = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_QualifiedName qn = makeQualifiedName(m_namespaceIndex, browseName);
    UA_VariableAttributes attrs = UA_VariableAttributes_default;
    attrs.displayName = makeText(displayName);
    attrs.accessLevel = UA_ACCESSLEVELMASK_READ;
    attrs.userAccessLevel = UA_ACCESSLEVELMASK_READ;

    UA_Double uaValue = value;
    UA_Variant_setScalarCopy(&attrs.value, &uaValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    attrs.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;

    UA_NodeId result = UA_NODEID_NULL;
    const UA_StatusCode status = UA_Server_addVariableNode(m_server, requested, parentNode,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                           qn,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                           attrs, nullptr, &result);

    UA_NodeId_clear(&requested);
    UA_QualifiedName_clear(&qn);
    UA_VariableAttributes_clear(&attrs);

    if (status != UA_STATUSCODE_GOOD)
        log(QStringLiteral("OPC UA: addDoubleVariableNode failed for %1 status=0x%2")
                .arg(nodeId, QString::number(status, 16)));

    return result;
}

UA_NodeId OpcUaServer::addMethodNode(const UA_NodeId &parentNode, const QString &nodeId,
                                     const QString &browseName, const QString &displayName,
                                     const QString &commandName)
{
    UA_NodeId requested = makeStringNodeId(m_namespaceIndex, nodeId);
    UA_QualifiedName qn = makeQualifiedName(m_namespaceIndex, browseName);
    UA_MethodAttributes attrs = UA_MethodAttributes_default;
    attrs.displayName = makeText(displayName);
    attrs.description = makeText(commandName);
    attrs.executable = true;
    attrs.userExecutable = true;

    UA_NodeId result = UA_NODEID_NULL;
    UA_StatusCode status = UA_Server_addMethodNode(
        m_server,
        requested,
        parentNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        qn,
        attrs,
        &OpcUaServer::invokeMethodCallback,
        0, nullptr,
        0, nullptr,
        this,
        &result);

    UA_NodeId_clear(&requested);
    UA_QualifiedName_clear(&qn);
    UA_MethodAttributes_clear(&attrs);

    if (status == UA_STATUSCODE_GOOD) {
    } else {
        log(QStringLiteral("OPC UA: addMethodNode failed for %1 status=0x%2")
                .arg(nodeId, QString::number(status, 16)));
    }

    return result;
}

UA_StatusCode OpcUaServer::invokeMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                                const UA_NodeId *methodId, void *methodContext,
                                                const UA_NodeId *objectId, void *objectContext,
                                                size_t inputSize, const UA_Variant *input,
                                                size_t outputSize, UA_Variant *output)
{
    Q_UNUSED(server);
    Q_UNUSED(sessionId);
    Q_UNUSED(sessionContext);
    Q_UNUSED(objectId);
    Q_UNUSED(objectContext);
    Q_UNUSED(inputSize);
    Q_UNUSED(input);
    Q_UNUSED(outputSize);
    Q_UNUSED(output);

    OpcUaServer *self = static_cast<OpcUaServer *>(methodContext);
    if (!self)
        return UA_STATUSCODE_BADINTERNALERROR;

    QString commandName;
    if (methodId && methodId->identifierType == UA_NODEIDTYPE_STRING) {
        const QString full = QString::fromUtf8(reinterpret_cast<const char *>(methodId->identifier.string.data),
                                               int(methodId->identifier.string.length));
        commandName = full.section('.', -1);
    }

    if (commandName.isEmpty())
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const bool ok = self->invokeCommand(commandName);
    if (ok) {
        self->refreshModel();
        return UA_STATUSCODE_GOOD;
    }

    return UA_STATUSCODE_BADUSERACCESSDENIED;
}
#endif

QVariantMap OpcUaServer::buildCabinetNode() const
{
    QVariantMap node;
    if (!m_backend)
        return node;

    const DeviceSnapshot snapshot = m_backend->snapshot();

    node[QStringLiteral("state")] = cabinetStateText(snapshot.cabinetMode);
    node[QStringLiteral("cabinetMode")] = snapshot.cabinetMode;
    node[QStringLiteral("systemState")] = snapshot.systemState;
    node[QStringLiteral("busConnected")] = snapshot.busConnected;
    node[QStringLiteral("doorOpen")] = snapshot.doorOpen;
    node[QStringLiteral("fireActive")] = snapshot.fireActive;
    node[QStringLiteral("programFireActive")] = snapshot.programFireActive;
    node[QStringLiteral("fireInput")] = snapshot.fireInput;
    node[QStringLiteral("testRunning")] = snapshot.testRunning;
    node[QStringLiteral("relayStateKnown")] = snapshot.relayStateKnown;
    node[QStringLiteral("relayMismatch")] = snapshot.relayMismatch;
    node[QStringLiteral("batteryChargePercent")] = snapshot.battery.chargePercent;
    return node;
}

QVariantList OpcUaServer::buildLineNodes() const
{
    QVariantList lines;
    if (!m_backend)
        return lines;

    const DeviceSnapshot snapshot = m_backend->snapshot();
    int index = 1;

    for (const LineSnapshot &line : snapshot.lines) {
        QVariantMap node;
        node[QStringLiteral("index")] = index;
        node[QStringLiteral("name")] = line.description;
        node[QStringLiteral("mode")] = line.mode;
        node[QStringLiteral("modeText")] = lineModeText(line.mode);
        node[QStringLiteral("status")] = line.status;
        node[QStringLiteral("statusText")] = lineStatusText(line.status);
        node[QStringLiteral("lineState")] = line.lineState;
        node[QStringLiteral("controlledPower")] = line.mpower;
        lines.push_back(node);
        ++index;
    }

    return lines;
}

QVariantList OpcUaServer::buildCommandNodes() const
{
    QVariantList commands;
    for (const QString &name : commandNames()) {
        QVariantMap cmd;
        cmd[QStringLiteral("name")] = name;
        cmd[QStringLiteral("callable")] = true;
        commands.push_back(cmd);
    }
    return commands;
}

QString OpcUaServer::cabinetStateText(int cabinetMode) const
{
    switch (cabinetMode) {
    case 1: return QStringLiteral("Пожар");
    case 2: return QStringLiteral("Тест");
    case 3: return QStringLiteral("Авария");
    default: return QStringLiteral("Работа");
    }
}

QString OpcUaServer::lineModeText(int mode) const
{
    switch (mode) {
    case 0: return QStringLiteral("Постоянная");
    case 1: return QStringLiteral("Непостоянная");
    case 2: return QStringLiteral("Линия отключена");
    default: return QStringLiteral("Неизвестно");
    }
}

QString OpcUaServer::lineStatusText(int status) const
{
    switch (status) {
    case 0: return QStringLiteral("ОК");
    case 1: return QStringLiteral("Авария");
    case 2: return QStringLiteral("Тест");
    case 3: return QStringLiteral("Резерв");
    default: return QStringLiteral("Неизвестно");
    }
}
