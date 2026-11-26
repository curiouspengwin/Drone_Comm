#include "dronenode.h"
// #include "packet_struct.h"
#include <mavlink.h> // Official MAVLink v2 header
#include <QNetworkDatagram>
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <cmath>

// C-style callback triggered by Zenoh whenever ANY drone publishes to "swarm/drone/*/pose"

static void zenohDataHandler(z_loaned_sample_t *sample, void *arg){
    DroneNode *self = static_cast<DroneNode*>(arg);
    const z_loaned_bytes_t *payload = z_sample_payload(sample); // Payload size

    if (z_bytes_len(payload) == sizeof(PeerPose)){
        PeerPose pose;
        // z_bytes_slice_iterator_t it = z_bytes_get_slice_iterator(payload);
        z_bytes_reader_t reader = z_bytes_get_reader(payload);
        z_bytes_reader_read(&reader, reinterpret_cast<uint8_t*>(&pose), sizeof(PeerPose));

        // Safely hand off the data from Zenoh's thread to this DroneNode's QThread
        QMetaObject::invokeMethod(self, "onPeerPoseReceived", Qt::QueuedConnection,
                                  Q_ARG(uint8_t, pose.senderId),
                                  Q_ARG(float, pose.x),
                                  Q_ARG(float, pose.y),
                                  Q_ARG(float, pose.z));
    }
}

DroneNode::DroneNode(uint8_t id, QObject *parent)
    : QObject(parent), m_id(id)
{
    float angle = id * 1.5f;
    m_pos = QVector3D(std::cos(angle) * 20.0f, std::sin(angle) * 20.0f, 10.0f + id * 3.0f);
    m_vel = QVector3D(0.0f, 0.0f, 0.0f);
    // QObject(nullptr) binds the new object to the thread its in.
}

DroneNode::~DroneNode(){
    if(m_zenohReady){
        z_drop(z_move(m_zSub));
        z_drop(z_move(m_zPub));
        z_drop(z_move(m_zSession));
    }
}

void DroneNode::start(){

    m_socket = new QUdpSocket(this);

    /*Qt's meta-object system automatically knows standard C++ types like int and float,
     * but fixed-width integer typedefs like uint8_t must be registered manually before
     * Qt can serialize them across threads.*/

    qRegisterMetaType<uint8_t>("uint8_t");

    qDebug() << "Drone" << m_id << "running on thread:" << QThread::currentThread();

    int portNo = m_id + 50000;

    // If follower, listen for peer broadcasts on port 15000 (50002)
    // Share address allows multiple processes in the same computer to listen to 15000 (50002)
    m_socket->bind(QHostAddress::LocalHost, portNo, QUdpSocket::ShareAddress| QUdpSocket::ReuseAddressHint);
    // (sender,&sender::event, receiver,&receiver::on_event)
    connect(m_socket, &QUdpSocket::readyRead, this, &DroneNode::readGcsDatagrams);

    // Initialize Zenoh Peer-to-Peer Session (Drone <-> Drone)
    z_owned_config_t config;
    z_config_default(&config);

    if (z_open(&m_zSession, z_move(config), nullptr) == 0) {
        // Declare Publisher: "swarm/drone/<id>/pose"
        QByteArray pubTopic = QString("swarm/drone/%1/pose").arg(m_id).toUtf8();
        z_view_keyexpr_t pubKey;
        z_view_keyexpr_from_str(&pubKey, pubTopic.constData());
        z_declare_publisher(z_loan(m_zSession), &m_zPub, z_loan(pubKey), nullptr);

        // Declare Wildcard Subscriber: "swarm/drone/*/pose"
        // Wildcard: Subscriber with * or **. Used to receive data from multiple matching keys.
        z_view_keyexpr_t subKey;
        z_view_keyexpr_from_str(&subKey, "swarm/drone/*/pose");
        z_owned_closure_sample_t callback;
        z_closure(&callback, zenohDataHandler, nullptr, this);
        z_declare_subscriber(z_loan(m_zSession), &m_zSub, z_loan(subKey), z_move(callback), nullptr);

        m_zenohReady = true;
        qDebug() << "[Drone" << m_id << "] Zenoh P2P connected on topic:" << pubTopic;
    } else {
        qWarning() << "[Drone" << m_id << "] Failed to open Zenoh session!";
    }

    // Run smooth 20Hz physics loop (every 50ms)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this ,&DroneNode::updatePhysicsAndSwarm);
    m_timer->start(50);

    // Run periodic updates at 2 Hz (every 500ms)
    // m_timer = new QTimer(this);
    // connect(m_timer, &QTimer::timeout, this, [this]() {
    //     if (move){
    //         m_posX += 1.5f; // Drone moves ahead
    //         m_posY += std::sin(m_posX*0.1f)*1.5f;
    //     }
    //     publishPoseToZenoh();
    //     sendTelemetryToGCS();
    // });

    // Timers across different threads are not synchronised.
    // We are using the timer to just send data every 500ms, so it doesnt matter.
    // There is a possibility to use master clock/timer
    // m_timer->start(500); //Starts the timer to fire every 500ms
}

