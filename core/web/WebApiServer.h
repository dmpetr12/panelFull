#pragma once

#include <QObject>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QDir>

class BackendController;

class WebApiServer : public QObject
{
    Q_OBJECT
public:
    explicit WebApiServer(BackendController *backend, QObject *parent = nullptr);

    bool start(quint16 port, const QString &webRoot);
    void stop();

private:
    void setupRoutes();
    QJsonObject mapToJsonObject(const QVariantMap &map) const;
    QJsonArray listToJsonArray(const QVariantList &list) const;
    QHttpServerResponse jsonOk(const QJsonObject &obj) const;
    QHttpServerResponse jsonError(const QString &message, int status = 400) const;
    bool checkAuth(const QHttpServerRequest &request) const;
    QString extractBearerToken(const QHttpServerRequest &request) const;

private:
    BackendController *m_backend = nullptr;
    QHttpServer m_server;
    QString m_webRoot;
    quint16 m_port = 0;

    // На старте — одна простая сессия в памяти
    QString m_authToken;
};