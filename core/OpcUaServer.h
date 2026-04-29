#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QStringList>

#ifdef PANEL_HAS_OPEN62541_SERVER
#include <open62541.h>
#endif

class BackendController;
class AppConfig;

class OpcUaServer : public QObject
{
    Q_OBJECT

public:
    explicit OpcUaServer(BackendController *backend, QObject *parent = nullptr);

    bool start(const AppConfig *config);
    void stop();

    bool enabled() const;
    bool running() const;
    QString endpoint() const;
    QString bind() const;
    int port() const;

    QVariantMap namespaceSnapshot() const;
    QStringList commandNames() const;
    bool invokeCommand(const QString &commandName);
    void refreshModel();

signals:
    void started();
    void stopped();
    void modelChanged();

private:
#ifdef PANEL_HAS_OPEN62541_SERVER
    bool initServer();
    void shutdownServer();
    void processServerEvents();
    void createAddressSpace();
    void writeScalarString(const QString &nodeId, const QString &value);
    void writeScalarBool(const QString &nodeId, bool value);
    void writeScalarInt32(const QString &nodeId, qint32 value);
    void writeScalarDouble(const QString &nodeId, double value);
    UA_NodeId addObjectNode(const UA_NodeId &parentNode, const QString &nodeId, const QString &browseName, const QString &displayName);
    UA_NodeId addStringVariableNode(const UA_NodeId &parentNode, const QString &nodeId, const QString &browseName, const QString &displayName, const QString &value);
    UA_NodeId addBoolVariableNode(const UA_NodeId &parentNode, const QString &nodeId, const QString &browseName, const QString &displayName, bool value);
    UA_NodeId addInt32VariableNode(const UA_NodeId &parentNode, const QString &nodeId, const QString &browseName, const QString &displayName, qint32 value);
    UA_NodeId addDoubleVariableNode(const UA_NodeId &parentNode, const QString &nodeId, const QString &browseName, const QString &displayName, double value);
    UA_NodeId addMethodNode(const UA_NodeId &parentNode, const QString &nodeId, const QString &browseName, const QString &displayName, const QString &commandName);

    static UA_StatusCode invokeMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                              const UA_NodeId *methodId, void *methodContext,
                                              const UA_NodeId *objectId, void *objectContext,
                                              size_t inputSize, const UA_Variant *input,
                                              size_t outputSize, UA_Variant *output);
#endif
    QVariantMap buildCabinetNode() const;
    QVariantList buildLineNodes() const;
    QVariantList buildCommandNodes() const;

    QString cabinetStateText(int cabinetMode) const;
    QString lineModeText(int mode) const;
    QString lineStatusText(int status) const;

private:
    BackendController *m_backend = nullptr;
    bool m_enabled = false;
    bool m_running = false;
    QString m_endpoint;
    QString m_bind;
    int m_port = 0;

#ifdef PANEL_HAS_OPEN62541_SERVER
    UA_Server *m_server = nullptr;
    QTimer *m_iterateTimer = nullptr;
    quint16 m_namespaceIndex = 1;
    UA_NodeId m_rootNodeId = UA_NODEID_NULL;
    UA_NodeId m_cabinetNodeId = UA_NODEID_NULL;
    UA_NodeId m_linesNodeId = UA_NODEID_NULL;
    UA_NodeId m_commandsNodeId = UA_NODEID_NULL;
    QVariantMap m_cabinetVariableNodes;
    QVariantList m_lineVariableNodes;
#endif
};