void DroneNode::onPeerPoseReceived(uint8_t peerId, float px, float py, float pz) {
    // Ignore our own published messages echoing back from the wildcard subscription
    if (peerId != m_id) {
        m_peers[peerId] = QVector3D(px, py, pz);
    }
}

void DroneNode::updatePhysicsAndSwarm(){
    const float dt = 0.05f; //50ms time step
    QVector3D desiredVel(0.0f,0.0f,0.0f);

    if (move) {
        // 1. Orbit Goal: Gently fly in a 3D figure-eight / orbit around the origin
        float t = QDateTime::currentMSecsSinceEpoch() * 0.0005f;
        QVector3D orbitTarget(std::cos(t) * 35.0f, std::sin(t) * 35.0f, 15.0f + std::sin(t * 2.0f) * 5.0f);
        QVector3D toTarget = (orbitTarget - m_pos).normalized() * 6.0f; // 6 m/s cruise speed

        // 2. Decentralized Boids Forces (Separation + Cohesion) from Zenoh peers
        QVector3D separation(0, 0, 0);
        QVector3D centerOfMass(0, 0, 0);

        // Need to mark m_peers as const cause QMap is a non const QT container
        for (const QVector3D& peerPos : std::as_const(m_peers)) {
            centerOfMass += peerPos;
            QVector3D diff = m_pos - peerPos;
            float dist = diff.length();

            // Push away strongly if closer than 12 meters
            if (dist < 12.0f && dist > 0.01f) {
                separation += diff.normalized() * ((12.0f - dist) * 1.5f);
            }
        }

        QVector3D cohesion(0, 0, 0);
        if (!m_peers.isEmpty()) {
            centerOfMass /= static_cast<float>(m_peers.size());
            cohesion = (centerOfMass - m_pos) * 0.15f; // Gently pull toward group center
        }

        desiredVel = toTarget + separation + cohesion;
        // desiredVel = orbitTarget;
    }
    // If move == false (Paused by GCS), desiredVel stays (0,0,0), so the drone smoothly brakes to a hover!
    // Apply inertia / smooth acceleration (lerp current velocity toward desired velocity)
    float responsiveness = 2.5f * dt;
    m_vel += (desiredVel - m_vel) * responsiveness;

    // Integrate position: pos = pos + vel * dt
    m_pos += m_vel * dt;

    // Publish D2D pose over Zenoh at 10 Hz (every 2nd tick)
    if (m_tickCount % 2 == 0) {
        publishPoseToZenoh();
    }

    // Send MAVLink telemetry to GCS at 5 Hz (every 4th tick)
    if (m_tickCount % 4 == 0) {
        sendTelemetryToGCS();
    }

    m_tickCount++;
}

