#include "WebApiServer.h"
#include "../core/BackendController.h"
#include "../core/OpcUaServer.h"

#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QTcpServer>
#include <QMimeDatabase>
#include <QUrlQuery>
#include <QDebug>
#include "logger.h"

WebApiServer::WebApiServer(BackendController *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
{
    setupRoutes();
}

bool WebApiServer::start(quint16 port, const QString &webRoot)
{
    m_webRoot = webRoot;
    m_port = port;

    auto *tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        log(QStringLiteral( "WebApiServer listen failed: %1" )
                    .arg(tcpServer->errorString()));
        tcpServer->deleteLater();
        return false;
    }

    if (!m_server.bind(tcpServer)) {
        log(QStringLiteral( "QHttpServer bind failed"));
        tcpServer->deleteLater();
        return false;
    }

    log(QStringLiteral("WebApiServer started on port %1 webRoot %2")
                        .arg(port) .arg(webRoot));

    return true;
}

void WebApiServer::stop()
{
    // При bind(QTcpServer*) сервер живёт через QTcpServer child.
    // Для минимального каркаса отдельной stop-логики не нужно.
}

QJsonObject WebApiServer::mapToJsonObject(const QVariantMap &map) const
{
    return QJsonObject::fromVariantMap(map);
}

QJsonArray WebApiServer::listToJsonArray(const QVariantList &list) const
{
    return QJsonArray::fromVariantList(list);
}

QHttpServerResponse WebApiServer::jsonOk(const QJsonObject &obj) const
{
    return QHttpServerResponse(obj);
}

QHttpServerResponse WebApiServer::jsonError(const QString &message, int status) const
{
    QJsonObject obj;
    obj["ok"] = false;
    obj["error"] = message;

    return QHttpServerResponse(
        QJsonDocument(obj).toJson(QJsonDocument::Compact),
        "application/json",
        static_cast<QHttpServerResponder::StatusCode>(status)
        );
}

QString WebApiServer::extractBearerToken(const QHttpServerRequest &request) const
{
    const QByteArray auth = request.value("Authorization");
    const QByteArray prefix = "Bearer ";
    if (!auth.startsWith(prefix))
        return {};

    return QString::fromUtf8(auth.mid(prefix.size()));
}

bool WebApiServer::checkAuth(const QHttpServerRequest &request) const
{
    if (m_authToken.isEmpty())
        return false;

    return extractBearerToken(request) == m_authToken;
}

