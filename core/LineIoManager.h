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
    Q_PROPERTY(bool forcedFireActive READ forcedFireActive NOTIFY forcedFireChanged)
    Q_PROPERTY(bool dispatcherActive READ dispatcherActive NOTIFY dispatcherActiveChanged)
    Q_PROPERTY(bool stopActive READ stopActive NOTIFY stopChanged)
    Q_PROPERTY(bool fireTestActive READ fireTestActive NOTIFY fireTestActiveChanged)
    Q_PROPERTY(int  fireTestLine READ fireTestLine NOTIFY fireTestLineChanged)
    Q_PROPERTY(bool singleLineTestActive READ singleLineTestActive NOTIFY singleLineTestActiveChanged)
    Q_PROPERTY(int  singleLineTestLine READ singleLineTestLine NOTIFY singleLineTestLineChanged)
    Q_PROPERTY(bool fireInput READ fireInput NOTIFY fireChanged)

public:
    explicit LineIoManager(QObject *parent = nullptr);

    void bind(ModbusBus *bus, LinesModel *linesModel, int numLines);

    Q_SLOT void onInputsUpdated(int moduleIndex, quint8 bits);

    Q_INVOKABLE void forceApplyAll();

    // внешние аварийные команды
    Q_INVOKABLE void setForcedFire(bool on);
    Q_INVOKABLE void setForcedStop(bool on);

    // TestController API
    Q_INVOKABLE bool requestFireTestStart(int lineIndex);
    Q_INVOKABLE void requestFireTestStop();
    bool requestSingleLineTestStart(int lineIndex);
    void requestSingleLineTestStop();

    bool fireActive() const { return m_fireInput || m_forcedFireCommand; }
    bool forcedFireActive() const { return m_forcedFireCommand; }
    bool stopActive() const { return m_stopInput || m_forcedStopCommand; }
    bool emergencyActive() const { return fireActive() || stopActive(); }

    bool fireInput() const { return m_fireInput; }
    bool dispatcherActive() const { return m_dispatcherForce; }

    bool fireTestActive() const { return m_fireTestActive; }
    int fireTestLine() const { return m_fireTestLine; }

    bool singleLineTestActive() const { return m_singleTestActive; }
    int singleLineTestLine() const { return m_singleTestLine; }

    void setAlarmLamp(bool on);

signals:
    void fireChanged(bool active);
    void dispatcherActiveChanged(bool active);
    void stopChanged(bool active);
    void emergencyStop();

    void fireTestActiveChanged(bool active);
    void forcedFireChanged(bool active);
    void fireTestLineChanged(int lineIndex);

    void singleLineTestActiveChanged(bool active);
    void singleLineTestLineChanged(int lineIndex);

private:
    static constexpr int MAX_MODULES = 7;
    static constexpr int MODULE0 = 0;

    // module0 inputs bits
    static constexpr int IN_FIRE     = 0; // IN1
    static constexpr int IN_STOP     = 1; // IN2
    static constexpr int IN_M0_LINE0 = 5; // IN6
    static constexpr int IN_M0_LINE1 = 6; // IN7
    static constexpr int IN_M0_LINE2 = 7; // IN8

    // module0 relay bits
    static constexpr int REL_SERV_0 = 0; // служебная коммутация
    static constexpr int REL_SERV_1 = 1; // лампа авария

private:
    void updateFireStopFromModule0(quint8 bits0);
    void cancelTestsDueToEmergency();

    void recomputeDesiredAll();
    void fillFireMode();
    void fillFireTestMode(int lineIndex);
    void fillSingleLineTestMode(int lineIndex);
    void fillStopMode();
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
    quint8 m_lastSentRelays[MAX_MODULES];

    // аппаратные входы
    bool m_fireInput = false;
    bool m_stopInput = false;

    // внешние/программные команды
    bool m_forcedFireCommand = false;
    bool m_forcedStopCommand = false;

    bool m_dispatcherForce = false;

    // служебный флаг тестов
    bool m_testForceFire = false;

    bool m_fireTestActive = false;
    int  m_fireTestLine = -1;

    bool m_singleTestActive = false;
    int  m_singleTestLine = -1;

    QTimer *m_StepSwich = nullptr;
    int m_twoStepDelayMs = 200;
    int m_lastMes = -1;

    bool m_alarmLampOn = false;

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