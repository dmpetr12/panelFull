#include "BackendController.h"

#include <QDebug>
#include <QCoreApplication>
#include <QSerialPortInfo>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDir>
#include <QStorageInfo>
#include <QDateTime>
#include <algorithm>

#include "AppConfig.h"
#include "PasswordManager.h"
#include "ValueProvider.h"
#include "linesmodel.h"
#include "modbusbus.h"
#include "UsbSerialWatcher.h"
#include "LineIoManager.h"
#include "TestController.h"
#include "schedulemanager.h"
#include "MaintenanceChecker.h"
#include "BatteryController.h"
#include "line.h"
#include "modbus/ModbusRtuSlave.h"
//#include "logger.h"

// id серийников
constexpr quint16 VID_FT232  = 0x0403;
constexpr quint16 PID_FT232  = 0x6001;

constexpr quint16 VID_PL2303 = 0x067B;
constexpr quint16 PID_PL2303 = 0x2303;

constexpr quint16 VID_CH340  = 0x1A86;
constexpr quint16 PID_CH340  = 0x7523;

constexpr quint16 RelayBaseAddress  = 1;
static constexpr int kAlarmRelayIndex = 1;

BackendController::BackendController(QObject *parent)
    : QObject(parent)
{
}

BackendController::~BackendController()
{
    stop();
}

bool BackendController::start()
{
    if (m_started)
        return true;

    createObjects();
    setupConfig();
    setupLines();
    setupBus();
    setupSerialWatcher();
    setupLineIo();
    setupTesting();
    setupMaintenance();
    setupSchedules();
    setupBattery();
    setupConnections();
    applyInitialState();
    if (m_modbusSlave) {
        m_modbusSlave->start("/dev/ttyUSB0", 115200, 1);
    }

    m_started = true;
    emit started();
    emit logMessage("Backend started");
    return true;
}

void BackendController::stop()
{
    if (!m_started)
        return;

    if (m_serialWatcher)
        m_serialWatcher->stop();

    if (m_bus)
        m_bus->disconnectDevice();
    if (m_modbusSlave)
        m_modbusSlave->stop();

    m_started = false;
    emit stopped();
    emit logMessage("Backend stopped");
}

void BackendController::createObjects()
{
    if (!m_config)            m_config = new AppConfig(this);
    if (!m_lines)             m_lines = new LinesModel(this);
    if (!m_bus)               m_bus = new ModbusBus(this);
    if (!m_lineIoManager)     m_lineIoManager = new LineIoManager(this);
    if (!m_testController)    m_testController = new TestController(this);
    if (!m_scheduleManager)   m_scheduleManager = new ScheduleManager(this);
    if (!m_maintenanceChecker)m_maintenanceChecker = new MaintenanceChecker(this);
    if (!m_passwordManager)   m_passwordManager = new PasswordManager(this);
    if (!m_batteryController) m_batteryController = new BatteryController(this);

    if (!m_inletU)        m_inletU = new ValueProvider(this);
    if (!m_inletI)        m_inletI = new ValueProvider(this);
    if (!m_inletP)        m_inletP = new ValueProvider(this);
    if (!m_inletF)        m_inletF = new ValueProvider(this);
    if (!m_testU)         m_testU = new ValueProvider(this);
    if (!m_testI)         m_testI = new ValueProvider(this);
    if (!m_testP)         m_testP = new ValueProvider(this);
    if (!m_temperature)   m_temperature = new ValueProvider(this);
    if (!m_modbusSlave)   m_modbusSlave = new ModbusRtuSlave(this, this);
}


void BackendController::setupConfig()
{
    if (!m_config->load("config.json")) {
        qWarning() << "config.json not found/invalid, using defaults";
    }

    qWarning() << "CONFIG:"
               << "numLines=" << m_config->numLines()
               << "maxRelayModules=" << m_config->maxRelayModules()
               << "timeAuto=" << m_config->timeAuto()
               << "timeTest=" << m_config->timeTest();
}

