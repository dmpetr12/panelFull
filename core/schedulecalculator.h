#pragma once

#include <QDateTime>

#include "scheduletypes.h"

class ScheduleCalculator
{
public:
    static QDateTime buildCandidatePlanned(const TestEntry &entry,
                                           const QDateTime &now);

private:
    static int weekdayKeyToQt(const QString &key);
    static int monthsForPeriod(const QString &period);
};
