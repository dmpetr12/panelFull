#ifndef SYSTEMOBJECTS_H
#define SYSTEMOBJECTS_H

#include <QObject>
#include <QString>
#include <QColor>

// --- Сеть ---
class Network : public QObject {
    Q_OBJECT
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QColor statusColor READ statusColor NOTIFY statusChanged)

public:
    enum Status { Ok, Failure };
    Q_ENUM(Status)

    explicit Network(QObject *parent = nullptr) : QObject(parent), m_status(Ok) {}

    Status status() const { return m_status; }
    void setStatus(Status s) {
        if (m_status == s) return;
        m_status = s;
        emit statusChanged();
    }

    QString statusText() const {
        return m_status == Ok ? "ОК" : "Авария";
    }

    QColor statusColor() const {
        return m_status == Ok ? QColor("#5EC85E") : QColor("#FF4C4C");
    }

signals:
    void statusChanged();

private:
    Status m_status;
};


// --- Система ---
class System : public QObject {
    Q_OBJECT
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QColor statusColor READ statusColor NOTIFY statusChanged)

public:
    enum Status { Ok, Failure, Test };
    Q_ENUM(Status)

    explicit System(QObject *parent = nullptr) : QObject(parent), m_status(Ok) {}

    Status status() const { return m_status; }
    void setStatus(Status s) {
        if (m_status == s) return;
        m_status = s;
        emit statusChanged();
    }

    QString statusText() const {
        return m_status == Ok ? "ОК" :
                   m_status == Failure ? "Авария" : "Тест";
    }

    QColor statusColor() const {
        return m_status == Ok ? QColor("#5EC85E") :
                   m_status == Failure ? QColor("#FF4C4C") : QColor("#FFD700");
    }

signals:
    void statusChanged();

private:
    Status m_status;
};


// --- Режим ---
class Mode : public QObject {
    Q_OBJECT
    Q_PROPERTY(State state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    Q_PROPERTY(QColor stateColor READ stateColor NOTIFY stateChanged)

public:
    enum State { Work, Emergency, Test, Fire, Force };
    Q_ENUM(State)

    explicit Mode(QObject *parent = nullptr) : QObject(parent), m_state(Work) {}

    State state() const { return m_state; }
    void setState(State st) {
        if (m_state == st) return;
        m_state = st;
        emit stateChanged();
    }

    QString stateText() const {
        switch (m_state) {
        case Work: return "Работа";
        case Emergency: return "Авария";
        case Test: return "Тест";
        case Fire: return "Пожар";
        case Force: return "Диспеч";
        }
        return "Unknown";
    }

    QColor stateColor() const {
        switch (m_state) {
        case Work: return QColor("#5EC85E");
        case Emergency: return QColor("#FF4C4C");
        case Test: return QColor("#FFC700");
        case Fire: return QColor("#FF4C4C");
        case Force: return QColor("#FFC700");
        }
        return QColor("gray");
    }

signals:
    void stateChanged();

private:
    State m_state;
};

#endif // SYSTEMOBJECTS_H
