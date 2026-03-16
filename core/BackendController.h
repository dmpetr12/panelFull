#pragma once

#include <QObject>
#include <QString>
#include "DeviceSnapshot.h"

class AppConfig;
class LinesModel;
class ModbusBus;
class UsbSerialWatcher;
class LineIoManager;
class TestController;
class ScheduleManager;
class MaintenanceChecker;
class PasswordManager;
class ValueProvider;
class BatteryController;
class ModbusRtuSlave;

class BackendController : public QObject
{
    Q_OBJECT

public:
    explicit BackendController(QObject *parent = nullptr);
    ~BackendController() override;

    bool start();
    void stop();
    bool startFunctionalTest();
    bool startDurationTest();
    bool stopCurrentTest();
    bool setForcedFire(bool on);
    bool resetAlarm();
    bool checkPassword(const QString &password) const;

    QStringList readLogs(int offset, int limit) const;
    QString exportLogsToUsb();
    QVariantMap testSummary() const;
    bool setSystemTime(qint64 msec);

    QVariantMap lineAt(int index) const;
    bool updateLine(int index, const QVariantMap &lineData);
    bool saveLines();
    bool startLineTest(int index, int durationSec);
    bool stopLineTest(int index);
    bool applyLineModes();
    bool changePassword(const QString &newPassword);

    QVariantList getAllTests() const;
    bool addTest(const QVariantMap &data);
    bool removeTest(int index);
    bool updateTestProperty(int index, const QString &key, const QVariant &value);
    bool writeLog(const QString &msg);


    AppConfig* config() const;
    LinesModel* lines() const;
    ModbusBus* bus() const;
    LineIoManager* lineIoManager() const;
    TestController* testController() const;
    ScheduleManager* scheduleManager() const;
    MaintenanceChecker* maintenanceChecker() const;
    PasswordManager* passwordManager() const;
    BatteryController* batteryController() const;
    DeviceSnapshot snapshot() const;

    ValueProvider* inletU() const;
    ValueProvider* inletI() const;
    ValueProvider* inletP() const;
    ValueProvider* inletF() const;
    ValueProvider* testU() const;
    ValueProvider* testI() const;
    ValueProvider* testP() const;
    ValueProvider* temperature() const;

signals:
    void started();
    void stopped();
    void logMessage(const QString &msg);
    void stateChanged();

private:
    void createObjects();
    void setupConfig();
    void setupLines();
    void setupBus();
    void setupSerialWatcher();
    void setupLineIo();
    void setupTesting();
    void setupMaintenance();
    void setupSchedules();
    void setupBattery();
    void setupConnections();
    void applyInitialState();

private:
    AppConfig *m_config = nullptr;
    LinesModel *m_lines = nullptr;
    ModbusBus *m_bus = nullptr;
    UsbSerialWatcher *m_serialWatcher = nullptr;
    LineIoManager *m_lineIoManager = nullptr;
    TestController *m_testController = nullptr;
    ScheduleManager *m_scheduleManager = nullptr;
    MaintenanceChecker *m_maintenanceChecker = nullptr;
    PasswordManager *m_passwordManager = nullptr;
    BatteryController *m_batteryController = nullptr;

    ValueProvider *m_inletU = nullptr;
    ValueProvider *m_inletI = nullptr;
    ValueProvider *m_inletP = nullptr;
    ValueProvider *m_inletF = nullptr;
    ValueProvider *m_testU = nullptr;
    ValueProvider *m_testI = nullptr;
    ValueProvider *m_testP = nullptr;
    ValueProvider *m_temperature = nullptr;

    QString m_linesFile = "lines.json";
    bool m_started = false;
    ModbusRtuSlave *m_modbusSlave = nullptr;// Modbus RTU 9600 8P1
};
