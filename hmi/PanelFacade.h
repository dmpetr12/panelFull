#pragma once

#include <QObject>
#include <QJsonObject>
#include <QTimer>

class QLocalSocket;

class PanelFacade : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ connected NOTIFY changed)

    Q_PROPERTY(bool fireActive READ fireActive NOTIFY changed)
    Q_PROPERTY(bool forcedFireActive READ forcedFireActive NOTIFY changed)
    Q_PROPERTY(bool fireInput READ fireInput NOTIFY changed)
    Q_PROPERTY(bool stopActive READ stopActive NOTIFY changed)
    Q_PROPERTY(bool dispatcherActive READ dispatcherActive NOTIFY changed)
    Q_PROPERTY(bool busConnected READ busConnected NOTIFY changed)
    Q_PROPERTY(bool testRunning READ testRunning NOTIFY changed)

    Q_PROPERTY(double batteryChargePercent READ batteryChargePercent NOTIFY changed)
    Q_PROPERTY(bool batteryLow READ batteryLow NOTIFY changed)
    Q_PROPERTY(bool batteryFault READ batteryFault NOTIFY changed)
    Q_PROPERTY(bool onBattery READ onBattery NOTIFY changed)
    Q_PROPERTY(bool charging READ charging NOTIFY changed)

    Q_PROPERTY(int systemState READ systemState NOTIFY changed)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY changed)

    Q_PROPERTY(bool fireTestActive READ fireTestActive NOTIFY changed)
    Q_PROPERTY(int fireTestLine READ fireTestLine NOTIFY changed)
    Q_PROPERTY(bool singleLineTestActive READ singleLineTestActive NOTIFY changed)
    Q_PROPERTY(int singleLineTestLine READ singleLineTestLine NOTIFY changed)

    Q_PROPERTY(QVariantList lines READ lines NOTIFY changed)
    Q_PROPERTY(double inletPValue READ inletPValue NOTIFY changed)
    Q_PROPERTY(double inletUValue READ inletUValue NOTIFY changed)
    Q_PROPERTY(double inletIValue READ inletIValue NOTIFY changed)
    Q_PROPERTY(double inletFValue READ inletFValue NOTIFY changed)
    Q_PROPERTY(double temperatureValue READ temperatureValue NOTIFY changed)

    Q_PROPERTY(double testPValue READ testPValue NOTIFY changed)
    Q_PROPERTY(double testUValue READ testUValue NOTIFY changed)
    Q_PROPERTY(double testIValue READ testIValue NOTIFY changed)
    Q_PROPERTY(bool temperatureAvailable READ temperatureAvailable NOTIFY changed)
    Q_PROPERTY(bool inletUAvailable READ inletUAvailable NOTIFY changed)
    Q_PROPERTY(bool inletIAvailable READ inletIAvailable NOTIFY changed)
    Q_PROPERTY(bool inletPAvailable READ inletPAvailable NOTIFY changed)
    Q_PROPERTY(bool inletFAvailable READ inletFAvailable NOTIFY changed)

    Q_PROPERTY(bool testUAvailable READ testUAvailable NOTIFY changed)
    Q_PROPERTY(bool testIAvailable READ testIAvailable NOTIFY changed)
    Q_PROPERTY(bool testPAvailable READ testPAvailable NOTIFY changed)

    Q_PROPERTY(QString logLevel READ logLevel NOTIFY changed)

public:
    explicit PanelFacade(QObject *parent = nullptr);

    bool connected() const;

    bool fireActive() const;
    bool forcedFireActive() const;
    bool fireInput() const;
    bool stopActive() const;
    bool dispatcherActive() const;
    bool busConnected() const;
    bool testRunning() const;

    double batteryChargePercent() const;
    bool batteryLow() const;
    bool batteryFault() const;
    bool onBattery() const;
    bool charging() const;
    bool inletUAvailable() const;
    bool inletIAvailable() const;
    bool inletPAvailable() const;
    bool inletFAvailable() const;

    bool testUAvailable() const;
    bool testIAvailable() const;
    bool testPAvailable() const;
    bool temperatureAvailable() const;

    int systemState() const;
    int lineCount() const;

    bool fireTestActive() const;
    int fireTestLine() const;
    bool singleLineTestActive() const;
    int singleLineTestLine() const;
    QVariantList lines() const;
    double temperatureValue() const;
    double inletPValue() const;
    double inletUValue() const;
    double inletIValue() const;
    double inletFValue() const;
    QString logLevel() const;

    Q_INVOKABLE bool startFunctionalTest();
    Q_INVOKABLE bool startDurationTest();
    Q_INVOKABLE bool stopCurrentTest();
    Q_INVOKABLE bool setForcedFire(bool on);
    Q_INVOKABLE bool resetAlarm();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool checkPassword(const QString &password);

    Q_INVOKABLE QVariantList readLogs(int offset, int limit);
    Q_INVOKABLE QString exportLogsToUsb();
    Q_INVOKABLE QVariantMap testSummary();
    Q_INVOKABLE bool setSystemTime(qint64 msec);
    Q_INVOKABLE bool changePassword(const QString &newPassword);
    Q_INVOKABLE int calcAllLinesTestDurationSec();

    double testPValue() const;
    double testUValue() const;
    double testIValue() const;

    Q_INVOKABLE QVariantMap lineAt(int index) const;
    Q_INVOKABLE bool updateLine(int index, const QVariantMap &lineData);
    Q_INVOKABLE bool saveLines();
    Q_INVOKABLE bool startLineTest(int index, int durationSec);
    Q_INVOKABLE bool stopLineTest(int index);
    Q_INVOKABLE bool applyLineModes();

    Q_INVOKABLE QVariantList getAllTests();
    Q_INVOKABLE bool addTest(const QVariantMap &data);
    Q_INVOKABLE bool removeTest(int index);
    Q_INVOKABLE bool updateTestProperty(int index, const QString &key, const QVariant &value);
    Q_INVOKABLE bool writeLog(const QString &msg);

signals:
    void changed();

private slots:
    void pollState();

private:
    bool sendCommand(const QJsonObject &req, QJsonObject *resp = nullptr);
    QJsonObject state() const;
    QJsonObject battery() const;

private:
    QString m_serverName = "emergency_panel_backend";
    QTimer m_pollTimer;
    bool m_connected = false;
    QJsonObject m_state;
};
