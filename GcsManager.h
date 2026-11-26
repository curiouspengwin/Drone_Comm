#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
// #include <QMap>
// #include <QVariantList>
// #include <QVariantMap>
// #include "packet_struct.h"
#include <mavlink.h>
#include "DroneModel.h"

class GcsManager : public QObject {
    Q_OBJECT
    // Q_PROPERTY(QVariantList drones READ drones NOTIFY dronesUpdated);
    // Q_PROPERTY(QVariantList droneIds READ droneIds NOTIFY droneIdsUpdated)
    Q_PROPERTY(DroneModel* droneModel READ droneModel CONSTANT)

public:
    explicit GcsManager(QObject *parent = nullptr) : QObject(parent){
        //Bind to standard GCS port
        if (!m_socket.bind(QHostAddress::LocalHost, 50000, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            qWarning() << "Failed to bind GCS socket:" << m_socket.errorString();
        }
        // m_socket.bind(QHostAddress::LocalHost, 50000, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        connect(&m_socket, &QUdpSocket::readyRead, this, &GcsManager::processDatagrams);
    }

    DroneModel* droneModel() {return &m_droneModel;}

    // QVariantList drones() const{
    //     QVariantList list;
    //     // Data of all drones appended and sent to QML
    //     for (const auto& droneData : m_drones){
    //         list.append(droneData);
    //     }
    //     return list;
    // }

    // QVariantList droneIds() const {
    //     QVariantList ids;
    //     for (int id : m_drones.keys()) {
    //         ids.append(id);
    //     }
    //     return ids;
    // }

    Q_INVOKABLE void sendCommand(int targetId){
        // qDebug() << "Called from qml for id: "<< m_id;
        mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

        // Standard GCS System ID is 255
        mavlink_msg_command_long_pack(
            255,                        // GCS sysid
            MAV_COMP_ID_MISSIONPLANNER, // GCS compid
            &msg,
            static_cast<uint8_t>(targetId), // Target drone sysid
            MAV_COMP_ID_AUTOPILOT1,         // Target component
            MAV_CMD_DO_PAUSE_CONTINUE,      // Official MAVLink command!
            0,                              // Confirmation
            0, 0, 0, 0, 0, 0, 0             // Params 1-7 (unused for toggle)
            );

        uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
        m_socket.writeDatagram(reinterpret_cast<const char*>(buffer), len, QHostAddress::LocalHost, 50000 + targetId);
        // uint8_t uid = static_cast<uint8_t>(id);
        // SwarmPacket g2dPacket{0xFD, uid, 3, 0, 0, 0, 0.0f};
        // QByteArray buffer(reinterpret_cast<const char*>(&g2dPacket), sizeof(g2dPacket));
        // m_socket.writeDatagram(buffer, QHostAddress::LocalHost, 50000+uid);
    }

// signals:
//     void dronesUpdated();
//     void droneIdsUpdated();

private slots:
    void processDatagrams() {

        // Initialize msg and status to avoid garbage data
        mavlink_message_t msg{};
        mavlink_status_t status{};

        while (m_socket.hasPendingDatagrams()) {
            QByteArray data = m_socket.receiveDatagram().data();

            for (int i =0; i<data.size(); ++i){
                if (mavlink_frame_char_buffer(&msg, &status, static_cast<uint8_t>(data[i]), &msg, &status) == MAVLINK_FRAMING_OK)
                {
                    uint8_t sysid = msg.sysid;

                    //Ignore our own GCS packets if any loop back
                    if (sysid == 255) continue;

                    // Ensure drone entry exists in our map
                    // if(!m_drones.contains(sysid)){
                    //     QVariantMap newDrone;
                    //     newDrone["id"] = sysid;
                    //     newDrone["x"] = 0.0f;
                    //     newDrone["y"] = 0.0f;
                    //     newDrone["z"] = 0.0f;
                    //     newDrone["active"] = true;
                    //     m_drones[sysid] = newDrone;
                    //     emit droneIdsUpdated();
                    // }

                    // Handle specific MAVLink messages
                    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                        mavlink_heartbeat_t hb;
                        mavlink_msg_heartbeat_decode(&msg, &hb);
                        m_droneModel.updateActive(sysid, hb.system_status == MAV_STATE_ACTIVE);
                        // m_drones[sysid]["active"] = (hb.system_status == MAV_STATE_ACTIVE);
                        // emit dronesUpdated();
                    }
                    else if (msg.msgid == MAVLINK_MSG_ID_LOCAL_POSITION_NED) {
                        mavlink_local_position_ned_t pos;
                        mavlink_msg_local_position_ned_decode(&msg, &pos);
                        m_droneModel.updatePosition(sysid, pos.x, pos.y, -pos.z);
                        // m_drones[sysid]["x"] = pos.x;
                        // m_drones[sysid]["y"] = pos.y;
                        // m_drones[sysid]["z"] = -pos.z; // Convert NED Down back to Altitude Up
                        // emit dronesUpdated();
                    }
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
    // QMap<int, QVariantMap> m_drones; // Key: sysid, Value: Drone properties
    DroneModel m_droneModel;
};
