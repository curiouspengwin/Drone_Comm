#include "dronenode.h"
#include "packet_struct.h"
#include <mavlink.h> // Official MAVLink v2 header
#include <QNetworkDatagram>
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <cmath>

DroneNode::DroneNode(uint8_t id, uint8_t totalDrones, QObject *parent)
    : QObject(parent), m_id(id), m_totalDrones(totalDrones)
{
    m_posX = id*15.0f;
    m_posY = id*10.0f;
    m_posZ = 10.0f;
    // QObject(nullptr) binds the new object to the thread its in.
}

void DroneNode::start(){

    m_socket = new QUdpSocket(this);
    m_timer = new QTimer(this);

    qDebug() << "Drone" << m_id << "running on thread:" << QThread::currentThread();

    int portNo = m_id + 50000;

    // If follower, listen for peer broadcasts on port 15000 (50002)
    // Share address allows multiple processes in the same computer to listen to 15000 (50002)
    m_socket->bind(QHostAddress::LocalHost, portNo, QUdpSocket::ShareAddress| QUdpSocket::ReuseAddressHint);
    // (sender,&sender::event, receiver,&receiver::on_event)
    connect(m_socket, &QUdpSocket::readyRead, this, &DroneNode::readDatagrams);


    // Run periodic updates at 2 Hz (every 500ms)
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (move){
            m_posX += 1.5f; // Drone moves ahead
            m_posY += std::sin(m_posX*0.1f)*1.5f;
            broadcastStateToPeers();
        }
        sendTelemetryToGCS();
    });

    // Timers across different threads are not synchronised.
    // We are using the timer to just send data every 500ms, so it doesnt matter.
    // There is a possibility to use master clock/timer
    m_timer->start(500); //Starts the timer to fire every 500ms
}

void DroneNode::sendTelemetryToGCS(){

    mavlink_message_t msg;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint32_t bootTimeMs = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF);

    // 1. Pack and send MAVLink HEARTBEAT ("I am alive!")
    mavlink_msg_heartbeat_pack(
        m_id,                        // sysid (Unique Drone ID)
        MAV_COMP_ID_AUTOPILOT1,      // compid (Autopilot component)
        &msg,
        MAV_TYPE_QUADROTOR,          // Vehicle type
        MAV_AUTOPILOT_GENERIC,       // Autopilot type
        MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
        0,
        move ? MAV_STATE_ACTIVE : MAV_STATE_STANDBY
        );

    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    m_socket->writeDatagram(reinterpret_cast<const char*>(buffer), len, QHostAddress::LocalHost, gcsPort);

    // 2. Pack and send MAVLink LOCAL_POSITION_NED (3D Coordinates)
    // Note: In NED (North-East-Down), altitude up is negative Z, so we pass -m_posZ
    mavlink_msg_local_position_ned_pack(
        m_id,
        MAV_COMP_ID_AUTOPILOT1,
        &msg,
        bootTimeMs,
        m_posX,     // X (North)
        m_posY,     // Y (East)
        -m_posZ,    // Z (Down)
        0.0f, 0.0f, 0.0f // vx, vy, vz (We will add real velocities in Phase 4!)
        );
    len = mavlink_msg_to_send_buffer(buffer, &msg);
    m_socket->writeDatagram(reinterpret_cast<const char*>(buffer), len, QHostAddress::LocalHost, gcsPort);

    // SwarmPacket packet{0xFD, m_id, 1, m_posX, m_posY, m_posZ, 95.0f};
    // // This is the core serialization step. A C++ struct is stored as raw bits in RAM.
    // // QByteArray buffer copies it to the buffer
    // QByteArray buffer(reinterpret_cast<const char*>(&packet), sizeof(packet));
    // // Send directly to the GCS listening port
    // m_socket->writeDatagram(buffer, QHostAddress::LocalHost, gcsPort);
}

void DroneNode::broadcastStateToPeers() {
    // Leader broadcasts position to peer drones on port 15000
    // Kept as SwarmPacket temporarily until Phase 3 (Zenoh)
    SwarmPacket d2dPacket{0xFD, m_id, 2, m_posX, m_posY, m_posZ, 0.0f};
    QByteArray buffer(reinterpret_cast<const char*>(&d2dPacket), sizeof(d2dPacket));

    for (uint8_t peerId = 1; peerId <= m_totalDrones; ++peerId){
        if (peerId != m_id){
            m_socket->writeDatagram(buffer, QHostAddress::LocalHost, 50000 + peerId);
        }
    }
}

void DroneNode::readDatagrams(){
    // Each drone thread uses its own MAVLink channel (MAVLINK_COMM_0 is thread-local if we use status struct)
    mavlink_message_t msg;
    mavlink_status_t status;

    while (m_socket->hasPendingDatagrams()){
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();


        // A. Check if it's our temporary D2D SwarmPacket (exact 19 bytes, msgType == 2)
        if (data.size() == sizeof(SwarmPacket)){
            // Deserialization. Mapping back to the struct format
            const auto* packet = reinterpret_cast<const SwarmPacket*>(data.constData());
            if  (packet->magic == 0xFD && packet->msgType == 2 && move){
                // Decentralized collision avoidance rule:
                // If a peer gets within 8 meters, nudge away slightly
                float dx = m_posX - packet->posX;
                float dy = m_posY - packet->posY;
                float dist = std::hypot(dx,dy);
                if (dist<8.0f && dist > 0.01f){
                    m_posX += (dx/dist) * 1.0f;
                    m_posY += (dy/dist) * 1.0f;
                }
                continue;
            }
        }

        // B. Otherwise, parse as a MAVLink v2 message from the GCS!
        for (int i = 0; i < data.size(); ++i){
            if (mavlink_frame_char_buffer(&msg, &status, static_cast<uint8_t>(data[i]), &msg, &status) == MAVLINK_FRAMING_OK)
            {
                if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                    mavlink_command_long_t cmd;
                    mavlink_msg_command_long_decode(&msg, &cmd);

                    // Check if this command is meant for ME and is a Pause/Resume command
                    if (cmd.target_system == m_id && cmd.command == MAV_CMD_DO_PAUSE_CONTINUE) {
                        move = !move;
                        qDebug() << "[Drone" << m_id << "] Received MAVLink Pause/Continue! Moving:" << move;
                    }
                }
            }
        }
    }
}
