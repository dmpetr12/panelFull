#ifndef TESTCONTROLLER_H
#define TESTCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

#include "linesmodel.h"
#include "LineIoManager.h"

class ModbusBus;

class TestController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool testActive READ testActive NOTIFY testActiveChanged)
    Q_PROPERTY(Source testSource READ testSource NOTIFY testSourceChanged)
    Q_PROPERTY(QDateTime lastLongSystemTest READ lastLongSystemTest NOTIFY lastLongSystemTestChanged)
    Q_PROPERTY(TestKind currentTestKind READ currentTestKind NOTIFY currentTestKindChanged)
    Q_PROPERTY(int measuredLine READ measuredLine NOTIFY measuredLineChanged)

public:
    explicit TestController(QObject *parent = nullptr);

    enum class Source { None = 0, Schedule = 1, Operator = 2 };
    Q_ENUM(Source)

    enum class TestKind { None = 0, Functional = 1, Duration = 2 };
    Q_ENUM(TestKind)

    TestKind currentTestKind() const { return m_currentTestKind; }

    void setModel(LinesModel *model)      { m_model = model; }
    void setIoManager(LineIoManager *io)  { m_io = io; }
    void setMeterFastIntervalMs(int ms)   { m_meterFastIntervalMs = ms; }
    void setBus(ModbusBus *bus)           { m_bus = bus; }

    bool testActive() const { return m_active != Active::None; }
    Source testSource() const { return m_source; }
    QDateTime lastLongSystemTest() const { return m_lastLongSystemTest; }
    bool lastLongTestResult() const { return m_lastLongSystemTestOk; }
    int measuredLine() const { return m_measuredLine; }

    Q_INVOKABLE void startTestOperator(int lineIndex, int sec);
    Q_INVOKABLE bool startTestScheduleAll(int sec);
    Q_INVOKABLE void startTest(int lineIndex, int sec);
    Q_INVOKABLE void startTestNoMeasure(int sec);
    Q_INVOKABLE void stopTest(int lineIndex);
    Q_INVOKABLE int calcAllLinesTestDurationSec() const;

    void setLinesSavePath(const QString& path) { m_linesSavePath = path; }
    void setLongTestThresholdMinutes(int m) { m_longThresholdMin = m; }
    void setLongTestToleranceBonus(double bonusPercent) { m_longTolBonus = bonusPercent; }

signals:
    void measuredLineChanged(int lineIndex);
    void testActiveChanged();
    void testSourceChanged();
    void lastLongSystemTestChanged();
    void currentTestKindChanged();

private:
    enum class Active { None, Single, All, NoMeas };

    void startTestInternal(int lineIndex, int sec);
    void stopAnyActiveTest();

    void cleanupSingleTimer();
    void cleanupNoMeasTimer();

    void allLineStatusReturn();

    void beginMeasurements();
    void endMeasurements();

    bool isLineTestable(Line *ln) const;
    int  findNextTestableIndex(int fromExclusive) const;
    int  findFirstTestableIndex() const;

    void startSingleLineTest(int lineIndex, int durationMs);
    void startAllLinesTest(int durationMs);
    void handleAllLinesStep();

    bool checkLinePowerInternal(Line *line) const;

    void setMeasuredLine(int idx);
    void setSource(Source s);
    void setActive(Active a);

    void recordLongSystemTestResult(bool ok);
    void loadMaintenance();

    void setCurrentTestKind(TestKind k);

private:
    LinesModel    *m_model = nullptr;
    LineIoManager *m_io    = nullptr;
    ModbusBus     *m_bus   = nullptr;

    int m_meterFastIntervalMs = 200;

    QTimer *m_singleTestTimer = nullptr;
    QTimer *m_noMeasTimer = nullptr;

    QTimer *m_allTestTimer = nullptr;
    bool    m_allTestRunning = false;
    int     m_allTestCurrentIndex = -1;

    int     m_shotTimeMesure = 10;
    int     m_shotTimeWarm = 60;
    int     minSecTestDuration = 60;

    Active  m_active = Active::None;
    Source  m_source = Source::None;
    int     m_measuredLine = -1;

    QString m_linesSavePath = "lines.json";
    int     m_longThresholdMin = 60;
    double  m_longTolBonus = 10.0;

    bool m_currentAllIsLong = false;
    bool m_currentAllAllOk  = true;

    QString   m_maintenancePath = "maintenance.json";
    QDateTime m_lastLongSystemTest;
    bool      m_lastLongSystemTestOk = true;
    TestKind  m_currentTestKind = TestKind::None;
};

#endif // TESTCONTROLLER_H