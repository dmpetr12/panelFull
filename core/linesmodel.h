#ifndef LINESMODEL_H
#define LINESMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "line.h"

class LinesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int systemState READ systemState NOTIFY systemStateChanged)
public:

    enum LineRoles {
        DescriptionRole = Qt::UserRole + 1,
        PowerRole,
        MPowerRole,
        CurrentRole,
        VoltageRole,
        ToleranceRole,
        ModeRole,
        StatusRole,
        LineStateRole
    };

    explicit LinesModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return m_lines.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
            return QVariant();

        Line *line = m_lines.at(index.row());
        switch (role) {
        case DescriptionRole: return line->description();
        case PowerRole: return line->power();
        case VoltageRole: return line->voltage();
        case MPowerRole: return line->mpower();
        case CurrentRole: return line->current();
        case ToleranceRole: return line->tolerance();
        case ModeRole: return line->mode();
        case StatusRole: return line->status();
        case LineStateRole: return line->lineState();
        default: return QVariant();
        }
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
            return false;

        Line *line = m_lines.at(index.row());
        switch (role) {
        case DescriptionRole: line->setDescription(value.toString()); break;
        case PowerRole: line->setPower(value.toDouble()); break;
        case MPowerRole: line->setmPower(value.toDouble()); break;
        case CurrentRole: line->setCurrent(value.toDouble()); break;
        case VoltageRole: line->setVoltage(value.toDouble()); break;
        case ToleranceRole: line->setTolerance(value.toDouble()); break;
        case ModeRole: line->setMode(static_cast<Line::Mode>(value.toInt())); break;
        case StatusRole: line->setStatus(static_cast<Line::Status>(value.toInt())); break;
        case LineStateRole: line->setLineState(static_cast<Line::LineState>(value.toInt())); break;
        default: return false;
        }

        emit dataChanged(index, index, { role });
        if (role == StatusRole || role == ModeRole)
            recomputeSystemState();
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[DescriptionRole] = "description";
        roles[PowerRole] = "power";
        roles[MPowerRole] = "mpower";
        roles[CurrentRole] = "current";
        roles[VoltageRole] = "voltage";
        roles[ToleranceRole] = "tolerance";
        roles[ModeRole] = "mode";
        roles[StatusRole] = "status";
        roles[LineStateRole] = "lineState";
        return roles;
    }
    Q_INVOKABLE Line* line(int index) const {
        if (index < 0 || index >= m_lines.size()) return nullptr;
        return m_lines.at(index);
    }

    Q_INVOKABLE void addLine(const QString &desc,
                             double power,
                             double mpower,
                             double current,
                             double voltage,
                             double tolerance,
                             Line::Mode mode,
                             Line::Status status,
                             Line::LineState lineState)
    {
        beginInsertRows(QModelIndex(), m_lines.size(), m_lines.size());
        Line *line = new Line(this);
        line->setDescription(desc);
        line->setPower(power);
        line->setmPower(mpower);
        line->setCurrent(current);
        line->setVoltage(voltage);
        line->setTolerance(tolerance);
        line->setMode(mode);
        line->setStatus(status);
        line->setLineState(lineState);
        hookLineSignals(line);
        m_lines.append(line);
        endInsertRows();
        recomputeSystemState();
        line->setLastMeasuredTest(QDateTime());
    }
    Q_INVOKABLE void removeLine(int index) {
        if (index < 0 || index >= m_lines.count())
            return;
        beginRemoveRows(QModelIndex(), index, index);
        delete m_lines.takeAt(index);
        endRemoveRows();
        recomputeSystemState();
    }

    Q_INVOKABLE void clear() {
        beginResetModel();
        qDeleteAll(m_lines);
        m_lines.clear();
        endResetModel();
        recomputeSystemState();
    }

    Q_INVOKABLE void updateLine(int row,
                                const QString &desc,
                                double power,
                                double mpower,
                                double current,
                                double voltage,
                                double tolerance,
                                int mode,
                                int status,
                                int lineState)
    {
        if (row < 0 || row >= m_lines.size()) return;
        QModelIndex idx = index(row);
        setData(idx, desc, DescriptionRole);
        setData(idx, power, PowerRole);
        setData(idx, mpower, MPowerRole);
        setData(idx, current, CurrentRole);
        setData(idx, voltage, VoltageRole);
        setData(idx, tolerance, ToleranceRole);
        setData(idx, mode, ModeRole);
        setData(idx, status, StatusRole);
        setData(idx, lineState, LineStateRole);
    }

    // 🔹 Сохранение в JSON файл
    Q_INVOKABLE bool saveToFile(const QString &filePath) {
        QJsonArray array;
        for (Line *line : m_lines) {
            QJsonObject obj;
            obj["description"] = line->description();
            obj["power"] = line->power();
            obj["mpower"] = line->mpower();
            obj["current"] = line->current();
            obj["voltage"] = line->voltage();
            obj["tolerance"] = line->tolerance();
            obj["mode"] = line->mode();
            obj["status"] = line->status();
            obj["lineState"] = line->lineState();
            obj["lastMeasuredTest"] = line->lastMeasuredTest().toString(Qt::ISODate);
            array.append(obj);
        }
        QJsonDocument doc(array);

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        file.write(doc.toJson());
        return true;
    }

    // 🔹 Загрузка из JSON файла
    Q_INVOKABLE bool loadFromFile(const QString &filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isArray())
            return false;

        beginResetModel();
        qDeleteAll(m_lines);
        m_lines.clear();

        QJsonArray array = doc.array();
        for (const QJsonValue &val : array) {
            QJsonObject obj = val.toObject();
            Line *line = new Line(this);
            hookLineSignals(line);
            line->setDescription(obj["description"].toString());
            line->setPower(obj["power"].toDouble());
            line->setmPower(obj["mpower"].toDouble());
            line->setCurrent(obj["current"].toDouble());
            line->setVoltage(obj["voltage"].toDouble());
            line->setTolerance(obj["tolerance"].toDouble());
            line->setMode(static_cast<Line::Mode>(obj["mode"].toInt()));
            line->setStatus(static_cast<Line::Status>(obj["status"].toInt()));
            line->setLineState(static_cast<Line::LineState>(obj["lineState"].toInt()));
            line->setLastMeasuredTest(QDateTime::fromString(obj["lastMeasuredTest"].toString(), Qt::ISODate));
            m_lines.append(line);
        }
        endResetModel();
        recomputeSystemState();
        return true;
    }

    int systemState() const { return m_systemState; }
