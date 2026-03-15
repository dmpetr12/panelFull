#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>

class BackendController;

class LocalIpcServer : public QObject
{
    Q_OBJECT

public:
    explicit LocalIpcServer(BackendController *backend, QObject *parent = nullptr);

    bool start(const QString &serverName = "emergency_panel_backend");
    void stop();

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void processMessage(QLocalSocket *socket, const QByteArray &data);
    void sendJson(QLocalSocket *socket, const QJsonObject &obj);

private:
    BackendController *m_backend = nullptr;
    QLocalServer m_server;
};
