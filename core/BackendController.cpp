#include "BackendController.h"

#include <QDebug>
#include <QCoreApplication>
#include <QProcess>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDir>
#include <QSerialPort>
#include <QStorageInfo>
#include <QDateTime>
#include <algorithm>
#include <QFileInfo>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _WIN32
#include <time.h>
#include <errno.h>
#include <string.h>
#endif


#include "AppConfig.h"
#include "PasswordManager.h"
#include "ValueProvider.h"
#include "linesmodel.h"
#include "modbusbus.h"
#include "LineIoManager.h"
#include "TestController.h"
#include "schedulemanager.h"
#include "MaintenanceChecker.h"
#include "BatteryController.h"
#include "line.h"
#include "modbus/ModbusRtuSlave.h"
#include "modbus/ModbusTcpServer.h"
#include "logger.h"

constexpr quint16 RelayBaseAddress  = 1;

static QSerialPort::Parity toParity(const QString &s)
{
    const QString v = s.trimmed().toUpper();
    if (v == "E")
        return QSerialPort::EvenParity;
    if (v == "O")
        return QSerialPort::OddParity;
    return QSerialPort::NoParity;
}

static QSerialPort::DataBits toDataBits(int bits)
{
    switch (bits) {
    case 5: return QSerialPort::Data5;
    case 6: return QSerialPort::Data6;
    case 7: return QSerialPort::Data7;
    case 8:
    default:
        return QSerialPort::Data8;
    }
}

static QSerialPort::StopBits toStopBits(int bits)
{
    switch (bits) {
    case 2: return QSerialPort::TwoStop;
    case 1:
    default:
        return QSerialPort::OneStop;
    }
}


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
    if (m_modbusSlave && m_config->modbusRtuEnabled()) {
        const bool ok = m_modbusSlave->start(
            m_config->modbusRtuDevice(),
            m_config->modbusRtuBaudRate(),
            toParity(m_config->modbusRtuParity()),
            toDataBits(m_config->modbusRtuDataBits()),
            toStopBits(m_config->modbusRtuStopBits()),
            m_config->modbusRtuSlaveId()
            );

        if (!ok) {
            emit logMessage(QStringLiteral("Не удалось запустить Modbus RTU сервер"));
        } else {
            emit logMessage(QStringLiteral("Modbus RTU сервер запущен на %1")
                                .arg(m_config->modbusRtuDevice()));
        }
    }
    if (m_modbusTcpServer && m_config->modbusTcpEnabled()) {
        m_modbusTcpServer->setBackend(this);
        //const QString tcpAddr = m_config ? m_config->modbusTcpListenAddress() : QStringLiteral("0.0.0.0");
        const int tcpPort = m_config ? m_config->modbusTcpPort() : 502;
        const bool okTcp = m_modbusTcpServer->start(QStringLiteral("0.0.0.0"), tcpPort);
        if (!okTcp) {
            emit logMessage(QStringLiteral("Не удалось запустить Modbus TCP сервер"));
        }else {
            emit logMessage(QStringLiteral("Modbus TCP сервер запущен на %1")
                                .arg(tcpPort));
        }
    }
    if (m_config) {
        m_logLevel = m_config->logLevel();
        emit logMessage( QStringLiteral("Backend logLevel loaded: %1" ).arg(m_logLevel));
    } else {
        m_logLevel = "INFO"; // дефолт, если конфиг отсутствует
        emit logMessage( QStringLiteral("ackend config missing, default logLevel: %1" ).arg(m_logLevel));
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

    if (m_bus)
        m_bus->disconnectDevice();
    if (m_modbusSlave)
        m_modbusSlave->stop();
    if (m_modbusTcpServer)
        m_modbusTcpServer->stop();

    m_started = false;
    emit stopped();
    emit logMessage("Backend stopped");
}

void BackendController::createObjects()
{
    if (!m_config)            m_config = new AppConfig(this);
    if (!m_lines)             m_lines = new LinesModel(this);
    if (!m_bus)               m_bus = new ModbusBus(m_config, this);
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
    if (!m_modbusTcpServer) m_modbusTcpServer = new ModbusTcpServer(this);
}


