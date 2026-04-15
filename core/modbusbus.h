#ifndef MODBUSBUS_H
#define MODBUSBUS_H

#include <QObject>
#include <QTimer>
#include <QtSerialBus/QModbusRtuSerialClient>
#include <QtSerialBus/QModbusDataUnit>
#include <deque>
#include <QHash>
#include <cstdint>

class AppConfig;

class ModbusBus : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit ModbusBus(AppConfig *config, QObject *parent = nullptr);
    ~ModbusBus();

    // Настройка шины
    void setPortName(const QString &portName);   // /dev/ttyUSB0 или "COM7"
    void setBaudRate(int baud);                  // 9600
    void setRelayModuleCount(int count);         // 1..8
    void setRelayBaseAddress(int addr);          // 1 (адрес первого модуля реле)
    void setTimeoutMs(int ms);                   // по умолчанию 350
    void setRetries(int n);                      // по умолчанию 0

    // Адреса устройств
    void setInletMeterAddr(int addr);            // ADL200 (default 10)
    void setTestMeterAddr(int addr);             // DJSF1352 (default 11)
    void setSht20Addr(int addr);                 // SHT20 (default 12)

    // SHT20 параметры (только температура)
    void setSht20TempInputReg(int reg);          // default 0x0300
    void setSht20TempScale(double scale);        // default 0.1

    Q_INVOKABLE void connectDevice();
    Q_INVOKABLE void disconnectDevice();
    bool isConnected() const;

    // ===== РЕЛЕ =====
    Q_INVOKABLE void setAllRelaysOff();
    Q_INVOKABLE void setModuleRelaysBits(int moduleIndex, quint8 bits); // CHANGED

    // ===== Режимы опроса =====
    Q_INVOKABLE void setModeNormal();   // 1 сек: входа всех модулей + ADL200 + SHT20
    Q_INVOKABLE void setModeTest();     // 500мс: входа модуля0 + DJSF

    // (опционально) ручной опрос
    Q_INVOKABLE void pollAllInputs();   // round-robin
    Q_INVOKABLE void pollAllRelays();   // round-robin

signals:
    void connectedChanged(bool connected);
    void errorOccurred(const QString &message);

    void relaysUpdated(int moduleIndex, quint8 bits);
    void inputsUpdated(int moduleIndex, quint8 bits);

    // ADL200 inlet (slave 10): U/I/P/F
    void inletMeterUpdated(double U, double I, double P, double F);

    // DJSF1352 test (slave 11): U/I/P
    void testMeterUpdated(double U, double I, double P);

    // SHT20 (slave 12): температура
    void temperatureUpdated(double T);

    void deviceOffline(const QString &name, int address);
    void deviceOnline(const QString &name, int address);

    void busOnline();
    void busOffline(const QString &reason);

private slots:
    void onStateChanged(QModbusDevice::State state);
    void onErrorOccurred(QModbusDevice::Error error);

