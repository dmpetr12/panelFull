#pragma once
#include <QObject>
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include "logger.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
extern void log(const QString &msg);

class TimeAjst : public QObject {
    Q_OBJECT
public:
    explicit TimeAjst(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void setSystemTime(qint64 msec) {
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(msec);

#ifdef _WIN32
        SYSTEMTIME st;
        dt = dt.toUTC(); // Windows ожидает UTC

        st.wYear         = dt.date().year();
        st.wMonth        = dt.date().month();
        st.wDay          = dt.date().day();
        st.wHour         = dt.time().hour();
        st.wMinute       = dt.time().minute();
        st.wSecond       = dt.time().second();
        st.wMilliseconds = dt.time().msec();

        if (!SetSystemTime(&st)) {
            qDebug() << "❌ Ошибка: не удалось установить время (нужны права администратора)";
        } else {
            qDebug() << "✅ Системное время обновлено (Windows):" << dt.toString();
        }
#else
        QString iso = dt.toString("yyyy-MM-dd HH:mm:ss");

        QProcess p1;
        p1.start("timedatectl", {"set-ntp", "false"});
        p1.waitForFinished();
        qDebug() << "set-ntp false exit:" << p1.exitCode();
        qDebug() << "stderr:" << p1.readAllStandardError();

        QProcess p2;
        p2.start("timedatectl", {"set-time", iso});
        p2.waitForFinished();
        qDebug() << "set-time exit:" << p2.exitCode();
        qDebug() << "stderr:" << p2.readAllStandardError();

        QProcess p3;
        p3.start("timedatectl", {"set-ntp", "true"});
        p3.waitForFinished();
        qDebug() << "set-ntp true exit:" << p3.exitCode();
        qDebug() << "stderr:" << p3.readAllStandardError();
#endif
    }
};
