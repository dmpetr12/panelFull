#include "schedulemanager.h"

#include <QJsonDocument>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

#include "schedulestorage.h"
#include "schedulecalculator.h"

namespace {
constexpr int kCheckIntervalMs = 20'000;
constexpr int kWindowSec = 90;
constexpr int kDedupeSec = 180;
}

ScheduleManager::ScheduleManager(QObject *parent)
    : QObject(parent)
    , m_filePath("schedule.json")
{
    connect(&m_timer, &QTimer::timeout,
            this, &ScheduleManager::checkSchedule);

    m_timer.start(kCheckIntervalMs);

    emit logMessage(QStringLiteral("ScheduleManager initialized, interval=%1 ms")
                        .arg(kCheckIntervalMs));
}

void ScheduleManager::loadFromFile(const QString &path)
{
    m_filePath = path.isEmpty() ? QStringLiteral("schedule.json") : path;

    QFileInfo fi(m_filePath);
    if (!fi.exists()) {
        emit logMessage(QStringLiteral("Schedule file not found, creating new: %1").arg(m_filePath));
        saveToFile(m_filePath);
        return;
    }

    m_tests = ScheduleStorage::load(m_filePath);
    m_lastTriggered = QVector<QDateTime>(m_tests.size());

    emit logMessage(QStringLiteral("Schedule loaded, entries=%1").arg(m_tests.size()));
}

void ScheduleManager::saveToFile(const QString &path)
{
    QString targetPath = path.isEmpty() ? m_filePath : path;
    if (targetPath.isEmpty())
        targetPath = QStringLiteral("schedule.json");

    if (ScheduleStorage::save(targetPath, m_tests)) {
        emit logMessage(QStringLiteral("Schedule saved to %1").arg(targetPath));
    } else {
        emit logMessage(QStringLiteral("Failed to save schedule to %1").arg(targetPath));
    }
}

void ScheduleManager::autoSave()
{
    static QDateTime lastSave;

    if (m_tests.isEmpty())
        return;

    if (lastSave.isValid() &&
        lastSave.msecsTo(QDateTime::currentDateTime()) < 500) {
        return;
    }

    lastSave = QDateTime::currentDateTime();
    saveToFile();
}

void ScheduleManager::addTest(const QVariantMap &data)
{
    TestEntry e;
    e.enabled = data.value(QStringLiteral("enabled"), true).toBool();
    e.period = data.value(QStringLiteral("period")).toString();
    e.startDate = QDate::fromString(
        data.value(QStringLiteral("startDate")).toString(),
        QStringLiteral("yyyy-MM-dd")
        );
    e.startTime = QTime::fromString(
        data.value(QStringLiteral("startTime")).toString(),
        QStringLiteral("HH:mm")
        );
    e.testType = data.value(QStringLiteral("testType")).toString();

    const QVariantList rawDays = data.value(QStringLiteral("weekDays")).toList();
    for (const auto &d : rawDays)
        e.weekDays.append(d.toString());

    m_tests.append(e);
    m_lastTriggered.resize(m_tests.size());

    emit logMessage(QStringLiteral("Test added: %1 (%2)")
                        .arg(e.testType, e.period));

    saveToFile();
}

void ScheduleManager::removeTest(int index)
{
    if (index < 0 || index >= m_tests.size())
        return;

    const QString name = m_tests[index].testType;
    m_tests.removeAt(index);

    if (index < m_lastTriggered.size())
        m_lastTriggered.removeAt(index);

    autoSave();

    emit logMessage(QStringLiteral("Test removed: %1").arg(name));
}

QVariantList ScheduleManager::getAllTests() const
{
    QVariantList list;

    for (const auto &e : m_tests) {
        QVariantMap map;
        map[QStringLiteral("enabled")] = e.enabled;
        map[QStringLiteral("period")] = e.period;
        map[QStringLiteral("startDate")] = e.startDate.toString(QStringLiteral("yyyy-MM-dd"));
        map[QStringLiteral("startTime")] = e.startTime.toString(QStringLiteral("HH:mm"));
        map[QStringLiteral("testType")] = e.testType;
        map[QStringLiteral("weekDays")] = e.weekDays;
        list.append(map);
    }

    return list;
}

void ScheduleManager::updateTestProperty(int index, const QString &key, const QVariant &value)
{
    if (index < 0 || index >= m_tests.size())
        return;

    TestEntry &e = m_tests[index];

    if (key == QStringLiteral("enabled"))
        e.enabled = value.toBool();
    else if (key == QStringLiteral("period"))
        e.period = value.toString();
    else if (key == QStringLiteral("startDate"))
        e.startDate = QDate::fromString(value.toString(), QStringLiteral("yyyy-MM-dd"));
    else if (key == QStringLiteral("startTime"))
        e.startTime = QTime::fromString(value.toString(), QStringLiteral("HH:mm"));
    else if (key == QStringLiteral("testType"))
        e.testType = value.toString();
    else if (key == QStringLiteral("weekDays"))
        e.weekDays = value.toStringList();
    else
        return;

    autoSave();

    emit logMessage(QStringLiteral("Test updated: [%1] for %2")
                        .arg(key, e.testType));
}

void ScheduleManager::checkSchedule()
{
    const QDateTime now = QDateTime::currentDateTime();

    if (m_lastTriggered.size() != m_tests.size())
        m_lastTriggered = QVector<QDateTime>(m_tests.size());

    for (int i = 0; i < m_tests.size(); ++i) {
        const auto &e = m_tests[i];

        if (!e.enabled)
            continue;

        if (!e.startDate.isValid() || !e.startTime.isValid()) {
            emit logMessage(QStringLiteral("Invalid schedule entry skipped: %1").arg(e.testType));
            continue;
        }

        const QDateTime planned =
            ScheduleCalculator::buildCandidatePlanned(e, now);

        if (!planned.isValid())
            continue;

        const qint64 secsAfter = planned.secsTo(now);

        if (secsAfter < 0 || secsAfter > kWindowSec)
            continue;

        if (m_lastTriggered[i].isValid()) {
            if (m_lastTriggered[i] == planned)
                continue;

            if (m_lastTriggered[i].secsTo(now) < kDedupeSec)
                continue;
        }

        m_lastTriggered[i] = planned;

        emit startTestRequested(e.testType);
        emit logMessage(QStringLiteral("Scheduled test start: [%1], period=%2, planned=%3")
                            .arg(e.testType,
                                 e.period,
                                 planned.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        return;
    }
}