void DroneNode::publishPoseToZenoh(){
    if (!m_zenohReady) return;

    PeerPose myPose{m_id, m_pos.x(), m_pos.y(), m_pos.z()};
    z_owned_bytes_t payload;
    z_bytes_copy_from_buf(&payload, reinterpret_cast<const uint8_t*>(&myPose), sizeof(PeerPose));

    // Fire-and-Forget publish to all subscribed peers!
    z_publisher_put(z_loan(m_zPub), z_move(payload), nullptr);

}

void DroneNode::sendTelemetryToGCS(){

    mavlink_message_t msg;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint32_t bootTimeMs = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF);

    // 1. Pack and send MAVLink HEARTBEAT ("I am alive!")
    // Call a or `mavlink_msg_<message_name>_pack()` function to populate a `mavlink_message_t`,
    // then serialize it into a byte array with `mavlink_msg_to_send_buffer()`
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
    // writeDatagram(Raw payload, Len of payload, Host address, port)
    m_socket->writeDatagram(reinterpret_cast<const char*>(buffer), len, QHostAddress::LocalHost, gcsPort);

    // 2. Pack and send MAVLink LOCAL_POSITION_NED (3D Coordinates)
    // Note: In NED (North-East-Down), altitude up is negative Z, so we pass -m_posZ
    mavlink_msg_local_position_ned_pack(
        m_id,
        MAV_COMP_ID_AUTOPILOT1,
        &msg,
        bootTimeMs,
        m_pos.x(),     // X (North)
        m_pos.y(),     // Y (East)
        -m_pos.z(),    // Z (Down)
        m_vel.x(), m_vel.y(), -m_vel.z() // vx, vy, vz
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



// void DroneNode::broadcastStateToPeers() {
//     // Leader broadcasts position to peer drones on port 15000
//     // Kept as SwarmPacket temporarily until Phase 3 (Zenoh)
//     SwarmPacket d2dPacket{0xFD, m_id, 2, m_posX, m_posY, m_posZ, 0.0f};
//     QByteArray buffer(reinterpret_cast<const char*>(&d2dPacket), sizeof(d2dPacket));

//     for (uint8_t peerId = 1; peerId <= m_totalDrones; ++peerId){
//         if (peerId != m_id){
//             m_socket->writeDatagram(buffer, QHostAddress::LocalHost, 50000 + peerId);
//         }
//     }
// }

void DroneNode::readGcsDatagrams(){
    // Each drone thread uses its own MAVLink channel (MAVLINK_COMM_0 is thread-local if we use status struct)
    mavlink_message_t msg{};
    mavlink_status_t status{};

    while (m_socket->hasPendingDatagrams()){
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();


        // A. Check if it's our temporary D2D SwarmPacket (exact 19 bytes, msgType == 2)
        // Replaced by Zenoh Pub-Sub model
        // if (data.size() == sizeof(SwarmPacket)){
        //     // Deserialization. Mapping back to the struct format
        //     const auto* packet = reinterpret_cast<const SwarmPacket*>(data.constData());
        //     if  (packet->magic == 0xFD && packet->msgType == 2 && move){
        //         // Decentralized collision avoidance rule:
        //         // If a peer gets within 8 meters, nudge away slightly
        //         float dx = m_posX - packet->posX;
        //         float dy = m_posY - packet->posY;
        //         float dist = std::hypot(dx,dy);
        //         if (dist<8.0f && dist > 0.01f){
        //             m_posX += (dx/dist) * 1.0f;
        //             m_posY += (dy/dist) * 1.0f;
        //         }
        //         continue;
        //     }
        // }

        // B. Otherwise, parse as a MAVLink v2 message from the GCS!
        for (int i = 0; i < data.size(); ++i){
            // msg struct that accumulates incoming bytes
            // status tracks the current parser state, running CRC and error counters
            // incoming byte
            // compiled message
            // completed output status
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