private:
    // CHANGED: только полная запись 8 катушек
    enum class ReqType { WriteCoils8, ReadInputs, ReadCoils, ReadHolding, ReadInputRegs };
    enum class MeterKind { None, ADL200_Inlet, DJSF_Test, SHT20_Temperature };

    struct Request {
        ReqType type = ReqType::ReadInputs;
        int slaveAddr = 1;

        quint8 coilsBits = 0;   // CHANGED

        int start = 0;
        int count = 8;

        int moduleIndex = 0;
        MeterKind meterKind = MeterKind::None;
    };

    void setupDevice();
    void clearQueues();
    void pump();
    void sendRequest(const Request &r);
    void recreateClient();
    void scheduleReconnect(const QString &reason);
    void handleTransportFailure(const QString &reason);
    void clearPendingWrites(); // CHANGED

    static const char* reqTypeName(ModbusBus::ReqType t) {
        switch (t) {
        case ModbusBus::ReqType::WriteCoils8:   return "WriteCoils (FC0F)";
        case ModbusBus::ReqType::ReadInputs:    return "ReadInputs (FC02)";
        case ModbusBus::ReqType::ReadCoils:     return "ReadCoils (FC01)";
        case ModbusBus::ReqType::ReadHolding:   return "ReadHolding (FC03)";
        case ModbusBus::ReqType::ReadInputRegs: return "ReadInputRegs (FC04)";
        }
        return "?";
    }

    // CHANGED
    static bool sameHighTarget(const ModbusBus::Request &a, const ModbusBus::Request &b)
    {
        if (a.type != b.type) return false;
        if (a.slaveAddr != b.slaveAddr) return false;

        switch (a.type) {
        case ModbusBus::ReqType::WriteCoils8:
            return a.start == b.start && a.count == b.count;
        default:
            return false;
        }
    }

    static bool samePeriodic(const Request& a, const Request& b);
    void enqueueHigh(const Request &r);
    void enqueueNorm(const Request &r);
    void enqueueLow (const Request &r);

    // === тики режимов ===
    void startModeTimers();
    void stopModeTimers();
    void normalTick();
    void testFireTick();
    void testFastTick();

    // === парсинг ===
    static double scaleDjsf(qint16 value, qint16 dp); // value * 10^(dp-3)

    // Очереди приоритетов
    std::deque<Request> m_qHigh;
    std::deque<Request> m_qNorm;
    std::deque<Request> m_qLow;

    bool m_busy = false;

    QModbusRtuSerialClient *m_modbus = nullptr;
    QString m_portName;
    int m_baudRate = 9600;
    int m_timeoutMs = 350;
    int m_retries = 0;

    int m_relayModuleCount = 1;
    int m_relayBaseAddr = 1;

    int m_pollModuleInputs = 0;
    int m_pollModuleRelays = 0;

    // Device addresses
    int m_addrInlet = 10; // ADL200
    int m_addrTest  = 11; // DJSF1352
    int m_addrSht20 = 12; // SHT20

    // SHT20 temperature
    int m_sht20TempReg = 0x0300;
    double m_sht20TempScale = 0.1;

    int m_sht20PollDivider = 0;
    int m_sht20PollEveryTicks = 5;   // раз в 5 normalTick

    enum class Mode { Normal, Test };
    Mode m_mode = Mode::Normal;

    QTimer m_tickNormal;   // 1 сек
    QTimer m_tickTestFast; // 500 мс
    QTimer m_tickTestFire; // 500 мс

    QTimer m_reconnect;
    bool m_busOnline = false;
    bool m_wantConnected = false;
    int m_reconnectMs = 10000;

    int m_consecutiveTransportErrors = 0;
    int m_transportFailThreshold = 10;
    bool m_hadAnySuccessSinceConnect = false;

    QString deviceNameForRequest(const Request &r) const;
    void markRequestSuccess(const Request &r);
    void markRequestFailure(const Request &r);
    void resetAllDeviceStates();

    struct DeviceState
    {
        QString name;
        bool online = true;
        int failCount = 0;
    };
    QHash<int, DeviceState> m_deviceStates;
    int m_failThreshold = 20;
    AppConfig *m_config = nullptr;
};

#endif // MODBUSBUS_H


/*
===============================================================================
ФИЛОСОФИЯ РАБОТЫ ModbusBus
===============================================================================

ModbusBus — это ЕДИНЫЙ мастер Modbus RTU в системе.
Он НИЧЕГО не знает о логике линий, тестов, UI или сценариях.
Его задача — ТОЛЬКО корректный и последовательный обмен по Modbus.

-------------------------------------------------------
1. Разделение ответственности
ModbusBus:
- знает:
  - Serial-порт
  - Modbus RTU
  - адреса slave-устройств
  - форматы регистров
- делает:
  - отправку команд
  - чтение данных
  - разбор "сырых" регистров
  - эмит сигналов с ФИЗИЧЕСКИМИ величинами (U, I, P, T)
- НЕ делает:
  - не управляет Line
  - не знает TestController
  - не меняет UI
  - не принимает решений

Логика приложения (LineIoManager / TestController / main):
- подписывается на сигналы ModbusBus
- решает, КУДА писать данные
- решает, ЧТО считать текущей линией
- решает, КОГДА опрашивать

-------------------------------------------------------
2. Очередь запросов и приоритеты

Все Modbus-запросы проходят через очереди:
- High priority:
  * управление реле
  * аварийные команды
- Normal priority:
  * регулярный опрос входов
  * измерители мощности
- Low priority:
  * второстепенные опросы

В каждый момент времени выполняется ТОЛЬКО ОДИН запрос.
Это:
- исключает коллизии
- упрощает отладку
- делает поведение детерминированным

-------------------------------------------------------
3. Режимы работы (Mode)
-------------------------------------------------------

ModbusBus имеет логические режимы:

- Normal:
  * обычный опрос входов
  * входной измеритель (ADL200)
  * датчики (SHT20)

- Test:
  * ускоренный опрос тестового измерителя (DJSF1352)
  * используется ТОЛЬКО во время тестов

ВАЖНО:
ModbusBus НЕ решает, когда включать Test.
Он выполняет команды:
    setModeNormal()
    setModeTest()

 решает — TestController.

-------------------------------------------------------
4. Обработка измерителей
-------------------------------------------------------

Каждый измеритель:
- читается своим Modbus-запросом
- парсится строго по документации
- масштабируется в ФИЗИЧЕСКИЕ единицы
- отдается наружу сигналом

Пример:
- DJSF1352:
    emit testMeterUpdated(U, I, P);

ModbusBus НЕ знает:
- что это "текущая линия"
- что это "эталон"
- что это "проверка допуска"

-------------------------------------------------------
5. Ошибки и устойчивость
-------------------------------------------------------

- Любая ошибка Modbus:
  * логируется
  * не ломает очередь
  * не останавливает систему

- Потеря порта:
  * ModbusBus отключается
  * внешняя логика решает, что делать дальше



===============================================================================
*/