signals:
    void systemStateChanged();

private:
    void hookLineSignals(Line *line)
    {    // при уничтожении line связи удалятся автоматически (context = this)
        connect(line, &Line::descriptionChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row < 0) return;
            emit dataChanged(index(row), index(row), { DescriptionRole });
        });

        connect(line, &Line::powerChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row < 0) return;
            emit dataChanged(index(row), index(row), { PowerRole });
        });

        connect(line, &Line::mpowerChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row < 0) return;
            emit dataChanged(index(row), index(row), { MPowerRole });
        });

        connect(line, &Line::currentChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row < 0) return;
            emit dataChanged(index(row), index(row), { CurrentRole });
        });

        connect(line, &Line::voltageChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row < 0) return;
            emit dataChanged(index(row), index(row), { VoltageRole });
        });

        connect(line, &Line::toleranceChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row < 0) return;
            emit dataChanged(index(row), index(row), { ToleranceRole });
        });


        connect(line, &Line::lineStateChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row < 0) return;
            emit dataChanged(index(row), index(row), { LineStateRole });
        });

        connect(line, &Line::statusChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row >= 0)
                emit dataChanged(index(row), index(row), { StatusRole });
            recomputeSystemState();
        });

        connect(line, &Line::modeChanged, this, [this, line](){
            int row = m_lines.indexOf(line);
            if (row >= 0)
                emit dataChanged(index(row), index(row), { ModeRole });
            recomputeSystemState();
        });
    }

    void recomputeSystemState()
    {   // приоритет: Test > Failure > Work
        bool anyTest = false;
        bool anyFailure = false;

        for (Line* ln : m_lines) {
            if (!ln) continue;
            if (ln->mode() == Line::NotUsed) continue;

            const auto st = ln->status();

            if (st == Line::Test) {
                anyTest = true;
                break;
            }
            if (st == Line::Failure  || st == Line::Undefined) {
                anyFailure = true;
            }
        }

        int newState = 0;              // Work
        if (anyTest) newState = 2;     // Test
        else if (anyFailure) newState = 1; // Emergency

        if (m_systemState != newState) {
            m_systemState = newState;
            emit systemStateChanged();
        }
    }

private:
    QList<Line*> m_lines;
    int m_systemState = 0;
};

#endif // LINESMODEL_H
