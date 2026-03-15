#include "schedulestorage.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

QList<TestEntry> ScheduleStorage::load(const QString &path)
{
    QList<TestEntry> tests;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ScheduleStorage: cannot open" << path << "for read";
        return tests;
    }

    const QByteArray data = file.readAll();
    file.close();

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "ScheduleStorage: invalid JSON array in" << path;
        return tests;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QJsonObject obj = value.toObject();

        TestEntry e;
        e.enabled = obj["enabled"].toBool(true);
        e.period = obj["period"].toString();
        e.startDate = QDate::fromString(obj["startDate"].toString(), "yyyy-MM-dd");
        e.startTime = QTime::fromString(obj["startTime"].toString(), "HH:mm");
        e.testType = obj["testType"].toString();

        const QJsonArray weekArr = obj["weekDays"].toArray();
        for (const QJsonValue &d : weekArr)
            e.weekDays << d.toString();

        tests.append(e);
    }

    return tests;
}

bool ScheduleStorage::save(const QString &path, const QList<TestEntry> &tests)
{
    QJsonArray arr;

    for (const auto &e : tests) {
        QJsonObject obj;
        obj["enabled"] = e.enabled;
        obj["period"] = e.period;
        obj["startDate"] = e.startDate.toString("yyyy-MM-dd");
        obj["startTime"] = e.startTime.toString("HH:mm");
        obj["testType"] = e.testType;

        QJsonArray weekArr;
        for (const auto &d : e.weekDays)
            weekArr.append(d);
        obj["weekDays"] = weekArr;

        arr.append(obj);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "ScheduleStorage: cannot open" << path << "for write";
        return false;
    }

    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
