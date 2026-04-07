#ifndef LINEIOMANAGER_H
#define LINEIOMANAGER_H

#include <QObject>
#include <QTimer>
#include <QtGlobal>
#include <cstdint>

class ModbusBus;
class LinesModel;

class LineIoManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool fireActive READ fireActive NOTIFY fireChanged)
    Q_PROPERTY(bool programFireActive READ programFireActive NOTIFY programFireChanged)

    Q_PROPERTY(bool stepTestActive READ stepTestActive NOTIFY stepTestActiveChanged)
    Q_PROPERTY(int  stepTestLine READ stepTestLine NOTIFY stepTestLineChanged)

    Q_PROPERTY(bool singleLineTestActive READ singleLineTestActive NOTIFY singleLineTestActiveChanged)
    Q_PROPERTY(int  singleLineTestLine READ singleLineTestLine NOTIFY singleLineTestLineChanged)

    Q_PROPERTY(bool fireInput READ fireInput NOTIFY fireChanged)
    Q_PROPERTY(bool doorOpen READ doorOpen NOTIFY doorOpenChanged)

public:
    explicit LineIoManager(QObject *parent = nullptr);

    void bind(ModbusBus *bus, LinesModel *linesModel, int numLines);

    Q_SLOT void onInputsUpdated(int moduleIndex, quint8 bits);
    Q_SLOT void onRelaysUpdated(int moduleIndex, quint8 bits); // CHANGED

    Q_SLOT void onBusOnline();                                 // CHANGED
    Q_SLOT void onBusOffline(const QString &reason);           // CHANGED

    Q_INVOKABLE void forceApplyAll();

    // программный пожар
    Q_INVOKABLE void setProgramFire(bool on);
    Q_INVOKABLE void stopProgramFire();

    // тест без измерений всех линий
    Q_INVOKABLE bool requestNoMeasTestStart();
    Q_INVOKABLE void requestNoMeasTestStop();

    // тест одной линии с измерением
    Q_INVOKABLE bool requestSingleLineTestStart(int lineIndex);
    Q_INVOKABLE void requestSingleLineTestStop();

    // тест всех линий с измерением по шагам
    Q_INVOKABLE bool requestStepTestStart(int lineIndex);
    Q_INVOKABLE void requestStepTestStop();

    // совместимость со старым кодом
    Q_INVOKABLE bool requestFireTestStart(int lineIndex) { return requestStepTestStart(lineIndex); }
    Q_INVOKABLE void requestFireTestStop() { requestStepTestStop(); }

    bool fireActive() const { return m_fireInput || m_programFire; }
    bool programFireActive() const { return m_programFire; }

    bool fireInput() const { return m_fireInput; }

    bool stepTestActive() const { return m_stepTestActive; }
    int stepTestLine() const { return m_stepTestLine; }

    bool singleLineTestActive() const { return m_singleTestActive; }
    int singleLineTestLine() const { return m_singleTestLine; }

    // совместимость со старым UI
    bool fireTestActive() const { return m_stepTestActive; }
    int fireTestLine() const { return m_stepTestLine; }

    void setAlarmLamp(bool on);

    bool noMeasTestActive() const { return m_noMeasTestActive; }
    bool doorOpen() const { return m_doorOpen; }

signals:
    void fireChanged(bool active);
    void programFireChanged(bool active);

    void stepTestActiveChanged(bool active);
    void stepTestLineChanged(int lineIndex);

    void singleLineTestActiveChanged(bool active);
    void singleLineTestLineChanged(int lineIndex);

    // совместимость со старым UI
    void fireTestActiveChanged(bool active);
    void fireTestLineChanged(int lineIndex);

    void emergencyStop();

    void programFireOnRequested();
    void programFireOffRequested();
    void doorOpenChanged(bool open);

private:
    enum class Mode {
        Normal,
        Fire,
        NoMeasTest,
        SingleLineTest,
        StepTest
    };

    static constexpr int MAX_MODULES = 7;
    static constexpr int MODULE0 = 0;

    // module0 inputs bits
    static constexpr int IN_FIRE     = 0; // IN1
    static constexpr int IN_PROG_FIRE_ON  = 1; // IN2 программный пожар ВКЛ
    static constexpr int IN_PROG_FIRE_OFF = 2; // IN3 программный пожар ВЫКЛ
    static constexpr int IN_DOOR          = 3; // IN4 дверь

    static constexpr int IN_M0_LINE0 = 5; // IN6
    static constexpr int IN_M0_LINE1 = 6; // IN7
    static constexpr int IN_M0_LINE2 = 7; // IN8

    // module0 relay bits
    static constexpr int REL_SERV_0 = 0; // служебная коммутация
    static constexpr int REL_SERV_1 = 1; // лампа авария

private:
    void updateFireFromModule0(quint8 bits0);
    void processProgramFireButtons(quint8 oldBits, quint8 newBits);
    void updateDoorFromModule0(quint8 bits0);
    void cancelTestsDueToEmergency();

    Mode currentMode() const;

    void recomputeDesiredAll();
    void fillFireMode();
    void fillNoMeasTestMode();
    void fillSingleLineTestMode(int lineIndex);
    void fillStepTestMode(int lineIndex);
    void fillNormalMode();

    bool wantLineOn(int lineIndex) const;

    void applyAllModules(bool force);
    void applyModuleIfChanged(int moduleIndex, bool force);

    bool mapLineToRelayBits(int lineIndex, int &moduleIndex, int &bitMeas, int &bitWork) const;
    bool mapLineToInputBit(int lineIndex, int &moduleIndex, int &inputBit) const;
    void syncLineStatesFromDesired();

    static inline bool bit(quint8 v, int b)
    {
        return ((v >> b) & 0x01) != 0;
    }

    static inline void setBit(quint8 &v, int b, bool on)
    {
        if (on) v |=  (quint8(1u) << b);
        else    v &= ~(quint8(1u) << b);
    }

private:
    ModbusBus  *m_bus = nullptr;
    LinesModel *m_lines = nullptr;
    int m_numLines = 0;

    quint8 m_lastInputs[MAX_MODULES];
    quint8 m_desiredRelays[MAX_MODULES];
    quint8 m_lastSentRelays[MAX_MODULES]; // CHANGED: теперь это "последнее подтверждённое состояние"

    // аппаратный пожар
    bool m_fireInput = false;

    // программный пожар
    bool m_programFire = false;

    // тесты
    bool m_noMeasTestActive = false;

    bool m_stepTestActive = false;
    int  m_stepTestLine = -1;

    bool m_singleTestActive = false;
    int  m_singleTestLine = -1;

    QTimer *m_StepSwich = nullptr;
    int m_twoStepDelayMs = 300;
    int m_lastMes = -1;

    bool m_alarmLampOn = false;
    bool m_doorOpen = false;

    enum class TwoStepKind { None, Step1, Step2, Step3 };
    TwoStepKind m_twoStepKind = TwoStepKind::None;

    void stopStepSwichTimers()
    {
        if (m_StepSwich)
            m_StepSwich->stop();

        m_twoStepKind = TwoStepKind::None;
        m_lastMes = -1;
    }
};

#endif // LINEIOMANAGER_H