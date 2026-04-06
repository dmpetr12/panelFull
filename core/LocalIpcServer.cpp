#include "LocalIpcServer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>

#include "BackendController.h"
#include "devicesnapshot_json.h"
#include "logger.h"


LocalIpcServer::LocalIpcServer(BackendController *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
{
    connect(&m_server, &QLocalServer::newConnection,
            this, &LocalIpcServer::onNewConnection);
    if (m_backend) {
        m_lastUiEvent = QJsonObject::fromVariantMap(m_backend->currentUiEvent());

        connect(m_backend, &BackendController::currentUiEventChanged,
                this,
                [this](const QString &code,
                       const QString &title,
                       const QString &text,
                       bool active)
                {
                    if (!m_backend)
                        return;

                    m_lastUiEvent = QJsonObject::fromVariantMap(m_backend->currentUiEvent());

                    // запасной вариант, если currentUiEvent() почему-то пустой
                    if (m_lastUiEvent.isEmpty()) {
                        QJsonObject ev;
                        ev["code"] = code;
                        ev["title"] = title;
                        ev["text"] = text;
                        ev["active"] = active;
                        m_lastUiEvent = ev;
                    }
                });
    }
}

bool LocalIpcServer::start(const QString &serverName)
{
    QLocalServer::removeServer(serverName);

    if (!m_server.listen(serverName)) {
        log(QString("IPC listen failed: %1").arg(m_server.errorString()));
        return false;
    }

    log(QString("IPC server started: %1").arg(serverName));
    return true;
}

void LocalIpcServer::stop()
{
    m_server.close();
}

void LocalIpcServer::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QLocalSocket *socket = m_server.nextPendingConnection();
        if (!socket)
            continue;

        connect(socket, &QLocalSocket::readyRead,
                this, &LocalIpcServer::onReadyRead);

        connect(socket, &QLocalSocket::disconnected,
                socket, &QObject::deleteLater);
    }
}

void LocalIpcServer::onReadyRead()
{
    auto *socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket)
        return;

    const QByteArray data = socket->readAll();
    processMessage(socket, data);
}

void LocalIpcServer::processMessage(QLocalSocket *socket, const QByteArray &data)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        sendJson(socket, {
                             {"ok", false},
                             {"error", "invalid json"}
                         });
        return;
    }

    const QJsonObject req = doc.object();
    const QString cmd = req.value("cmd").toString();

    if (cmd == "getState") {
        QJsonObject resp;
        resp["ok"] = true;
        resp["state"] = deviceSnapshotToJson(m_backend->snapshot());
        resp["uiEvent"] = m_lastUiEvent;

        sendJson(socket, resp);
        return;
    }

    if (cmd == "startFunctionalTest") {
        sendJson(socket, {{"ok", m_backend->startFunctionalTest()}});
        return;
    }

    if (cmd == "startDurationTest") {
        sendJson(socket, {{"ok", m_backend->startDurationTest()}});
        return;
    }

    if (cmd == "stopCurrentTest") {
        sendJson(socket, {{"ok", m_backend->stopCurrentTest()}});
        return;
    }

    if (cmd == "setProgramFire") {
        const bool on = req.value("on").toBool(false);
        sendJson(socket, {{"ok", m_backend->setForcedFire(on)}});
        return;
    }

    if (cmd == "resetAlarm") {
        sendJson(socket, {{"ok", m_backend->resetAlarm()}});
        return;
    }

    if (cmd == "checkPassword") {
        const QString password = req.value("password").toString();
        sendJson(socket, {
                             {"ok", true},
                             {"match", m_backend->checkPassword(password)}
                         });
        return;
    }

    if (cmd == "readLogs") {
        const int offset = req.value("offset").toInt(0);
        const int limit = req.value("limit").toInt(200);

        QStringList lines = m_backend->readLogs(offset, limit);

        QJsonArray arr;
        for (const QString &s : lines)
            arr.append(s);

        sendJson(socket, {
                             {"ok", true},
                             {"lines", arr}
                         });
        return;
    }

    if (cmd == "exportLogsToUsb") {
        const QString msg = m_backend->exportLogsToUsb();

        sendJson(socket, {
                             {"ok", true},
                             {"message", msg}
                         });
        return;
    }

    if (cmd == "testSummary") {
        const QVariantMap summary = m_backend->testSummary();

        sendJson(socket, {
                             {"ok", true},
                             {"summary", QJsonObject::fromVariantMap(summary)}
                         });
        return;
    }

    if (cmd == "setSystemTime") {

        qint64 ms = req.value("time").toVariant().toLongLong();

        bool ok = m_backend->setSystemTime(ms);

        sendJson(socket, {
                             {"ok", ok}
                         });

        return;
    }

    if (cmd == "lineAt") {
        const int index = req.value("index").toInt(-1);

        sendJson(socket, {
                             {"ok", true},
                             {"line", QJsonObject::fromVariantMap(m_backend->lineAt(index))}
                         });
        return;
    }

    if (cmd == "updateLine") {
        const int index = req.value("index").toInt(-1);
        const QVariantMap lineData = req.value("lineData").toObject().toVariantMap();

        const bool ok = m_backend->updateLine(index, lineData);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    if (cmd == "saveLines") {
        const bool ok = m_backend->saveLines();

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    if (cmd == "startLineTest") {
        const int index = req.value("index").toInt(-1);
        const int durationSec = req.value("durationSec").toInt(0);

        const bool ok = m_backend->startLineTest(index, durationSec);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    if (cmd == "stopLineTest") {
        const int index = req.value("index").toInt(-1);

        const bool ok = m_backend->stopLineTest(index);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    else if (cmd == "calcAllLinesTestDurationSec") {
        QJsonObject resp;
        resp["ok"] = true;
        resp["durationSec"] = m_backend->calcAllLinesTestDurationSec();
        sendJson(socket, resp);
        return;
    }

    if (cmd == "applyLineModes") {
        const bool ok = m_backend->applyLineModes();

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    if (cmd == "changePassword") {
        const QString password = req.value("password").toString();
        const bool ok = m_backend->changePassword(password);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }
    if (cmd == "getAllTests") {
        const QVariantList tests = m_backend->getAllTests();

        sendJson(socket, {
                             {"ok", true},
                             {"tests", QJsonArray::fromVariantList(tests)}
                         });
        return;
    }

    if (cmd == "addTest") {
        const QVariantMap data = req.value("data").toObject().toVariantMap();
        const bool ok = m_backend->addTest(data);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    if (cmd == "removeTest") {
        const int index = req.value("index").toInt(-1);
        const bool ok = m_backend->removeTest(index);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    if (cmd == "updateTestProperty") {
        const int index = req.value("index").toInt(-1);
        const QString key = req.value("key").toString();
        const QVariant value = req.value("value").toVariant();

        const bool ok = m_backend->updateTestProperty(index, key, value);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }
    if (cmd == "updateWeekDays") {
        const int index = req.value("index").toInt(-1);

        QStringList days;
        const QJsonArray arr = req.value("days").toArray();
        for (const QJsonValue &v : arr)
            days << v.toString();

        const bool ok = m_backend->updateWeekDays(index, days);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }

    if (cmd == "writeLog") {
        const QString message = req.value("message").toString();
        const bool ok = m_backend->writeLog(message);

        sendJson(socket, {
                             {"ok", ok}
                         });
        return;
    }


    sendJson(socket, {
                         {"ok", false},
                         {"error", "unknown command"}
                     });

}

void LocalIpcServer::sendJson(QLocalSocket *socket, const QJsonObject &obj)
{
    const QByteArray out = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    socket->write(out);
    socket->flush();
}
