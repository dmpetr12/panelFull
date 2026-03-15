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
        QString iso = dt.toString("yyyy-MM-dd hh:mm:ss");

        // 1. Выключаем NTP
       //int r1 = QProcess::execute("timedatectl", { "set-ntp", "false" });
        //qDebug() << "файл .hset-ntp false exit code:" << r1;

        // 2. Ставим время
        int r2 = QProcess::execute("timedatectl", { "set-time", iso });
        qDebug() << "set-time exit code:" << r2;
log(QStringLiteral("⏲ Системное время обновлено ") + iso);
        // 3. Включаем NTP обратно
        //int r3 = QProcess::execute("timedatectl", { "set-ntp", "true" });
        //qDebug() << "set-ntp true exit code:" << r3;

        // struct timeval tv;
        // tv.tv_sec  = dt.toSecsSinceEpoch();
        // tv.tv_usec = 0;

        // if (settimeofday(&tv, nullptr) != 0) {
        //     perror("❌ Ошибка установки времени");
        // } else {
        //     qDebug() << "✅ Системное время обновлено (Linux):" << dt.toString();
        // }
#endif
    }
};