void BackendController::setupLines()
{
    if (!m_lines->loadFromFile(m_linesFile)) {
        for (int i = 1; i <= m_config->numLines(); ++i) {
            m_lines->addLine(
                QString("Линия %1").arg(i),
                0.0, 0.0, 0.0, 0.0, 5.0,
                Line::NotUsed,
                Line::Undefined,
                Line::Off
                );
        }
    }

    for (int row = 0; row < m_lines->rowCount(); ++row) {
        Line *ln = m_lines->line(row);
        if (!ln)
            continue;

        if (ln->mode() == Line::Constant)
            ln->setLineState(Line::On);
        else
            ln->setLineState(Line::Off);
    }
}

void BackendController::setupBus()
{
    QObject::connect(m_bus, &ModbusBus::errorOccurred,
                     this, [this](const QString &msg) {
                         qWarning() << "RelaysBus error:" << msg;
                         emit logMessage(QStringLiteral("RelaysBus error: %1").arg(msg));
                     });

    QObject::connect(m_bus, &ModbusBus::temperatureUpdated,
                     this, [this](double t) {
                         m_temperature->setValue(t);
                         emit stateChanged();
                     });

    QObject::connect(m_bus, &ModbusBus::inletMeterUpdated,
                     this, [this](double U, double I, double P, double F) {
                         m_inletU->setValue(U);
                         m_inletI->setValue(I);
                         m_inletP->setValue(P);
                         m_inletF->setValue(F);
                         emit stateChanged();
                     });

    QObject::connect(m_bus, &ModbusBus::testMeterUpdated,
                     this, [this](double U, double I, double P) {
                         m_testU->setValue(U);
                         m_testI->setValue(I);
                         m_testP->setValue(P);
                         emit stateChanged();
                     });

    QObject::connect(m_bus, &ModbusBus::connectedChanged,
                     this, [this](bool connected) {
                         if (!connected) {
                             m_inletU->reset();
                             m_inletI->reset();
                             m_inletP->reset();
                             m_inletF->reset();
                             m_testU->reset();
                             m_testI->reset();
                             m_testP->reset();
                             m_temperature->reset();
                         }
                         emit stateChanged();
                     });

    m_bus->setBaudRate(9600);
    m_bus->setTimeoutMs(250);
    m_bus->setRetries(0);

    m_bus->setRelayModuleCount(m_config->maxRelayModules());
    m_bus->setRelayBaseAddress(RelayBaseAddress);

    m_bus->setInletMeterAddr(10);
    m_bus->setTestMeterAddr(11);
    m_bus->setSht20Addr(12);

    m_bus->setSht20TempInputReg(0x0001);
    m_bus->setSht20TempScale(0.1);
}

void BackendController::setupSerialWatcher()
{
    const QString sp = m_config->serialPort();

    auto setupRelaysBus = [this](const QString &portName) {
        m_bus->disconnectDevice();
        m_bus->setPortName(portName);
        m_bus->connectDevice();
    };

    if (sp.startsWith("/dev/") || sp.startsWith("COM")) {
        log(QString("[BACKEND] Using fixed serial port: %1").arg(sp));
        setupRelaysBus(sp);
        return;
    }

    quint16 vid = 0;
    quint16 pid = 0;

    if (sp == "FT232") {
        vid = VID_FT232;
        pid = PID_FT232;
    } else if (sp == "PL2303") {
        vid = VID_PL2303;
        pid = PID_PL2303;
    } else if (sp == "CH340") {
        vid = VID_CH340;
        pid = PID_CH340;
    } else {
        log(QString("[BACKEND] Unknown serialPort in config: '%1'").arg(sp));
        return;
    }

    m_serialWatcher = new UsbSerialWatcher(vid, pid, 2000, this);

    QObject::connect(m_serialWatcher, &UsbSerialWatcher::portAvailable,
                     this, [this, setupRelaysBus](const QString &portName) {
                         log(QString("[BACKEND] Adapter available on %1").arg(portName));
                         setupRelaysBus(portName);
                     });

    QObject::connect(m_serialWatcher, &UsbSerialWatcher::portLost,
                     this, [this]() {
                         log("[BACKEND] Adapter lost, disconnecting bus");
                         m_bus->disconnectDevice();
                     });

    m_serialWatcher->start();
}

