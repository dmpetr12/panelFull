#pragma once

#include <QObject>

    class BatteryController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double voltage READ voltage NOTIFY stateChanged)
    Q_PROPERTY(double current READ current NOTIFY stateChanged)
    Q_PROPERTY(double chargePercent READ chargePercent NOTIFY stateChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY stateChanged)
    Q_PROPERTY(bool batteryLow READ batteryLow NOTIFY stateChanged)
    Q_PROPERTY(bool batteryFault READ batteryFault NOTIFY stateChanged)
    Q_PROPERTY(bool onBattery READ onBattery NOTIFY stateChanged)

public:
    explicit BatteryController(QObject *parent = nullptr);

    double voltage() const;
    double current() const;
    double chargePercent() const;

    bool charging() const;
    bool batteryLow() const;
    bool batteryFault() const;
    bool onBattery() const;

public slots:
    void setVoltage(double value);
    void setCurrent(double value);
    void setChargePercent(double value);
    void setCharging(bool value);
    void setBatteryLow(bool value);
    void setBatteryFault(bool value);
    void setOnBattery(bool value);

signals:
    void stateChanged();

private:
    double m_voltage = 0.0;
    double m_current = 0.0;
    double m_chargePercent = 0.0;

    bool m_charging = false;
    bool m_batteryLow = false;
    bool m_batteryFault = false;
    bool m_onBattery = false;
};
