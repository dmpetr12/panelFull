#pragma once
#include <QObject>
#include <QTimer>
#include <QDateTime>

#include <QVariantMap>
#include <QVariantList>

#include "linesmodel.h"
#include "TestController.h"

#include "logger.h"

class MaintenanceChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int overdueLinesCount READ overdueLinesCount NOTIFY overdueLinesCountChanged)
    Q_PROPERTY(bool longTestOverdue READ longTestOverdue NOTIFY longTestOverdueChanged)

public:
    explicit MaintenanceChecker(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &MaintenanceChecker::onDailyTick);
    }

    void bind(LinesModel *lines, TestController *tests)
    {
        m_lines = lines;
        m_tests = tests;
    }

    Q_INVOKABLE void startDaily(int hour = 7, int minute = 0)
    {
        m_hour = hour;
        m_minute = minute;

        // Проверим сразу при старте приложения (полезно)
        QTimer::singleShot(500, this, [this]{ checkNow(false); });
        scheduleNext();
    }

    Q_INVOKABLE QVariantMap testSummary() const
    {
        QVariantMap out;
        if (!m_lines || !m_tests) {
            out["longTestDate"] = "—";
            out["longTestResult"]= "-";
            out["lines"] = QVariantList{};
            return out;
        }

        const QDateTime lastLong = m_tests->lastLongSystemTest();
        out["longTestDate"] = lastLong.isValid()
                                  ? lastLong.toString("dd.MM.yyyy HH:mm")
                                  : QString("никогда");
        out["longTestResult"]= m_tests->lastLongTestResult() ? "OK" :"Неисправен";

        QVariantList lines;
        lines.reserve(m_lines->rowCount());

        for (int i = 0; i < m_lines->rowCount(); ++i) {
            Line *ln = m_lines->line(i);
            if (!ln) continue;
            if (ln->mode() == Line::NotUsed) continue;

            const QDateTime last = ln->lastMeasuredTest();

            QVariantMap row;
            row["index"] = i + 1;
            row["description"] = ln->description();
            row["status"] = ln->statusString();
            row["lastTestDate"] = last.isValid()
                                      ? last.toString("dd.MM.yyyy")
                                      : QString("никогда");

            lines.push_back(row);
        }

        out["lines"] = lines;
        return out;
    }

    int overdueLinesCount() const { return m_overdueLinesCount; }
    bool longTestOverdue() const { return m_longTestOverdue; }

signals:
    void overdueLinesCountChanged();
    void longTestOverdueChanged();

    // Можно ловить в QML и показывать popup/иконку
    void maintenanceWarning(int overdueLines, bool longTestOverdue);

private slots:
    void onDailyTick()
    {
        checkNow(false);
        scheduleNext();
    }

private:
    void scheduleNext()
    {
        const QDateTime now = QDateTime::currentDateTime();

        QDateTime next(now.date(), QTime(m_hour, m_minute));
        if (next <= now)
            next = next.addDays(1);

        const qint64 ms = now.msecsTo(next);
        m_timer.start(int(ms > 0 ? ms : 60'000)); // защита
    }

    void setOverdueLinesCount(int v)
    {
        if (m_overdueLinesCount == v) return;
        m_overdueLinesCount = v;
        emit overdueLinesCountChanged();
    }

    void setLongTestOverdue(bool v)
    {
        if (m_longTestOverdue == v) return;
        m_longTestOverdue = v;
        emit longTestOverdueChanged();
    }

    void checkNow(bool quiet)
    {
        if (!m_lines || !m_tests) return;

        const QDateTime now = QDateTime::currentDateTime();
        log(QString("Состояние линий;дата=%1").arg(now.toString("dd-MM-yy")));
        // --- 1) длинный тест шкафа ---
        const QDateTime lastLong = m_tests->lastLongSystemTest();
        bool longOverdue = false;

        if (!lastLong.isValid()) longOverdue = true;
        else if (lastLong.daysTo(now) > longLimitDays) longOverdue = true;

        setLongTestOverdue(longOverdue);

        // --- 2) линии ---
        int overdue = 0;
        QStringList overdueNames;

        for (int i = 0; i < m_lines->rowCount(); ++i) {
            Line *ln = m_lines->line(i);
            if (!ln) continue;
            if (ln->mode() == Line::NotUsed) continue;
            log(QString("%1;состояние %3;дата теста %4").arg(ln->description())
                    .arg(ln->status() == Line::OK ? "ОК" : "авария")
                    .arg(ln->lastMeasuredTest().isValid() ? (ln->lastMeasuredTest()).toString("dd-MM-yy") : "не проводился"));

            const QDateTime last = ln->lastMeasuredTest();
            bool isOverdue = false;

            if (!last.isValid()) {
                isOverdue = true;
            } else if (last.daysTo(now) > lineLimitDays) {
                isOverdue = true;
            }

            if (isOverdue) {
                overdue++;
                overdueNames << QString("%1 (%2)")
                                    .arg(ln->description())
                                    .arg(last.isValid() ? last.toString("yyyy-MM-dd") : "никогда");
            }
        }

        setOverdueLinesCount(overdue);

        // --- сообщения только если нужно ---
        if (quiet) return;                 // тихий режим — только обновили свойства
        QStringList parts;

        if (longOverdue) {
            parts << QString("🚨 Тест шкафа на время не выполнялся > %1 дней (последний: %2)")
                         .arg(longLimitDays)
                         .arg(lastLong.isValid() ? lastLong.toString("yyyy-MM-dd") : "никогда");
        }

        if (overdue > 0) {
            parts << QString("⚠️ %1 линий не тестировались > %2 дней: %3")
                .arg(overdue).arg(lineLimitDays).arg(overdueNames.join(", "));
        }

        if (parts.isEmpty()) return;       // всё ок — ничего не спамим

        const QString msg = parts.join("\n");
        emit maintenanceWarning(m_overdueLinesCount, m_longTestOverdue);
        log(msg);
    }

private:
    LinesModel *m_lines = nullptr;
    TestController *m_tests = nullptr;

    QTimer m_timer;
    int m_hour = 7;
    int m_minute = 0;

    int m_overdueLinesCount = 0;
    bool m_longTestOverdue = false;

    const int lineLimitDays = 30;
    const int longLimitDays = 365;
};