DeviceSnapshot BackendController::snapshot() const
{
    DeviceSnapshot s;

    if (m_inletU) s.inletU = m_inletU->value();
    if (m_inletI) s.inletI = m_inletI->value();
    if (m_inletP) s.inletP = m_inletP->value();
    if (m_inletF) s.inletF = m_inletF->value();

    if (m_testU) s.testU = m_testU->value();
    if (m_testI) s.testI = m_testI->value();
    if (m_testP) s.testP = m_testP->value();

    if (m_temperature) s.temperature = m_temperature->value();

    if (m_bus) {
        s.busConnected = m_bus->isConnected();
    }

    if (m_lineIoManager) {
        s.fireActive = m_lineIoManager->fireActive();
        s.fireInput = m_lineIoManager->fireInput();
        s.stopActive = m_lineIoManager->stopActive();
        s.dispatcherActive = m_lineIoManager->dispatcherActive();

        s.fireTestActive = m_lineIoManager->fireTestActive();
        s.fireTestLine = m_lineIoManager->fireTestLine();

        s.singleLineTestActive = m_lineIoManager->singleLineTestActive();
        s.singleLineTestLine = m_lineIoManager->singleLineTestLine();
    }

    if (m_lines) {
        s.systemState = m_lines->systemState();

        for (int row = 0; row < m_lines->rowCount(); ++row) {
            Line *ln = m_lines->line(row);
            if (!ln)
                continue;

            LineSnapshot ls;
            ls.description = ln->description();
            ls.mpower = ln->mpower();
            ls.power = ln->power();
            ls.current = ln->current();
            ls.voltage = ln->voltage();
            ls.tolerance = ln->tolerance();
            ls.mode = static_cast<int>(ln->mode());
            ls.status = static_cast<int>(ln->status());
            ls.lineState = static_cast<int>(ln->lineState());
            ls.lastMeasuredTest = ln->lastMeasuredTest();

            s.lines.push_back(ls);
        }
    }

    if (m_testController) {
        s.testRunning = m_testController->testActive();
    }

    if (m_batteryController) {
        s.battery.voltage = m_batteryController->voltage();
        s.battery.current = m_batteryController->current();
        s.battery.chargePercent = m_batteryController->chargePercent();
        s.battery.charging = m_batteryController->charging();
        s.battery.batteryLow = m_batteryController->batteryLow();
        s.battery.batteryFault = m_batteryController->batteryFault();
        s.battery.onBattery = m_batteryController->onBattery();
    }

    return s;
}

