#pragma once

#include <QList>
#include <QString>

#include "scheduletypes.h"

class ScheduleStorage
{
public:
    static QList<TestEntry> load(const QString &path);
    static bool save(const QString &path, const QList<TestEntry> &tests);
};
