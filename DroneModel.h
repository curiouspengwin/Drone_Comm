#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QList>

struct DroneInfo {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool active = true;
};

class DroneModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        XRole,
        YRole,
        ZRole,
        ActiveRole
    };

    explicit DroneModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_drones.size();
    }

    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() >= m_drones.size()) return {};
        const auto &d = m_drones[index.row()];
        switch (role) {
        case IdRole:     return d.id;
        case XRole:      return d.x;
        case YRole:      return d.y;
        case ZRole:      return d.z;
        case ActiveRole: return d.active;
        }
        return {};
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            {IdRole,     "droneId"},
            {XRole,      "posX"},
            {YRole,      "posY"},
            {ZRole,      "posZ"},
            {ActiveRole, "active"}
        };
    }

    void updatePosition(int sysid, float x, float y, float z) {
        int row = ensureDrone(sysid);
        m_drones[row].x = x;
        m_drones[row].y = y;
        m_drones[row].z = z;
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {XRole, YRole, ZRole});
    }

    void updateActive(int sysid, bool active) {
        int row = ensureDrone(sysid);
        if (m_drones[row].active != active) {
            m_drones[row].active = active;
            QModelIndex idx = index(row);
            emit dataChanged(idx, idx, {ActiveRole});
        }
    }

signals:
    void countChanged();

private:
    int ensureDrone(int sysid) {
        if (m_idToRow.contains(sysid)) {
            return m_idToRow[sysid];
        }
        int newRow = m_drones.size();
        beginInsertRows(QModelIndex(), newRow, newRow);
        m_drones.append({sysid, 0.0f, 0.0f, 0.0f, true});
        m_idToRow[sysid] = newRow;
        endInsertRows();
        emit countChanged();
        return newRow;
    }

    QList<DroneInfo> m_drones;
    QHash<int, int> m_idToRow;
};