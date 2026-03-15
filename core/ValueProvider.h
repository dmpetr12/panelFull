#ifndef VALUEPROVIDER_H
#define VALUEPROVIDER_H

#include <QObject>

class ValueProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(double value READ value NOTIFY valueChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
    explicit ValueProvider(QObject *parent = nullptr)
        : QObject(parent)
    {}

    double value() const { return m_value; }
    bool valid()  const { return m_valid; }

    Q_INVOKABLE void setValue(double v) {
        m_value = v;
        if (!m_valid) {
            m_valid = true;
            emit validChanged();
        }
        emit valueChanged();
    }

    Q_INVOKABLE void reset() {
        if (!m_valid) return;
        m_valid = false;
        emit validChanged();
    }

signals:
    void valueChanged();
    void validChanged();

private:
    double m_value = 0.0;
    bool   m_valid = false;
};

#endif // VALUEPROVIDER_H
