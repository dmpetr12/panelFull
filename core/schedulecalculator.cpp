#include "schedulecalculator.h"

#include <QSet>

int ScheduleCalculator::weekdayKeyToQt(const QString &key)
{
    if (key == "Mon") return 1;
    if (key == "Tue") return 2;
    if (key == "Wed") return 3;
    if (key == "Thu") return 4;
    if (key == "Fri") return 5;
    if (key == "Sat") return 6;
    if (key == "Sun") return 7;
    return 0;
}

int ScheduleCalculator::monthsForPeriod(const QString &period)
{
    if (period == QStringLiteral("раз в месяц")) return 1;
    if (period == QStringLiteral("раз в 3 месяца")) return 3;
    if (period == QStringLiteral("раз в полгода")) return 6;
    if (period == QStringLiteral("раз в год")) return 12;
    return 0;
}

QDateTime ScheduleCalculator::buildCandidatePlanned(const TestEntry &e,
                                                    const QDateTime &now)
{
    if (!e.startTime.isValid() || !e.startDate.isValid())
        return QDateTime();

    const QString p = e.period.isEmpty()
                          ? QStringLiteral("один раз")
                          : e.period.trimmed();

    if (p == QStringLiteral("один раз")) {
        return QDateTime(e.startDate, e.startTime);
    }

    if (p == QStringLiteral("ежедневно")) {
        QDate day = now.date();
        if (day < e.startDate)
            day = e.startDate;

        QDateTime planned(day, e.startTime);

        if (planned > now) {
            planned = planned.addDays(-1);
            if (planned.date() < e.startDate)
                return QDateTime();
        }

        return planned;
    }

    if (p == QStringLiteral("дни недели")) {
        if (e.weekDays.isEmpty())
            return QDateTime();

        QSet<int> allowed;
        for (const QString &k : e.weekDays) {
            const int d = weekdayKeyToQt(k);
            if (d)
                allowed.insert(d);
        }

        if (allowed.isEmpty())
            return QDateTime();

        QDate day = now.date();
        if (day < e.startDate)
            day = e.startDate;

        QDateTime planned(day, e.startTime);
        if (planned > now)
            day = day.addDays(-1);

        for (int back = 0; back < 8; ++back) {
            if (day < e.startDate)
                return QDateTime();

            QDateTime cand(day, e.startTime);
            if (cand <= now && allowed.contains(day.dayOfWeek()))
                return cand;

            day = day.addDays(-1);
        }

        return QDateTime();
    }

    const int stepMonths = monthsForPeriod(p);
    if (stepMonths > 0) {
        const QDateTime base(
            QDate(e.startDate.year(), e.startDate.month(), e.startDate.day()),
            e.startTime
            );

        if (!base.isValid())
            return QDateTime();

        if (now < base)
            return QDateTime();

        const int monthsDiff =
            (now.date().year() - base.date().year()) * 12 +
            (now.date().month() - base.date().month());

        const int k = (monthsDiff / stepMonths) * stepMonths;
        QDateTime planned = base.addMonths(k);

        if (planned > now)
            planned = planned.addMonths(-stepMonths);

        if (planned < base)
            planned = base;

        if (planned <= now)
            return planned;

        return QDateTime();
    }

    return QDateTime(e.startDate, e.startTime);
}
