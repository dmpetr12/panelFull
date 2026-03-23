#pragma once

#include <QDateTime>
#include <QString>
#include <vector>

struct LineSnapshot
{
    QString description;
    double mpower = 0.0;
    double power = 0.0;
    double current = 0.0;
    double voltage = 0.0;
    double tolerance = 0.0;
    int mode = 0;
    int status = 0;
    int lineState = 0;
    QDateTime lastMeasuredTest;
};

struct BatterySnapshot
{
    double voltage = 0.0;
    double current = 0.0;
    double chargePercent = 0.0;

    bool charging = false;
    bool batteryLow = false;
    bool batteryFault = false;
    bool onBattery = false;
};

struct DeviceSnapshot
{
    double inletU = 0.0;
    double inletI = 0.0;
    double inletP = 0.0;
    double inletF = 0.0;

    bool inletUAvailable = false;
    bool inletIAvailable = false;
    bool inletPAvailable = false;
    bool inletFAvailable = false;

    double testU = 0.0;
    double testI = 0.0;
    double testP = 0.0;

    bool testUAvailable = false;
    bool testIAvailable = false;
    bool testPAvailable = false;

    double temperature = 0.0;
    bool temperatureAvailable = false;

    bool busConnected = false;

    // пожар
    bool fireActive = false;         // аппаратный ИЛИ программный
    bool fireInput = false;          // только аппаратный вход
    bool programFireActive = false;  // только программный пожар

    // тесты
    bool stepTestActive = false;
    int  stepTestLine = -1;

    bool singleLineTestActive = false;
    int  singleLineTestLine = -1;

    bool noMeasTestActive = false;

    bool testRunning = false;

    int systemState = 0;

    std::vector<LineSnapshot> lines;
    BatterySnapshot battery;

    QString logLevel = "INFO";
};