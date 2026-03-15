#pragma once

#include <QString>
#include <QStringList>
#include <QDate>
#include <QTime>

struct TestEntry
{
    bool enabled = true;
    QString period;
    QDate startDate;
    QTime startTime;
    QString testType;
    QStringList weekDays;
};
