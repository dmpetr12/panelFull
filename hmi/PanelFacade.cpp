#include "PanelFacade.h"

#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QVariantList>
#include <QVariantMap>

PanelFacade::PanelFacade(QObject *parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout,
            this, &PanelFacade::pollState);

    m_pollTimer.start(1000);
    pollState();
}

bool PanelFacade::connected() const
{
    return m_connected;
}

QJsonObject PanelFacade::state() const
{
    return m_state;
}

QJsonObject PanelFacade::battery() const
{
    return m_state.value("battery").toObject();
}

bool PanelFacade::fireActive() const
{
    return state().value("fireActive").toBool(false);
}

bool PanelFacade::stopActive() const
{
    return state().value("stopActive").toBool(false);
}

bool PanelFacade::dispatcherActive() const
{
    return state().value("dispatcherActive").toBool(false);
}

bool PanelFacade::busConnected() const
{
    return state().value("busConnected").toBool(false);
}

bool PanelFacade::testRunning() const
{
    return state().value("testRunning").toBool(false);
}

double PanelFacade::batteryChargePercent() const
{
    return battery().value("chargePercent").toDouble(0.0);
}

bool PanelFacade::batteryLow() const
{
    return battery().value("batteryLow").toBool(false);
}

bool PanelFacade::batteryFault() const
{
    return battery().value("batteryFault").toBool(false);
}

bool PanelFacade::onBattery() const
{
    return battery().value("onBattery").toBool(false);
}

bool PanelFacade::charging() const
{
    return battery().value("charging").toBool(false);
}

int PanelFacade::systemState() const
{
    return state().value("systemState").toInt(0);
}

int PanelFacade::lineCount() const
{
    return state().value("lines").toArray().size();
}

bool PanelFacade::fireTestActive() const
{
    return state().value("fireTestActive").toBool(false);
}

bool PanelFacade::fireInput() const
{
    return state().value("fireInput").toBool(false);
}

int PanelFacade::fireTestLine() const
{
    return state().value("fireTestLine").toInt(-1);
}

bool PanelFacade::singleLineTestActive() const
{
    return state().value("singleLineTestActive").toBool(false);
}

int PanelFacade::singleLineTestLine() const
{
    return state().value("singleLineTestLine").toInt(-1);
}

QVariantList PanelFacade::lines() const
{
    QVariantList result;
    const QJsonArray arr = state().value("lines").toArray();

    for (const QJsonValue &v : arr)
        result.append(v.toObject().toVariantMap());

    return result;
}

double PanelFacade::inletPValue() const
{
    return state().value("inletP").toDouble(0.0);
}

double PanelFacade::inletUValue() const
{
    return state().value("inletU").toDouble(0.0);
}

double PanelFacade::inletIValue() const
{
    return state().value("inletI").toDouble(0.0);
}

double PanelFacade::inletFValue() const
{
    return state().value("inletF").toDouble(0.0);
}

double PanelFacade::temperatureValue() const
{
    return state().value("temperature").toDouble(0.0);
}

double PanelFacade::testPValue() const
{
    return state().value("testP").toDouble(0.0);
}

double PanelFacade::testUValue() const
{
    return state().value("testU").toDouble(0.0);
}

double PanelFacade::testIValue() const
{
    return state().value("testI").toDouble(0.0);
}


bool PanelFacade::sendCommand(const QJsonObject &req, QJsonObject *resp)
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName);

    if (!socket.waitForConnected(300)) {
       //qWarning() << "Backend not available";
        m_connected = false;
        emit changed();
        return false;
    }

    socket.write(QJsonDocument(req).toJson(QJsonDocument::Compact));

    if (!socket.waitForBytesWritten(300))
        return false;

    if (!socket.waitForReadyRead(500))
        return false;

    const QByteArray raw = socket.readAll();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    if (resp)
        *resp = doc.object();

    return doc.object().value("ok").toBool(false);
}

void PanelFacade::pollState()
{
    QJsonObject resp;
    const bool ok = sendCommand({{"cmd", "getState"}}, &resp);

    if (!ok) {
        if (m_connected) {
            m_connected = false;
            emit changed();
        }
        return;
    }

    m_connected = true;
    m_state = resp.value("state").toObject();
    emit changed();
}

void PanelFacade::refresh()
{
    pollState();
}

bool PanelFacade::startFunctionalTest()
{
    return sendCommand({{"cmd", "startFunctionalTest"}});
}

bool PanelFacade::startDurationTest()
{
    return sendCommand({{"cmd", "startDurationTest"}});
}

bool PanelFacade::stopCurrentTest()
{
    return sendCommand({{"cmd", "stopCurrentTest"}});
}

