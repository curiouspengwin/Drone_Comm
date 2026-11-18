#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QMap>
#include <QVariantList>
#include <QVariantMap>
#include "packet_struct.h"

class GcsManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList drones READ drones NOTIFY dronesUpdated);

public:
    explicit GcsManager(QObject *parent = nullptr) : QObject(parent){
        //Bind to standard GCS port
        m_socket.bind(QHostAddress::LocalHost, 50000, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        connect(&m_socket, &QUdpSocket::readyRead, this, &GcsManager::processDatagrams);
    }

    QVariantList drones() const{
        QVariantList list;
        // Data of all drones appended and sent to QML
        for (const auto& droneData : m_drones){
            list.append(droneData);
        }
        return list;
    }

    Q_INVOKABLE void sendCommand(int id){
        // qDebug() << "Called from qml for id: "<< m_id;
        uint8_t uid = static_cast<uint8_t>(id);
        SwarmPacket g2dPacket{0xFD, uid, 3, 0, 0, 0, 0.0f};
        QByteArray buffer(reinterpret_cast<const char*>(&g2dPacket), sizeof(g2dPacket));
        m_socket.writeDatagram(buffer, QHostAddress::LocalHost, 50000+uid);
    }

signals:
    void dronesUpdated();

private slots:
    void processDatagrams() {
        while (m_socket.hasPendingDatagrams()) {
            QByteArray data = m_socket.receiveDatagram().data();
            if (data.size() == sizeof(SwarmPacket)) {
                const auto* p = reinterpret_cast<const SwarmPacket*>(data.constData());
                if (p->magic == 0xFD && p->msgType == 1) {
                    QVariantMap info;
                    info["id"] = p->sysid;
                    info["x"] = p->posX;
                    info["y"] = p->posY;
                    info["z"] = p->posZ;
                    info["battery"] = p->battery;
                    // Update induvidual drone data
                    m_drones[p->sysid] = info;
                    emit dronesUpdated();
                }
            }
        }
    }

// public slots:
//     void buttonClicked(int m_id){
//         sendCommand(m_id);
//     }

private:
    QUdpSocket m_socket;
    QMap<int, QVariantMap> m_drones; // Key: sysid, Value: Drone properties
};
