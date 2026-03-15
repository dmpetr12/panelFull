/*Сделаем JSON-сообщения.
 Запросы:
{ "cmd": "getState" }
{ "cmd": "startFunctionalTest" }
{ "cmd": "startDurationTest" }
{ "cmd": "stopCurrentTest" }
{ "cmd": "setForcedFire", "on": true }
{ "cmd": "resetAlarm" }

Ответы:
{ "ok": true, "state": { ... } }
{ "ok": true }
{ "ok": false, "error": "..." } */

#pragma once

#include <QJsonArray>
#include <QJsonObject>

#include "DeviceSnapshot.h"

inline QJsonObject lineSnapshotToJson(const LineSnapshot &l)
{
    QJsonObject obj;
    obj["description"] = l.description;
    obj["mpower"] = l.mpower;
    obj["power"] = l.power;
    obj["current"] = l.current;
    obj["voltage"] = l.voltage;
    obj["tolerance"] = l.tolerance;
    obj["mode"] = l.mode;
    obj["status"] = l.status;
    obj["lineState"] = l.lineState;
    obj["lastMeasuredTest"] = l.lastMeasuredTest.toString(Qt::ISODate);
    return obj;
}

inline QJsonObject batterySnapshotToJson(const BatterySnapshot &b)
{
    QJsonObject obj;
    obj["voltage"] = b.voltage;
    obj["current"] = b.current;
    obj["chargePercent"] = b.chargePercent;
    obj["charging"] = b.charging;
    obj["batteryLow"] = b.batteryLow;
    obj["batteryFault"] = b.batteryFault;
    obj["onBattery"] = b.onBattery;
    return obj;
}

inline QJsonObject deviceSnapshotToJson(const DeviceSnapshot &s)
{
    QJsonObject obj;

    obj["inletU"] = s.inletU;
    obj["inletI"] = s.inletI;
    obj["inletP"] = s.inletP;
    obj["inletF"] = s.inletF;

    obj["testU"] = s.testU;
    obj["testI"] = s.testI;
    obj["testP"] = s.testP;

    obj["temperature"] = s.temperature;

    obj["busConnected"] = s.busConnected;
    obj["fireInput"] = s.fireInput;
    obj["fireActive"] = s.fireActive;
    obj["stopActive"] = s.stopActive;
    obj["dispatcherActive"] = s.dispatcherActive;

    obj["fireTestActive"] = s.fireTestActive;
    obj["fireTestLine"] = s.fireTestLine;

    obj["singleLineTestActive"] = s.singleLineTestActive;
    obj["singleLineTestLine"] = s.singleLineTestLine;

    obj["systemState"] = s.systemState;
    obj["testRunning"] = s.testRunning;

    obj["battery"] = batterySnapshotToJson(s.battery);

    QJsonArray lines;
    for (const auto &line : s.lines)
        lines.append(lineSnapshotToJson(line));
    obj["lines"] = lines;

    return obj;
}