bool PanelFacade::setForcedFire(bool on)
{
    return sendCommand({
        {"cmd", "setForcedFire"},
        {"on", on}
    });
}

bool PanelFacade::resetAlarm()
{
    return sendCommand({{"cmd", "resetAlarm"}});
}

bool PanelFacade::checkPassword(const QString &password)
{
    QJsonObject resp;
    const bool ok = sendCommand({
                                    {"cmd", "checkPassword"},
                                    {"password", password}
                                }, &resp);

    if (!ok)
        return false;

    return resp.value("match").toBool(false);
}

QVariantList PanelFacade::readLogs(int offset, int limit)
{
    QJsonObject resp;
    const bool ok = sendCommand({
                                    {"cmd", "readLogs"},
                                    {"offset", offset},
                                    {"limit", limit}
                                }, &resp);

    QVariantList result;
    if (!ok)
        return result;

    const QJsonArray arr = resp.value("lines").toArray();
    for (const QJsonValue &v : arr)
        result.append(v.toString());

    return result;
}

QString PanelFacade::exportLogsToUsb()
{
    QJsonObject resp;
    const bool ok = sendCommand({
                                    {"cmd", "exportLogsToUsb"}
                                }, &resp);

    if (!ok)
        return QStringLiteral("Ошибка экспорта логов");

    return resp.value("message").toString(QStringLiteral("Операция завершена"));
}

QVariantMap PanelFacade::testSummary()
{
    QJsonObject resp;
    const bool ok = sendCommand({
                                    {"cmd", "testSummary"}
                                }, &resp);

    QVariantMap result;
    if (!ok)
        return result;

    const QJsonObject summary = resp.value("summary").toObject();
    result = summary.toVariantMap();

    return result;
}

bool PanelFacade::setSystemTime(qint64 msec)
{
    return sendCommand({
        {"cmd", "setSystemTime"},
        {"time", msec}
    });
}

QVariantMap PanelFacade::lineAt(int index) const
{
    QJsonObject resp;

    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(300))
        return {};

    QJsonObject req{
        {"cmd", "lineAt"},
        {"index", index}
    };

    socket.write(QJsonDocument(req).toJson(QJsonDocument::Compact));

    if (!socket.waitForBytesWritten(300))
        return {};

    if (!socket.waitForReadyRead(500))
        return {};

    const QByteArray raw = socket.readAll();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};

    resp = doc.object();
    if (!resp.value("ok").toBool(false))
        return {};

    return resp.value("line").toObject().toVariantMap();
}


bool PanelFacade::updateLine(int index, const QVariantMap &lineData)
{
    return sendCommand({
        {"cmd", "updateLine"},
        {"index", index},
        {"lineData", QJsonObject::fromVariantMap(lineData)}
    });
}

bool PanelFacade::saveLines()
{
    return sendCommand({
        {"cmd", "saveLines"}
    });
}

bool PanelFacade::startLineTest(int index, int durationSec)
{
    return sendCommand({
        {"cmd", "startLineTest"},
        {"index", index},
        {"durationSec", durationSec}
    });
}

bool PanelFacade::stopLineTest(int index)
{
    return sendCommand({
        {"cmd", "stopLineTest"},
        {"index", index}
    });
}

bool PanelFacade::applyLineModes()
{
    return sendCommand({
        {"cmd", "applyLineModes"}
    });
}

bool PanelFacade::changePassword(const QString &newPassword)
{
    return sendCommand({
        {"cmd", "changePassword"},
        {"password", newPassword}
    });
}

QVariantList PanelFacade::getAllTests()
{
    QJsonObject resp;
    const bool ok = sendCommand({
                                    {"cmd", "getAllTests"}
                                }, &resp);

    if (!ok)
        return {};

    return resp.value("tests").toArray().toVariantList();
}

bool PanelFacade::addTest(const QVariantMap &data)
{
    return sendCommand({
        {"cmd", "addTest"},
        {"data", QJsonObject::fromVariantMap(data)}
    });
}

bool PanelFacade::removeTest(int index)
{
    return sendCommand({
        {"cmd", "removeTest"},
        {"index", index}
    });
}

bool PanelFacade::updateTestProperty(int index, const QString &key, const QVariant &value)
{
    return sendCommand({
        {"cmd", "updateTestProperty"},
        {"index", index},
        {"key", key},
        {"value", QJsonValue::fromVariant(value)}
    });
}

bool PanelFacade::writeLog(const QString &msg)
{
    return sendCommand({
        {"cmd", "writeLog"},
        {"message", msg}
    });
}


