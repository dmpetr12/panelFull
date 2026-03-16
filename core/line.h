#ifndef LINE_H
#define LINE_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QDateTime>

class Line : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(double mpower READ mpower WRITE setmPower NOTIFY mpowerChanged)
    Q_PROPERTY(double power READ power WRITE setPower NOTIFY powerChanged)
    Q_PROPERTY(double current READ current WRITE setCurrent NOTIFY currentChanged)
    Q_PROPERTY(double voltage READ voltage WRITE setVoltage NOTIFY voltageChanged)
    Q_PROPERTY(double tolerance READ tolerance WRITE setTolerance NOTIFY toleranceChanged)
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(LineState lineState READ lineState WRITE setLineState NOTIFY lineStateChanged)
    Q_PROPERTY(QDateTime lastMeasuredTest READ lastMeasuredTest WRITE setLastMeasuredTest NOTIFY lastMeasuredTestChanged)

public:
    enum Mode {
        Constant,
        NonConstant,
        NotUsed
    };
    Q_ENUM(Mode)

    enum Status {
        OK,
        Failure,
        Test,
        Undefined
    };
    Q_ENUM(Status)

    enum LineState {
        Off,
        On
    };
    Q_ENUM(LineState)

    // --- Constructors: обязательно инициализируем все поля ---
    explicit Line(QObject *parent = nullptr)
        : QObject(parent),
        m_description(QString()),
        m_power(0.0),
        m_mpower(0.0),
        m_current(0.0),
        m_voltage(0.0),
        m_tolerance(0.0),
        m_mode(Constant),
        m_status(OK),
        m_oldStatus(OK),
        m_lineState(Off),
        m_lastMeasuredTest(QDateTime())
    {}

    Line(const QString &desc,
         double mpower,
         double power,
         double current,
         double voltage,
         double tolerance,
         Mode mode = Constant,
         Status status = OK,
         LineState lineState = Off,
         QObject *parent = nullptr)
        : QObject(parent),
        m_description(desc),
        m_power(power),
        m_mpower(mpower),
        m_current(current),
        m_voltage(voltage),
        m_tolerance(tolerance),
        m_mode(mode),
        m_status(status),
        m_lineState(lineState),
        m_lastMeasuredTest()
    {}

    // --- getters ---
    QString description() const { return m_description; }
    double mpower() const { return m_mpower; }
    double power() const { return m_power; }
    double current() const { return m_current; }
    double voltage() const { return m_voltage; }
    double tolerance() const { return m_tolerance; }
    Mode mode() const { return m_mode; }
    Status status() const { return m_status; }
    Status oldStatus() const { return m_oldStatus; }
    LineState lineState() const { return m_lineState; }
    QDateTime lastMeasuredTest() const { return m_lastMeasuredTest; }

public slots:
    // --- setters ---
    void setDescription(const QString &desc) {
        if (m_description != desc) {
            m_description = desc;
            emit descriptionChanged();
        }
    }

    void setPower(double p) {
        if (!qFuzzyCompare(m_power, p)) {
            m_power = p;
            emit powerChanged();
        }
    }
    void setmPower(double p) {
        if (!qFuzzyCompare(m_mpower, p)) {
            m_mpower = p;
            emit mpowerChanged();
        }
    }
    void setCurrent(double p) {
        if (!qFuzzyCompare(m_current, p)) {
            m_current = p;
            emit currentChanged();
        }
    }
    void setVoltage(double p) {
        if (!qFuzzyCompare(m_voltage, p)) {
            m_voltage = p;
            emit voltageChanged();
        }
    }
    void setTolerance(double t) {
        if (!qFuzzyCompare(m_tolerance, t)) {
            m_tolerance = t>50 ? 50 : t;
            emit toleranceChanged();
        }
    }

    void setMode(Mode m) {
        if (m_mode != m) {
            m_mode = m;
            emit modeChanged();
        }
    }

    void setStatus(Status s) {
        if (m_status != s) {
            m_status = s;
            emit statusChanged();
        }
    }
    void setOldStatus(Status s) { m_oldStatus = s;}

    void setLineState(LineState st) {
        if (m_lineState != st) {
            m_lineState = st;
            emit lineStateChanged();
        }
    }

    void setLastMeasuredTest(const QDateTime& dt) {
        if (m_lastMeasuredTest != dt) {
            m_lastMeasuredTest = dt;
            emit lastMeasuredTestChanged();
        }
    }

    // Вспомогательные методы для QML
    Q_INVOKABLE QString statusString() const {
        switch (m_status) {
        case OK: return "OK";
        case Failure: return "АВАРИЯ";
        case Test: return "ТЕСТ";
        case Undefined: return "—";
        }
        return "Unknown";
    }

    Q_INVOKABLE QColor statusColor() const {
        switch (m_status) {
        case OK:
            return QColor("#5EC85E");
        case Failure:
            return QColor("#FF4C4C");
        case Test:
            return QColor("#FFC700");
        case Undefined:
            return QColor("gray");
        }
        return QColor("gray");
    }

    Q_INVOKABLE QString lineStateString() const {
        return m_lineState == On ? "вкл" : "выкл";
    }

signals:
    void descriptionChanged();
    void powerChanged();
    void mpowerChanged();
    void currentChanged();
    void voltageChanged();
    void toleranceChanged();
    void modeChanged();
    void statusChanged();
    void lineStateChanged();
    void lastMeasuredTestChanged();

private:
    QString m_description;
    double m_power;
    double m_mpower;
    double m_current;
    double m_voltage;
    double m_tolerance;
    Mode m_mode;
    Status m_status;
    Status m_oldStatus = Line::OK;
    LineState m_lineState;
    QDateTime m_lastMeasuredTest;
};

#endif // LINE_H
