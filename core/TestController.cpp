#include "TestController.h"
#include "modbusbus.h"
#include "logger.h"

void TestController::setCurrentTestKind(TestKind k)
{
    if (m_currentTestKind == k)
        return;
    m_currentTestKind = k;
    emit currentTestKindChanged();
}

TestController::TestController(QObject *parent)
    : QObject(parent)
    , m_allTestTimer(new QTimer(this))
{
    m_allTestTimer->setSingleShot(true);
    connect(m_allTestTimer, &QTimer::timeout, this, [this]() {
        handleAllLinesStep();
    });
    loadMaintenance();
}

void TestController::startTestOperator(int lineIndex, int sec)
{
    stopAnyActiveTest();
    setSource(Source::Operator);
    startTestInternal(lineIndex, sec);
}

bool TestController::startTestScheduleAll(int sec)
{
    if (!m_model || !m_io) return false;
    if (sec < minSecTestDuration) sec = minSecTestDuration;

    if (testActive() && m_source == Source::Operator) {
        log("⛔ Schedule: запуск теста всех линий запрещён — идёт операторский тест");
        return false;
    }

    if (testActive() && m_source == Source::Schedule && m_currentAllIsLong) {
        log("⛔ Schedule: запуск запрещён — уже идёт длинный тест по расписанию");
        return false;
    }

    stopAnyActiveTest();
    setSource(Source::Schedule);

    startTestInternal(-2, sec);
    return true;
}

void TestController::startTest(int lineIndex, int sec)
{
    startTestOperator(lineIndex, sec);
}

void TestController::startTestNoMeasure(int sec)
{
    if (!m_io) return;
    if (sec <= 0) return;

    stopAnyActiveTest();

    log(QString("Тест без измерений на %1 секунд").arg(sec));

    setActive(Active::NoMeas);
    setCurrentTestKind(TestKind::Functional);

    if (!m_io->requestNoMeasTestStart()) {
        setActive(Active::None);
        setSource(Source::None);
        setCurrentTestKind(TestKind::None);
        return;
    }

    m_fireNoMeasureTimer = new QTimer(this);
    m_fireNoMeasureTimer->setSingleShot(true);

    connect(m_fireNoMeasureTimer, &QTimer::timeout, this, [this]() {
        if (m_io)
            m_io->requestNoMeasTestStop();

        log("Тест без измерений завершён");
        cleanupFireNoMeasureTimer();
        setActive(Active::None);
        setSource(Source::None);
        setCurrentTestKind(TestKind::None);
    });

    m_fireNoMeasureTimer->start(sec * 1000);
}

void TestController::stopTest(int lineIndex)
{
    Q_UNUSED(lineIndex);
    stopAnyActiveTest();
    log("Тест принудительно завершён");
}

void TestController::startTestInternal(int lineIndex, int sec)
{
    if (!m_model || !m_io) return;

    const int fullSec = calcAllLinesTestDurationSec();

    if (lineIndex == -1) {
        sec = fullSec;
    } else if (lineIndex > -1) {
        if (sec < minSecTestDuration)
            sec = minSecTestDuration;
    } else if (sec < fullSec) {
        sec = fullSec;
    }

    if (lineIndex < -2 || lineIndex >= m_model->rowCount())
        return;

    const int durationMs = sec * 1000;

    beginFireMeasurements();

    if (lineIndex >= 0)
        startSingleLineTest(lineIndex, durationMs);
    else
        startAllLinesTest(durationMs);

    setCurrentTestKind((sec >= m_longThresholdMin * 60) ? TestKind::Duration : TestKind::Functional);
}

void TestController::stopAnyActiveTest()
{
    if (m_singleTestTimer) {
        m_singleTestTimer->stop();
        cleanupSingleTimer();
    }

    if (m_allTestRunning) {
        m_allTestRunning = false;
        if (m_allTestTimer && m_allTestTimer->isActive())
            m_allTestTimer->stop();
    }

    if (m_fireNoMeasureTimer) {
        m_fireNoMeasureTimer->stop();
        cleanupFireNoMeasureTimer();
    }

    if (m_io) {
        m_io->requestSingleLineTestStop();
        m_io->requestStepTestStop();
        m_io->requestNoMeasTestStop();
    }

    if (m_active != Active::NoMeas) {
        endFireMeasurements();
        allLineStatusReturn();
    }

    m_currentAllIsLong = false;
    setActive(Active::None);
    setSource(Source::None);
    setMeasuredLine(-1);
    setCurrentTestKind(TestKind::None);
}

void TestController::cleanupSingleTimer()
{
    if (!m_singleTestTimer) return;
    m_singleTestTimer->deleteLater();
    m_singleTestTimer = nullptr;
}

