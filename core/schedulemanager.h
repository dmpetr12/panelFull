#pragma once

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QVector>

#include "scheduletypes.h"

class ScheduleManager : public QObject
{
    Q_OBJECT

public:
    explicit ScheduleManager(QObject *parent = nullptr);

    Q_INVOKABLE void loadFromFile(const QString &path);
    Q_INVOKABLE void saveToFile(const QString &path = QString());
    Q_INVOKABLE void addTest(const QVariantMap &data);
    Q_INVOKABLE void removeTest(int index);
    Q_INVOKABLE QVariantList getAllTests() const;
    Q_INVOKABLE void updateTestProperty(int index, const QString &key, const QVariant &value);

signals:
    void startTestRequested(QString testType);
    void logMessage(const QString &msg);

private slots:
    void checkSchedule();

private:
    void autoSave();

private:
    QList<TestEntry> m_tests;
    QTimer m_timer;
    QString m_filePath;
    QVector<QDateTime> m_lastTriggered;
};
