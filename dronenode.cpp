#include "dronenode.h"
#include "packet_struct.h"
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

    SwarmPacket packet{0xFD, m_id, 1, m_posX, m_posY, m_posZ, 95.0f};
    // This is the core serialization step. A C++ struct is stored as raw bits in RAM.
    // QByteArray buffer copies it to the buffer
    QByteArray buffer(reinterpret_cast<const char*>(&packet), sizeof(packet));
    // Send directly to the GCS listening port
    m_socket->writeDatagram(buffer, QHostAddress::LocalHost, gcsPort);
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

    while (m_socket->hasPendingDatagrams()){
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();


        // A. Check if it's our temporary D2D SwarmPacket (exact 19 bytes, msgType == 2)
        if (datagram.data().size() == sizeof(SwarmPacket)){
            // Deserialization. Mapping back to the struct format
            const auto* packet = reinterpret_cast<const SwarmPacket*>(datagram.data().constData());
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
            }
            if  (packet->msgType == 3){
                // If received stop command, stop and resume the drone
                move = !move;
            }
        }
    }
}
