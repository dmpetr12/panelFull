#pragma once

#include <QObject>
#include <QTimer>
#include <QSerialPortInfo>
#include <QDebug>

class UsbSerialWatcher : public QObject
{
    Q_OBJECT
public:
    explicit UsbSerialWatcher(quint16 vid,
                              quint16 pid,
                              int checkIntervalMs = 2000,
                              QObject *parent = nullptr)
        : QObject(parent)
        , m_vid(vid)
        , m_pid(pid)
    {
        m_timer.setInterval(checkIntervalMs);
        m_timer.setSingleShot(false);
        connect(&m_timer, &QTimer::timeout,
                this, &UsbSerialWatcher::checkNow);
    }

    void start() { m_timer.start(); }
    void stop()  { m_timer.stop();  }

    quint16 vid() const { return m_vid; }
    quint16 pid() const { return m_pid; }

signals:
    // Нашли/изменился порт: portName типа "COM7" или "/dev/ttyUSB0"
    void portAvailable(const QString &portName);
    // Устройство пропало (порта больше нет)
    void portLost();

private slots:
    void checkNow()
    {
        QString found;
        const auto ports = QSerialPortInfo::availablePorts();

        for (const QSerialPortInfo &info : ports) {
            if (info.vendorIdentifier() == m_vid &&
                info.productIdentifier() == m_pid)
            {
#ifdef Q_OS_WIN
                found = info.portName();        // "COM7"
#else
                found = info.systemLocation();  // "/dev/ttyUSB0"
#endif
                // можно залогировать подробности один раз
                if (found != m_lastPortName) {
                    qDebug() << "[UsbSerialWatcher]"
                             << "VID:" << QString("0x%1").arg(info.vendorIdentifier(), 4, 16, QLatin1Char('0'))
                             << "PID:" << QString("0x%1").arg(info.productIdentifier(), 4, 16, QLatin1Char('0'))
                             << "on"   << found
                             << "desc:" << info.description()
                             << "serial:" << info.serialNumber();
                }
                break;
            }
        }

        if (found.isEmpty()) {
            // ничего не нашли
            if (!m_lastPortName.isEmpty()) {
                // было — стало нет -> потеряли устройство
                qDebug() << "[UsbSerialWatcher] device lost, last port =" << m_lastPortName;
                m_lastPortName.clear();
                emit portLost();
            }
        } else {
            // нашли порт
            if (found != m_lastPortName) {
                m_lastPortName = found;
                qDebug() << "[UsbSerialWatcher] device available on" << found;
                emit portAvailable(found);
            }
        }
    }

private:
    quint16 m_vid;
    quint16 m_pid;
    QTimer  m_timer;
    QString m_lastPortName;
};