void BackendController::setupConfig()
{
    if (!m_config->load("config.json")) {
        emit logMessage(QStringLiteral( "config.json not found/invalid, using defaults"));
    }

    emit logMessage(QStringLiteral("CONFIG: numLines=%1 maxRelayModules=%2 timeAuto=%3 timeTest=%4").
        arg(m_config->numLines()).arg( m_config->maxRelayModules()).
            arg(m_config->timeAuto()).arg(m_config->timeTest()));
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
    QObject::connect(m_bus, &ModbusBus::busOnline,
                     this, []() {
                         qWarning() << "RelaysBus: связь с шиной восстановлена";
                     });

    QObject::connect(m_bus, &ModbusBus::busOffline,
                     this, [](const QString &reason) {
                         if (reason.isEmpty())
                             qWarning() << "RelaysBus: шина недоступна";
                         else
                             qWarning() << "RelaysBus: шина недоступна:" << reason;
                     });

    QObject::connect(m_bus, &ModbusBus::deviceOffline,
                     this, [this](const QString &name, int address) {
                         emit logMessage(QStringLiteral("%1 (addr=%2): устройство недоступно")
                                             .arg(name)
                                             .arg(address));
                     });

    QObject::connect(m_bus, &ModbusBus::deviceOnline,
                     this, [this](const QString &name, int address) {
                         emit logMessage(QStringLiteral("%1 (addr=%2): связь восстановлена")
                                             .arg(name)
                                             .arg(address));
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
    const QString portName = m_config->serialPort();

    log(QString("[BACKEND] Using fixed internal RS485 port: %1").arg(portName));
    log(QString("[BACKEND] Internal RS485 canonical path: %1").arg(QFileInfo(portName).canonicalFilePath()));

    m_bus->disconnectDevice();
    m_bus->setPortName(portName);
    m_bus->connectDevice();
}

DeviceSnapshot BackendController::snapshot() const
{
    DeviceSnapshot s;

    if (m_inletU) {
        s.inletU = m_inletU->value();
        s.inletUAvailable = m_inletU->valid();
    }

    if (m_inletI) {
        s.inletI = m_inletI->value();
        s.inletIAvailable = m_inletI->valid();
    }

    if (m_inletP) {
        s.inletP = m_inletP->value();
        s.inletPAvailable = m_inletP->valid();
    }

    if (m_inletF) {
        s.inletF = m_inletF->value();
        s.inletFAvailable = m_inletF->valid();
    }

    if (m_testU) {
        s.testU = m_testU->value();
        s.testUAvailable = m_testU->valid();
    }

    if (m_testI) {
        s.testI = m_testI->value();
        s.testIAvailable = m_testI->valid();
    }

    if (m_testP) {
        s.testP = m_testP->value();
        s.testPAvailable = m_testP->valid();
    }

    if (m_temperature) {
        s.temperature = m_temperature->value();
        s.temperatureAvailable = m_temperature->valid();
    }

    if (m_bus) {
        s.busConnected = m_bus->isConnected();
    }

    if (m_lineIoManager) {
        s.fireActive = m_lineIoManager->fireActive();
        s.fireInput = m_lineIoManager->fireInput();
        s.programFireActive = m_lineIoManager->programFireActive();

        s.stepTestActive = m_lineIoManager->stepTestActive();
        s.stepTestLine = m_lineIoManager->stepTestLine();

        s.singleLineTestActive = m_lineIoManager->singleLineTestActive();
        s.singleLineTestLine = m_lineIoManager->singleLineTestLine();

        s.noMeasTestActive = m_lineIoManager->noMeasTestActive();
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

    if (m_config)
        s.logLevel = m_config->logLevel();
    else
        s.logLevel = "INFO";

    return s;
}

void BackendController::setupLineIo()
{
    m_lineIoManager->bind(m_bus, m_lines, m_config->numLines());

    QObject::connect(m_bus, &ModbusBus::inputsUpdated,
                     m_lineIoManager, &LineIoManager::onInputsUpdated);

    QObject::connect(m_bus, &ModbusBus::busOffline,
                     this, [this](const QString &) {
                         m_busWasOffline = true;
                     });

    QObject::connect(m_bus, &ModbusBus::busOnline,
                     this, [this]() {
                         // Первый старт — не считаем восстановлением
                         if (!m_busInitialized) {
                             m_busInitialized = true;
                             return;
                         }

                         // Если реального offline не было — ничего не делаем
                         if (!m_busWasOffline)
                             return;

                         m_busWasOffline = false;

                         m_bus->setModeNormal();
                         m_lineIoManager->forceApplyAll();

                         log(QStringLiteral("Связь с Modbus восстановлена, состояние реле пере-применено"));
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::fireChanged,
                     this, [this](bool active) {
                         log(QString("Пожар: %1").arg(active ? QStringLiteral("АКТИВЕН")
                                                             : QStringLiteral("СНЯТ")));
                         emit stateChanged();
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::programFireChanged,
                     this, [this](bool active) {
                         log(QString("Программный пожар: %1").arg(active ? QStringLiteral("ВКЛ")
                                                                         : QStringLiteral("ВЫКЛ")));
                         emit stateChanged();
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::singleLineTestActiveChanged,
                     this, [this](bool) {
                         emit stateChanged();
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::stepTestActiveChanged,
                     this, [this](bool) {
                         emit stateChanged();
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::singleLineTestLineChanged,
                     this, [this](int) {
                         emit stateChanged();
                     });

    QObject::connect(m_lineIoManager, &LineIoManager::stepTestLineChanged,
                     this, [this](int) {
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
                         m_lineIoManager->setAlarmLamp(emergency);
                         emit stateChanged();
                     });

    QObject::connect(m_batteryController, &BatteryController::stateChanged,
                     this, &BackendController::stateChanged);

    if (m_modbusTcpServer) {
        QObject::connect(m_modbusTcpServer, &ModbusTcpServer::logMessage,
                         this, &BackendController::logMessage);
    }

    connect(m_modbusSlave, &ModbusRtuSlave::errorOccurred,
            this, [this](const QString &msg) {
                emit logMessage(msg);
            });

    connect(m_modbusSlave, &ModbusRtuSlave::serverOffline,
            this, [this](const QString &reason) {
                emit logMessage(reason.isEmpty()
                                ? QStringLiteral("Modbus RTU сервер недоступен")
                                : QStringLiteral("Modbus RTU сервер недоступен: %1").arg(reason));
            });

    connect(m_modbusSlave, &ModbusRtuSlave::serverOnline,
            this, [this]() {
                emit logMessage(QStringLiteral("Связь Modbus RTU сервера восстановлена"));
            });
}

void BackendController::applyInitialState()
{
    const bool emergency = (m_lines->systemState() == 1);
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

    m_lineIoManager->setProgramFire(on);

    emit logMessage(QStringLiteral("Ручной пожарный режим: %1")
                        .arg(on ? QStringLiteral("ВКЛ") : QStringLiteral("ВЫКЛ")));
    emit stateChanged();
    return true;
}

bool BackendController::setForcedStop(bool on)
{
    if (!m_lineIoManager)
        return false;

    if (on) {
       m_lineIoManager->setProgramFire(false);
    }

    emit logMessage(QStringLiteral("Сброс пожара: %1")
                        .arg(on ? QStringLiteral("ВЫПОЛНЕН")
                                : QStringLiteral("—")));
    emit stateChanged();
    return true;
}

int BackendController::calcAllLinesTestDurationSec() const
{
    return m_testController ? m_testController->calcAllLinesTestDurationSec() : 0;
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
    const QDateTime dt = QDateTime::fromMSecsSinceEpoch(msec, Qt::LocalTime);
    const QString iso = dt.toString("yyyy-MM-dd HH:mm:ss");

    log(QString("setSystemTime requested: %1").arg(iso));

#ifdef _WIN32
    SYSTEMTIME st;
    const QDateTime utc = dt.toUTC();

    st.wYear         = utc.date().year();
    st.wMonth        = utc.date().month();
    st.wDay          = utc.date().day();
    st.wHour         = utc.time().hour();
    st.wMinute       = utc.time().minute();
    st.wSecond       = utc.time().second();
    st.wMilliseconds = utc.time().msec();

    if (!SetSystemTime(&st)) {
        log("SetSystemTime failed");
        return false;
    }

    log(QString("System time updated successfully (Windows): %1")
            .arg(utc.toString(Qt::ISODate)));
    return true;

#else
    if (!dt.isValid()) {
        log(QString("setSystemTime failed: invalid datetime, msec=%1").arg(msec));
        return false;
    }

    const qint64 secs = dt.toSecsSinceEpoch();
    if (secs <= 0) {
        log(QString("setSystemTime failed: invalid epoch seconds=%1").arg(secs));
        return false;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(secs);
    ts.tv_nsec = 0;

    if (::clock_settime(CLOCK_REALTIME, &ts) != 0) {
        const int err = errno;
        log(QString("clock_settime failed errno=%1 err='%2'")
                .arg(err)
                .arg(QString::fromUtf8(strerror(err))));
        return false;
    }

    log(QString("System time updated successfully (Linux direct): %1").arg(iso));

    QProcess p;
    p.start("hwclock", {"--systohc"});
    if (!p.waitForFinished(3000)) {
        log("hwclock --systohc timeout");
    } else {
        const QString err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        log(QString("hwclock --systohc exit=%1 err='%2'")
                .arg(p.exitCode())
                .arg(err));
    }

    return true;
#endif
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
