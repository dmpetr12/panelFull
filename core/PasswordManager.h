// PasswordManager.h
#pragma once
#include <QObject>
#include <QSettings>

class PasswordManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)

public:
    explicit PasswordManager(QObject* parent = nullptr)
        : QObject(parent), settings("MyCompany","Panel") {}

    QString password() const {
        return settings.value("password", "1234").toString(); // стартовое значение
    }

    void setPassword(const QString& p) {
        if (p != password()) {
            settings.setValue("password", p);
            emit passwordChanged();
        }
    }

signals:
    void passwordChanged();

private:
    QSettings settings;
};