void TestController::cleanupFireNoMeasureTimer()
{
    if (!m_fireNoMeasureTimer) return;
    m_fireNoMeasureTimer->deleteLater();
    m_fireNoMeasureTimer = nullptr;
}

void TestController::allLineStatusReturn()
{
    if (!m_model) return;
    const int count = m_model->rowCount();
    for (int i = 0; i < count; ++i) {
        Line *ln = m_model->line(i);
        if (ln && ln->status() == Line::Test)
            ln->setStatus(ln->oldStatus());
    }
}

void TestController::beginFireMeasurements()
{
    if (m_bus) m_bus->setModeTest();
}

void TestController::endFireMeasurements()
{
    if (m_bus) m_bus->setModeNormal();
}

bool TestController::isLineTestable(Line *ln) const
{
    if (!ln) return false;
    if (ln->mode() == Line::NotUsed) return false;
    return true;
}

int TestController::findNextTestableIndex(int fromExclusive) const
{
    const int count = m_model ? m_model->rowCount() : 0;
    for (int i = fromExclusive + 1; i < count; ++i)
        if (isLineTestable(m_model->line(i)))
            return i;
    return -1;
}

int TestController::findFirstTestableIndex() const
{
    return findNextTestableIndex(-1);
}

void TestController::startSingleLineTest(int lineIndex, int durationMs)
{
    Line *line = m_model->line(lineIndex);
    if (!isLineTestable(line)) {
        endFireMeasurements();
        return;
    }

    setActive(Active::Single);
    setCurrentTestKind(TestKind::Functional);

    log(QString("ТЕСТ линия: %1 на %2 секунд")
            .arg(line->description())
            .arg(durationMs / 1000));

    line->setOldStatus(line->status());
    line->setStatus(Line::Test);
    line->setPower(0);

    if (!m_io->requestSingleLineTestStart(lineIndex)) {
        line->setStatus(line->oldStatus());
        endFireMeasurements();
        log(QString("Тест линии %1 не запустился").arg(lineIndex));
        setActive(Active::None);
        setSource(Source::None);
        setCurrentTestKind(TestKind::None);
        return;
    }

    setMeasuredLine(lineIndex);

    m_singleTestTimer = new QTimer(this);
    m_singleTestTimer->setSingleShot(true);

    connect(m_singleTestTimer, &QTimer::timeout, this, [this, line]() {
        const bool ok = checkLinePowerInternal(line);
        line->setStatus(ok ? Line::OK : Line::Failure);
        line->setLastMeasuredTest(QDateTime::currentDateTime());

        log(QString("⏰ ТЕСТ завершён: %1 mpower=%2 power=%3 tol=%4 -> %5")
                .arg(line->description())
                .arg(line->mpower())
                .arg(line->power())
                .arg(line->tolerance())
                .arg(ok ? "OK" : "Неисправность"));

        if (m_model && !m_linesSavePath.isEmpty())
            m_model->saveToFile(m_linesSavePath);

        if (m_io) {
            m_io->requestSingleLineTestStop();
            setMeasuredLine(-1);
        }

        cleanupSingleTimer();
        setActive(Active::None);
        setSource(Source::None);
        setCurrentTestKind(TestKind::None);
        endFireMeasurements();
    });

    m_singleTestTimer->start(durationMs);
    log(QString("Тест линии %1 старт").arg(lineIndex));
}

void TestController::startAllLinesTest(int durationMs)
{
    m_currentAllAllOk = true;
    m_currentAllIsLong = (durationMs >= m_longThresholdMin * 60 * 1000);

    const int first = findFirstTestableIndex();
    if (first < 0) {
        log("ТЕСТ ВСЕХ: нет линий для теста");
        endFireMeasurements();
        return;
    }

    const int count = m_model->rowCount();
    int countFact = 0;
    for (int i = 0; i < count; ++i) {
        Line *ln = m_model->line(i);
        if (isLineTestable(ln)) {
            ln->setOldStatus(ln->status());
            ln->setStatus(Line::Test);
            ln->setPower(0);
            countFact++;
        }
    }

    setActive(Active::All);
    setCurrentTestKind(m_currentAllIsLong ? TestKind::Duration : TestKind::Functional);
    m_allTestRunning = true;
    m_allTestCurrentIndex = first;

    int warmupMs = durationMs - countFact * m_shotTimeMesure * 1000;
    if (warmupMs < m_shotTimeWarm * 1000)
        warmupMs = m_shotTimeWarm * 1000;

    if (!m_io->requestStepTestStart(m_allTestCurrentIndex)) {
        m_allTestRunning = false;
        endFireMeasurements();
        allLineStatusReturn();
        setActive(Active::None);
        setSource(Source::None);
        setCurrentTestKind(TestKind::None);
        return;
    }

    setMeasuredLine(m_allTestCurrentIndex);

    m_allTestTimer->start(warmupMs);
    log(QString("Тест всех линий старт, прогрев %1 ms").arg(warmupMs));
}