void WebApiServer::setupRoutes()
{
    // healthcheck
    m_server.route("/api/health", []() {
        return QHttpServerResponse(QJsonObject{
            {"ok", true}
        });
    });

    // status
    m_server.route("/api/status", [this]() {
        if (!m_backend)
            return jsonError("backend unavailable", 500);

        QJsonObject obj;
        obj["ok"] = true;
        obj["data"] = mapToJsonObject(m_backend->webState());
        return jsonOk(obj);
    });

    // opc ua skeleton model
    m_server.route("/api/opcua/model", [this](const QHttpServerRequest &request) {
        if (!checkAuth(request))
            return jsonError("unauthorized", 401);

        if (!m_backend || !m_backend->opcUaServer())
            return jsonError("opc ua unavailable", 500);

        QJsonObject obj;
        obj["ok"] = true;
        obj["data"] = mapToJsonObject(m_backend->opcUaServer()->namespaceSnapshot());
        return jsonOk(obj);
    });

    // all lines
    m_server.route("/api/lines", [this]() {
        if (!m_backend)
            return jsonError("backend unavailable", 500);

        QJsonObject obj;
        obj["ok"] = true;
        obj["data"] = listToJsonArray(m_backend->allLines());
        return jsonOk(obj);
    });

    // one line
    m_server.route("/api/lines/<arg>", [this](int index) {
        if (!m_backend)
            return jsonError("backend unavailable", 500);

        const QVariantMap line = m_backend->lineAt(index);
        if (line.isEmpty())
            return jsonError("line not found", 404);

        QJsonObject obj;
        obj["ok"] = true;
        obj["data"] = mapToJsonObject(line);
        obj["index"] = index;
        return jsonOk(obj);
    });

    // login
    m_server.route("/api/login", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!m_backend)
                           return jsonError("backend unavailable", 500);

                       const auto doc = QJsonDocument::fromJson(request.body());
                       if (!doc.isObject())
                           return jsonError("invalid json body", 400);

                       const QJsonObject body = doc.object();
                       const QString password = body.value("password").toString();

                       if (!m_backend->checkPassword(password))
                           return jsonError("invalid password", 401);

                       const QByteArray seed =
                           password.toUtf8() +
                           QByteArray::number(QDateTime::currentMSecsSinceEpoch());

                       m_authToken = QString::fromUtf8(
                           QCryptographicHash::hash(seed, QCryptographicHash::Sha256).toHex()
                           );

                       QJsonObject obj;
                       obj["ok"] = true;
                       obj["token"] = m_authToken;
                       return jsonOk(obj);
                   });

    // start functional test
    m_server.route("/api/test/start-functional", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->startFunctionalTest();

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to start functional test", 500);
                   });

    // start duration test
    m_server.route("/api/test/start-duration", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->startDurationTest();

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to start duration test", 500);
                   });

    // stop current test
    m_server.route("/api/test/stop", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->stopCurrentTest();

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to stop test", 500);
                   });

    // fire on
    m_server.route("/api/fire/on", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->setForcedFire(true);

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to enable fire", 500);
                   });

    // fire off
    m_server.route("/api/fire/off", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->setForcedFire(false);

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to disable fire", 500);
                   });

    // alarm reset
    m_server.route("/api/alarm/reset", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->resetAlarm();

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to reset alarm", 500);
                   });

    // logs
    m_server.route("/api/logs", [this](const QHttpServerRequest &request) {
        if (!m_backend)
            return jsonError("backend unavailable", 500);

        const QUrlQuery q(request.url());
        const int offset = q.queryItemValue("offset").toInt();
        const int limit = q.queryItemValue("limit").toInt();

        const QStringList logs = m_backend->readLogs(offset, limit > 0 ? limit : 100);

        QJsonArray arr;
        for (const auto &line : logs)
            arr.push_back(line);

        QJsonObject obj;
        obj["ok"] = true;
        obj["data"] = arr;
        return jsonOk(obj);
    });

    m_server.route("/api/logs/files", [this](const QHttpServerRequest &request) {
        if (!checkAuth(request))
            return jsonError("unauthorized", 401);

        if (!m_backend)
            return jsonError("backend unavailable", 500);

        QJsonObject obj;
        obj["ok"] = true;
        obj["data"] = listToJsonArray(m_backend->logFilesInfo());
        return jsonOk(obj);
    });

    m_server.route("/api/logs/download-all", [this](const QHttpServerRequest &request) {
        if (!checkAuth(request))
            return jsonError("unauthorized", 401);

        if (!m_backend)
            return jsonError("backend unavailable", 500);

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            m_backend->readAllLogsText().toUtf8(),
            QHttpServerResponder::StatusCode::Ok
        );
    });

    // schedule
    m_server.route("/api/schedule", [this]() {
        if (!m_backend)
            return jsonError("backend unavailable", 500);

        QJsonObject obj;
        obj["ok"] = true;
        obj["data"] = listToJsonArray(m_backend->getAllTests());
        return jsonOk(obj);
    });

    m_server.route("/api/schedule/add", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const auto doc = QJsonDocument::fromJson(request.body());
                       if (!doc.isObject())
                           return jsonError("invalid json body", 400);

                       const bool ok = m_backend && m_backend->addTest(doc.object().toVariantMap());
                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to add schedule item", 500);
                   });

    m_server.route("/api/schedule/<arg>/update", QHttpServerRequest::Method::Post,
                   [this](int index, const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const auto doc = QJsonDocument::fromJson(request.body());
                       if (!doc.isObject())
                           return jsonError("invalid json body", 400);

                       const QVariantMap body = doc.object().toVariantMap();

                       bool ok = true;
                       for (auto it = body.constBegin(); it != body.constEnd(); ++it) {
                           if (it.key() == QStringLiteral("weekDays"))
                               continue;
                           ok = ok && m_backend->updateTestProperty(index, it.key(), it.value());
                       }

                       if (body.contains(QStringLiteral("weekDays"))) {
                           QStringList days;
                           const QVariant weekDaysValue = body.value(QStringLiteral("weekDays"));
                           if (weekDaysValue.canConvert<QStringList>()) {
                               days = weekDaysValue.toStringList();
                           } else {
                               const QVariantList rawDays = weekDaysValue.toList();
                               for (const QVariant &day : rawDays)
                                   days.append(day.toString());
                           }
                           ok = ok && m_backend->updateWeekDays(
                                           index,
                                           days
                                           );
                       }

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to update schedule item", 500);
                   });

    m_server.route("/api/schedule/<arg>/remove", QHttpServerRequest::Method::Post,
                   [this](int index, const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->removeTest(index);
                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to remove schedule item", 500);
                   });

    // system time
    m_server.route("/api/system/time", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const auto doc = QJsonDocument::fromJson(request.body());
                       if (!doc.isObject())
                           return jsonError("invalid json body", 400);

                       const qint64 msec = doc.object().value("msec").toVariant().toLongLong();
                       if (msec <= 0)
                           return jsonError("invalid msec", 400);

                       const bool ok = m_backend && m_backend->setSystemTime(msec);
                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to set system time", 500);
                   });

    // password
    m_server.route("/api/password/change", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const auto doc = QJsonDocument::fromJson(request.body());
                       if (!doc.isObject())
                           return jsonError("invalid json body", 400);

                       const QString newPassword = doc.object().value("newPassword").toString().trimmed();
                       if (newPassword.isEmpty())
                           return jsonError("empty password", 400);

                       const bool ok = m_backend && m_backend->changePassword(newPassword);
                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to change password", 500);
                   });

    // line update
    m_server.route("/api/lines/<arg>/update", QHttpServerRequest::Method::Post,
                   [this](int index, const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const auto doc = QJsonDocument::fromJson(request.body());
                       if (!doc.isObject())
                           return jsonError("invalid json body", 400);

                       const QVariantMap body = doc.object().toVariantMap();
                       bool ok = m_backend && m_backend->updateLine(index, body);
                       ok = ok && m_backend->saveLines();

                       if (body.contains(QStringLiteral("mode"))) {
                           ok = ok && m_backend->applyLineModes();
                       }

                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to update line", 500);
                   });

    m_server.route("/api/lines/<arg>/test/start", QHttpServerRequest::Method::Post,
                   [this](int index, const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       int durationSec = 60;
                       const auto doc = QJsonDocument::fromJson(request.body());
                       if (doc.isObject()) {
                           durationSec = qMax(1, doc.object().value("durationSec").toInt(durationSec));
                       }

                       const bool ok = m_backend && m_backend->startLineTest(index, durationSec);
                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to start line test", 500);
                   });

    m_server.route("/api/lines/<arg>/test/stop", QHttpServerRequest::Method::Post,
                   [this](int index, const QHttpServerRequest &request) {
                       if (!checkAuth(request))
                           return jsonError("unauthorized", 401);

                       const bool ok = m_backend && m_backend->stopLineTest(index);
                       QJsonObject obj;
                       obj["ok"] = ok;
                       return ok ? jsonOk(obj) : jsonError("failed to stop line test", 500);
                   });

    // static: /
    m_server.route("/", [this]() {
        QFile f(QDir(m_webRoot).filePath("index.html"));
        if (!f.open(QIODevice::ReadOnly))
            return QHttpServerResponse("index.html not found",
                                       QHttpServerResponder::StatusCode::NotFound);

        return QHttpServerResponse(
            "text/html; charset=utf-8",
            f.readAll(),
            QHttpServerResponder::StatusCode::Ok
            );
    });

    // static: /assets/<arg>
    m_server.route("/assets/<arg>", [this](const QString &fileName) {
        const QString fullPath = QDir(m_webRoot + "/assets").filePath(fileName);
        QFileInfo fi(fullPath);
        if (!fi.exists() || !fi.isFile())
            return QHttpServerResponse("not found",
                                       QHttpServerResponder::StatusCode::NotFound);

        QFile f(fullPath);
        if (!f.open(QIODevice::ReadOnly))
            return QHttpServerResponse("cannot open file",
                                       QHttpServerResponder::StatusCode::InternalServerError);

        QMimeDatabase db;
        const QMimeType mime = db.mimeTypeForFile(fi);
        return QHttpServerResponse(
            mime.name().toUtf8(),
            f.readAll(),
            QHttpServerResponder::StatusCode::Ok
            );
    });

}