void BackendController::setupLineIo()
{
    m_lineIoManager->bind(m_bus, m_lines, m_config->numLines());

    QObject::connect(m_bus, &ModbusBus::inputsUpdated,
                     m_lineIoManager, &LineIoManager::onInputsUpdated);

    QObject::connect(m_bus, &ModbusBus::connectedChanged,
                     this, [this](bool connected) {
                         if (connected) {
                             m_bus->setModeNormal();
                             m_bus->setAllRelaysOffFast();
                             m_lineIoManager->forceApplyAll();

                             const bool emergency = (m_lines->systemState() == 1);
                             m_bus->setRelayGlobal(kAlarmRelayIndex, emergency);
                         }
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::fireChanged,
                     this, [this](bool active) {
                         log(QString("FIRE changed: %1").arg(active));
                         emit stateChanged();
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::stopChanged,
                     this, [this](bool active) {
                         log(QString("STOP changed: %1").arg(active));
                         emit stateChanged();
                     });
}

void BackendController::setupTesting()
{
    m_testController->setModel(m_lines);
    m_testController->setIoManager(m_lineIoManager);
    m_testController->setBus(m_bus);
    m_testController->setLinesSavePath(m_linesFile);
    m_testController->setLongTestThresholdMinutes(m_config->timeAuto() * 60);
    m_testController->setLongTestToleranceBonus(10.0);

    QObject::connect(m_lineIoManager, &LineIoManager::emergencyStop,
                     m_testController, [this]() {
                         m_testController->stopTest(-1);
                     });

    QObject::connect(m_bus, &ModbusBus::testMeterUpdated,
                     this, [this](double U, double I, double P) {
                         const int idx = m_testController->measuredLine();
                         if (idx < 0 || idx >= m_lines->rowCount())
                             return;

                         Line *ln = m_lines->line(idx);
                         if (!ln)
                             return;

                         ln->setVoltage(U);
                         ln->setCurrent(I);
                         ln->setPower(P);

                         emit stateChanged();
                     });
}

void BackendController::setupMaintenance()
{
    m_maintenanceChecker->bind(m_lines, m_testController);
    m_maintenanceChecker->startDaily(7, 0);
}
void BackendController::setupSchedules()
{
    QObject::connect(m_scheduleManager, &ScheduleManager::logMessage,
                     this, &BackendController::logMessage);

    m_scheduleManager->loadFromFile("schedule.json");

    QObject::connect(m_scheduleManager, &ScheduleManager::startTestRequested,
                     this, [this](const QString &testType) {
                         if (testType == QStringLiteral("Функциональный")) {
                             startFunctionalTest();
                         } else if (testType == QStringLiteral("На время")) {
                             startDurationTest();
                         } else {
                             emit logMessage(QStringLiteral("Неизвестный тип теста: ") + testType);
                         }
                     });
}

void BackendController::setupBattery()
{
    // Пока каркас.
    // Когда появятся реальные данные батареи, сюда повесим сигналы от ModbusBus
    // или отдельного battery device.

    QObject::connect(m_temperature, &ValueProvider::valueChanged,
                     this, [this]() {
                         // Временный пример логики:
                         // высокая температура может потом участвовать в battery fault
                         emit stateChanged();
                     });
}

void BackendController::setupConnections()
{
    QObject::connect(this, &BackendController::logMessage,
                     this, [](const QString &msg) {
                         log(msg);
                     });

    QObject::connect(m_lines, &LinesModel::dataChanged,
                     this, &BackendController::stateChanged);

    QObject::connect(m_lines, &LinesModel::systemStateChanged,
                     this, [this]() {
                         const bool emergency = (m_lines->systemState() == 1);
                         m_bus->setRelayGlobal(kAlarmRelayIndex, emergency);
                         m_lineIoManager->setAlarmLamp(emergency);
                         emit stateChanged();
                     });

    QObject::connect(m_batteryController, &BatteryController::stateChanged,
                     this, &BackendController::stateChanged);
}

void BackendController::applyInitialState()
{
    const bool emergency = (m_lines->systemState() == 1);
    m_bus->setRelayGlobal(kAlarmRelayIndex, emergency);
    m_lineIoManager->setAlarmLamp(emergency);
}

bool BackendController::startFunctionalTest()
{
    if (!m_testController || !m_config)
        return false;

    const int sec = qMax(1, int(m_config->timeTest()) * 60);
    m_testController->startTestScheduleAll(sec);

    emit logMessage(QStringLiteral("Запущен функциональный тест"));
    emit stateChanged();
    return true;
}

bool BackendController::startDurationTest()
{
    if (!m_testController || !m_config)
        return false;

    const int sec = qMax(1, int(m_config->timeAuto()) * 3600);
    m_testController->startTestScheduleAll(sec);

    emit logMessage(QStringLiteral("Запущен тест на время"));
    emit stateChanged();
    return true;
}

bool BackendController::stopCurrentTest()
{
    if (!m_testController)
        return false;

    m_testController->stopTest(-1);

    emit logMessage(QStringLiteral("Текущий тест остановлен"));
    emit stateChanged();
    return true;
}

bool BackendController::setForcedFire(bool on)
{
    if (!m_lineIoManager)
        return false;

    m_lineIoManager->setForcedFire(on);

    emit logMessage(QStringLiteral("Ручной пожарный режим: %1")
                        .arg(on ? QStringLiteral("ВКЛ") : QStringLiteral("ВЫКЛ")));
    emit stateChanged();
    return true;
}

bool BackendController::checkPassword(const QString &password) const
{
    return m_passwordManager && m_passwordManager->password() == password;
}

bool BackendController::resetAlarm()
{
    if (!m_lines)
        return false;

    bool changed = false;

    for (int row = 0; row < m_lines->rowCount(); ++row) {
        Line *ln = m_lines->line(row);
        if (!ln)
            continue;

        if (ln->status() == Line::Failure) {
            ln->setStatus(Line::Undefined);
            changed = true;
        }
    }

    if (m_lineIoManager)
        m_lineIoManager->setAlarmLamp(false);

    if (m_bus)
        m_bus->setRelayGlobal(kAlarmRelayIndex, false);

    emit logMessage(QStringLiteral("Сброс аварии"));

    if (changed)
        emit stateChanged();

    return true;
}

QStringList BackendController::readLogs(int offset, int limit) const
{
    QStringList result;

    if (limit <= 0 || offset < 0)
        return result;

    QFile f("logs/system_log.txt");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    QString all = in.readAll();
    f.close();

    QStringList lines = all.split('\n', Qt::SkipEmptyParts);

    std::reverse(lines.begin(), lines.end());

    if (offset < lines.size()) {
        int end = qMin(offset + limit, lines.size());
        for (int i = offset; i < end; ++i)
            result << lines[i];
    }

    limit = limit - result.size();
    offset = offset - lines.size();

    if (limit <= 0)
        return result;

    if (offset < 0)
        offset = 0;

    QFile f1("logs/system_log_1.txt");
    if (!f1.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in1(&f1);
    in1.setEncoding(QStringConverter::Utf8);
    QString all1 = in1.readAll();
    f1.close();

    QStringList lines1 = all1.split('\n', Qt::SkipEmptyParts);

    std::reverse(lines1.begin(), lines1.end());

    if (offset < lines1.size()) {
        int end = qMin(offset + limit, lines1.size());
        for (int i = offset; i < end; ++i)
            result << lines1[i];
    }

    return result;
}


QString BackendController::exportLogsToUsb()
{
    const QStringList logFiles = {
        QStringLiteral("logs/system_log.txt"),
        QStringLiteral("logs/system_log_1.txt")
    };

    QStorageInfo target;
    const auto volumes = QStorageInfo::mountedVolumes();

    for (const QStorageInfo &volume : volumes) {

        if (!volume.isValid() || !volume.isReady())
            continue;

        if (volume.isReadOnly())
            continue;

        const QString rootPath = volume.rootPath();

#ifdef Q_OS_LINUX
        if (!rootPath.startsWith("/media")
            && !rootPath.startsWith("/run/media")
            && !rootPath.startsWith("/mnt"))
            continue;
#endif

#ifdef Q_OS_WIN
        if (rootPath.toLower() == "c:/" ||
            rootPath.toLower() == "d:/")
            continue;
#endif

        if (rootPath == "/")
            continue;

        target = volume;
        break;
    }

    if (!target.isValid())
        return "Флешка не найдена";

    const QString dirName =
        "logs_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    const QString destDirPath =
        QDir(target.rootPath()).filePath(dirName);

    if (!QDir().mkpath(destDirPath))
        return QString("Не удалось создать каталог: %1").arg(destDirPath);

    int okCount = 0;
    int failCount = 0;
    int missingCount = 0;

    for (const QString &fileName : logFiles) {

        QFile src(fileName);

        if (!src.exists()) {
            ++missingCount;
            continue;
        }

        const QString destPath =
            QDir(destDirPath).filePath(fileName);

        if (QFile::exists(destPath))
            QFile::remove(destPath);

        if (src.copy(destPath))
            ++okCount;
        else
            ++failCount;
    }

    return QString("Скопировано: %1, ошибок: %2, нет файла: %3, папка: %4")
        .arg(okCount)
        .arg(failCount)
        .arg(missingCount)
        .arg(destDirPath);
}

QVariantMap BackendController::testSummary() const
{
    if (!m_maintenanceChecker) {
        QVariantMap result;
        result["longTestResult"] = QStringLiteral("—");
        result["longTestDate"] = QStringLiteral("—");
        result["lines"] = QVariantList{};
        return result;
    }

    return m_maintenanceChecker->testSummary();
}


bool BackendController::setSystemTime(qint64 msec)
{
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(msec);

    QString cmd = "date -s \"" + dt.toString("yyyy-MM-dd hh:mm:ss") + "\"";

    int res = system(cmd.toUtf8().constData());

    return res == 0;
}

QVariantMap BackendController::lineAt(int index) const
{
    QVariantMap map;

    if (!m_lines)
        return map;

    Line *ln = m_lines->line(index);
    if (!ln)
        return map;

    map["description"] = ln->description();
    map["power"] = ln->power();
    map["mpower"] = ln->mpower();
    map["current"] = ln->current();
    map["voltage"] = ln->voltage();
    map["tolerance"] = ln->tolerance();
    map["mode"] = static_cast<int>(ln->mode());
    map["status"] = static_cast<int>(ln->status());
    map["lineState"] = static_cast<int>(ln->lineState());
    map["lastMeasuredTest"] = ln->lastMeasuredTest();

    return map;
}

bool BackendController::updateLine(int index, const QVariantMap &lineData)
{
    if (!m_lines)
        return false;

    Line *ln = m_lines->line(index);
    if (!ln)
        return false;

    if (lineData.contains("description"))
        ln->setDescription(lineData.value("description").toString());

    if (lineData.contains("mpower"))
        ln->setmPower(lineData.value("mpower").toDouble());

    if (lineData.contains("tolerance"))
        ln->setTolerance(lineData.value("tolerance").toDouble());

    if (lineData.contains("mode"))
        ln->setMode(static_cast<Line::Mode>(lineData.value("mode").toInt()));

    return true;
}

bool BackendController::saveLines()
{
    if (!m_lines)
        return false;

    return m_lines->saveToFile(m_linesFile);
}

bool BackendController::startLineTest(int index, int durationSec)
{
    if (!m_testController)
        return false;

    m_testController->startTest(index, durationSec);
    return true;
}

bool BackendController::stopLineTest(int index)
{
    if (!m_testController)
        return false;

    m_testController->stopTest(index);
    return true;
}

bool BackendController::applyLineModes()
{
    if (!m_lineIoManager)
        return false;

    m_lineIoManager->forceApplyAll();
    return true;
}

bool BackendController::changePassword(const QString &newPassword)
{
    if (!m_passwordManager)
        return false;

    m_passwordManager->setPassword(newPassword);
    return true;
}

QVariantList BackendController::getAllTests() const
{
    if (!m_scheduleManager)
        return {};

    return m_scheduleManager->getAllTests();
}

bool BackendController::addTest(const QVariantMap &data)
{
    if (!m_scheduleManager)
        return false;

    m_scheduleManager->addTest(data);
    return true;
}

bool BackendController::removeTest(int index)
{
    if (!m_scheduleManager)
        return false;

    m_scheduleManager->removeTest(index);
    return true;
}

bool BackendController::updateTestProperty(int index, const QString &key, const QVariant &value)
{
    if (!m_scheduleManager)
        return false;

    m_scheduleManager->updateTestProperty(index, key, value);
    return true;
}

bool BackendController::writeLog(const QString &msg)
{
    log(msg);
    return true;
}


AppConfig* BackendController::config() const
{
    return m_config;
}

LinesModel* BackendController::lines() const
{
    return m_lines;
}

ModbusBus* BackendController::bus() const
{
    return m_bus;
}

LineIoManager* BackendController::lineIoManager() const
{
    return m_lineIoManager;
}

TestController* BackendController::testController() const
{
    return m_testController;
}

ScheduleManager* BackendController::scheduleManager() const
{
    return m_scheduleManager;
}

MaintenanceChecker* BackendController::maintenanceChecker() const
{
    return m_maintenanceChecker;
}

PasswordManager* BackendController::passwordManager() const
{
    return m_passwordManager;
}

BatteryController* BackendController::batteryController() const
{
    return m_batteryController;
}

ValueProvider* BackendController::inletU() const
{
    return m_inletU;
}

ValueProvider* BackendController::inletI() const
{
    return m_inletI;
}

ValueProvider* BackendController::inletP() const
{
    return m_inletP;
}

ValueProvider* BackendController::inletF() const
{
    return m_inletF;
}

ValueProvider* BackendController::testU() const
{
    return m_testU;
}

ValueProvider* BackendController::testI() const
{
    return m_testI;
}

ValueProvider* BackendController::testP() const
{
    return m_testP;
}

ValueProvider* BackendController::temperature() const
{
    return m_temperature;
}
