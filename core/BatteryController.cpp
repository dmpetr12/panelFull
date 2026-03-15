#include "BatteryController.h"

#include <QtGlobal>

BatteryController::BatteryController(QObject *parent)
    : QObject(parent)
{
}

double BatteryController::voltage() const
{
    return m_voltage;
}

double BatteryController::current() const
{
    return m_current;
}

double BatteryController::chargePercent() const
{
    return m_chargePercent;
}

bool BatteryController::charging() const
{
    return m_charging;
}

bool BatteryController::batteryLow() const
{
    return m_batteryLow;
}

bool BatteryController::batteryFault() const
{
    return m_batteryFault;
}

bool BatteryController::onBattery() const
{
    return m_onBattery;
}

void BatteryController::setVoltage(double value)
{
    if (qFuzzyCompare(m_voltage, value))
        return;

    m_voltage = value;
    emit stateChanged();
}

void BatteryController::setCurrent(double value)
{
    if (qFuzzyCompare(m_current, value))
        return;

    m_current = value;
    emit stateChanged();
}

void BatteryController::setChargePercent(double value)
{
    if (qFuzzyCompare(m_chargePercent, value))
        return;

    m_chargePercent = value;
    emit stateChanged();
}

void BatteryController::setCharging(bool value)
{
    if (m_charging == value)
        return;

    m_charging = value;
    emit stateChanged();
}

void BatteryController::setBatteryLow(bool value)
{
    if (m_batteryLow == value)
        return;

    m_batteryLow = value;
    emit stateChanged();
}

void BatteryController::setBatteryFault(bool value)
{
    if (m_batteryFault == value)
        return;

    m_batteryFault = value;
    emit stateChanged();
}

void BatteryController::setOnBattery(bool value)
{
    if (m_onBattery == value)
        return;

    m_onBattery = value;
    emit stateChanged();
}