void TestController::handleAllLinesStep()
{
    if (!m_allTestRunning || !m_model || !m_io)
        return;

    const int idx = m_allTestCurrentIndex;
    Line *line = (idx >= 0 && idx < m_model->rowCount()) ? m_model->line(idx) : nullptr;

    if (isLineTestable(line)) {
        const bool ok = checkLinePowerInternal(line);
        line->setStatus(ok ? Line::OK : Line::Failure);
        line->setLastMeasuredTest(QDateTime::currentDateTime());
        if (!ok) m_currentAllAllOk = false;

        log(QString("⏰ Линия %1: mpower=%2 power=%3 tol=%4 -> %5")
                .arg(line->description())
                .arg(line->mpower())
                .arg(line->power())
                .arg(m_currentAllIsLong ? line->tolerance() + m_longTolBonus : line->tolerance())
                .arg(ok ? "OK" : "Неиспр."));
    }

    const int nextIdx = findNextTestableIndex(m_allTestCurrentIndex);
    if (nextIdx < 0) {
        m_io->requestStepTestStop();
        log("✅ ТЕСТ всех линий завершён");
        m_allTestRunning = false;
        endFireMeasurements();
        allLineStatusReturn();
        setMeasuredLine(-1);
        setActive(Active::None);
        setSource(Source::None);
        setCurrentTestKind(TestKind::None);

        if (m_currentAllIsLong)
            recordLongSystemTestResult(m_currentAllAllOk);

        if (m_model && !m_linesSavePath.isEmpty())
            m_model->saveToFile(m_linesSavePath);

        m_currentAllIsLong = false;
        return;
    }

    m_allTestCurrentIndex = nextIdx;

    if (!m_io->requestStepTestStart(nextIdx)) {
        m_io->requestStepTestStop();
        log("⚠️ Не удалось включить измерение для следующей линии, прерываем тест");
        m_allTestRunning = false;
        endFireMeasurements();
        allLineStatusReturn();
        setActive(Active::None);
        setSource(Source::None);
        setCurrentTestKind(TestKind::None);
        setMeasuredLine(-1);
        return;
    }

    setMeasuredLine(m_allTestCurrentIndex);
    m_allTestTimer->start(m_shotTimeMesure * 1000);
}

bool TestController::checkLinePowerInternal(Line *line) const
{
    const double ref  = line->mpower();
    const double meas = line->power();
    if (ref <= 0.0) return false;

    double tol = line->tolerance();
    if (m_currentAllIsLong && m_active == Active::All)
        tol = std::min(50.0, tol + m_longTolBonus);

    const double diff    = std::fabs(meas - ref);
    const double maxDiff = ref * tol / 100.0;
    return diff <= maxDiff;
}

void TestController::setMeasuredLine(int idx)
{
    if (m_measuredLine == idx) return;
    m_measuredLine = idx;
    emit measuredLineChanged(idx);
}

void TestController::setSource(Source s)
{
    if (m_source == s) return;
    m_source = s;
    emit testSourceChanged();
}

void TestController::setActive(Active a)
{
    if (m_active == a) return;
    m_active = a;
    emit testActiveChanged();
}

void TestController::recordLongSystemTestResult(bool ok)
{
    m_lastLongSystemTest = QDateTime::currentDateTime();
    m_lastLongSystemTestOk = ok;
    emit lastLongSystemTestChanged();
    log(QString("🧾 Длинный тест шкафа: %1").arg(ok ? "PASS" : "Неиспр."));

    QJsonObject obj;
    obj["lastLongSystemTest"] = m_lastLongSystemTest.toString(Qt::ISODate);
    obj["lastLongSystemTestOk"] = m_lastLongSystemTestOk;

    QFile f(m_maintenancePath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        f.close();
    }
}

void TestController::loadMaintenance()
{
    QFile f(m_maintenancePath);
    if (!f.open(QIODevice::ReadOnly)) return;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;

    const auto obj = doc.object();
    m_lastLongSystemTest = QDateTime::fromString(obj.value("lastLongSystemTest").toString(), Qt::ISODate);
    m_lastLongSystemTestOk = obj.value("lastLongSystemTestOk").toBool(true);
}

int TestController::calcAllLinesTestDurationSec() const
{
    if (!m_model)
        return 0;

    int countFact = 0;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (isLineTestable(m_model->line(i)))
            countFact++;
    }

    return (countFact - 1) * m_shotTimeMesure + m_shotTimeWarm;
}